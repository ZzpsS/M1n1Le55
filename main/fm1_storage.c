#include "fm1_storage.h"
#include "fm1_exchange_file.h"
#include "fm1_adapter_wire.h"
#include "fm1_sample_format.h"
#include "fm1_package.h"
#include "fm1_host.h"
#include "bsp/esp-bsp.h"
#include "esp_vfs_fat.h"
#include "esp_timer.h"
#include "esp_log.h"
#include "esp_heap_caps.h"
#include "mbedtls/sha256.h"
#include "cJSON.h"
#include "freertos/FreeRTOS.h"
#include "freertos/semphr.h"
#include "freertos/queue.h"
#include <dirent.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <errno.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#define ROOT BSP_SD_MOUNT_POINT "/fm1"
#define LIB ROOT "/firmware"
#define BACKUPS ROOT "/backups"
static fm1_storage_state_t state;
static SemaphoreHandle_t guard,io;
static QueueHandle_t requests;
static FILE *upload;
static unsigned upload_kind;
static char upload_name[64],upload_path[160],upload_sha[65];
static uint32_t upload_size,upload_written;
static int64_t upload_time;
static bool mount_attempted;
static bool rescan_needed;
static TaskHandle_t sd_task;

static void message(const char *s,bool busy)
{
    xSemaphoreTake(guard,portMAX_DELAY);
    snprintf(state.message,sizeof(state.message),"%s",s);state.busy=busy;state.revision++;
    xSemaphoreGive(guard);
}
void fm1_storage_snapshot(fm1_storage_state_t *out)
{
    if(!guard){memset(out,0,sizeof(*out));return;}
    xSemaphoreTake(guard,portMAX_DELAY);*out=state;xSemaphoreGive(guard);
}
static bool exists(const char *path){struct stat s;return stat(path,&s)==0;}
static bool directory(const char *path)
{
    struct stat s;if(stat(path,&s)==0)return S_ISDIR(s.st_mode);
    return mkdir(path,0775)==0;
}
static void history(const char *text)
{
    xSemaphoreTake(guard,portMAX_DELAY);
    memmove(state.history[1],state.history[0],5*sizeof(state.history[0]));
    snprintf(state.history[0],sizeof(state.history[0]),"%s",text);state.revision++;
    xSemaphoreGive(guard);
}
static void record(const char *text)
{
    history(text);
    FILE *f=fopen(ROOT "/activity.log","a");
    if(f){fprintf(f,"%s\n",text);fclose(f);}
}
static bool digest_file(const char *path,char result[65])
{
    FILE *f=fopen(path,"rb");if(!f)return false;
    mbedtls_sha256_context ctx;mbedtls_sha256_init(&ctx);
    bool ok=mbedtls_sha256_starts(&ctx,0)==0;
    uint8_t bytes[2048],hash[32];size_t n;unsigned blocks=0;
    while(ok && (n=fread(bytes,1,sizeof(bytes),f))>0) {
        ok=mbedtls_sha256_update(&ctx,bytes,n)==0;if(++blocks%32==0)vTaskDelay(1);
    }
    ok=ok&&!ferror(f);if(fclose(f))ok=false;
    if(ok)ok=mbedtls_sha256_finish(&ctx,hash)==0;
    mbedtls_sha256_free(&ctx);
    if(ok)fm1_hash_text(hash,result);
    return ok;
}
static int package_order(const void *a,const void *b)
{return strcmp(((const fm1_package_t*)a)->name,((const fm1_package_t*)b)->name);}
static void scan(void)
{
    if(upload)return;
    message("Reading SD card...",true);
    if(!state.mounted) {
        // A failed BSP mount may retain its LDO handle. Retry only after reboot.
        if(mount_attempted){message("SD unavailable. Check FAT32 card, then restart Tab5.",false);return;}
        mount_attempted=true;
        const esp_vfs_fat_sdmmc_mount_config_t mount={.format_if_mount_failed=false,.max_files=5,.allocation_unit_size=16384};
        bsp_sdcard_cfg_t config={.mount=&mount};
        esp_err_t err=bsp_sdcard_sdmmc_mount(&config);
        if(err!=ESP_OK){char s[120];snprintf(s,sizeof(s),"SD unavailable (%s). Card was not formatted.",esp_err_to_name(err));message(s,false);return;}
        sdmmc_card_t *card=bsp_sdcard_get_handle();
        xSemaphoreTake(guard,portMAX_DELAY);state.mounted=true;
        state.capacity_mb=(uint64_t)card->csd.capacity*card->csd.sector_size/1024/1024;xSemaphoreGive(guard);
        FILE *log=fopen(ROOT "/activity.log","r");
        if(log){char line[120];while(fgets(line,sizeof(line),log)){line[strcspn(line,"\r\n")]=0;history(line);}fclose(log);}
    }
    if(!directory(ROOT)||!directory(LIB)||!directory(BACKUPS)){message("SD folder unavailable / check free space or card.",false);return;}
    fm1_package_t *files=calloc(FM1_LIBRARY_MAX,sizeof(*files));
    if(!files){message("Not enough memory to read library.",false);return;}
    DIR *dir=opendir(LIB);unsigned count=0,omitted=0;
    if(!dir){free(files);message("Cannot read firmware folder.",false);return;}
    struct dirent *entry;
    while((entry=readdir(dir))) {
        if(!fm1_package_name(entry->d_name))continue;
        char path[180];snprintf(path,sizeof(path),LIB "/%.63s",entry->d_name);
        struct stat st;if(stat(path,&st)||!S_ISREG(st.st_mode))continue;
        if(count==FM1_LIBRARY_MAX){omitted++;continue;}
        fm1_package_t *p=&files[count++];snprintf(p->name,sizeof(p->name),"%.63s",entry->d_name);
        p->size=st.st_size;
        if(st.st_size<=960 || st.st_size>FM1_PACKAGE_LIMIT){strcpy(p->check,"Invalid package size");continue;}
        uint8_t header[960];FILE *f=fopen(path,"rb");size_t n=f?fread(header,1,sizeof(header),f):0;if(f)fclose(f);
        p->model_ok=fm1_package_identity(header,n,p->identity);
        if(!p->model_ok){strcpy(p->check,"Not an FM-1 package");continue;}
        if(!digest_file(path,p->sha256)){strcpy(p->check,"SD read error");continue;}
        strcat(path,".sha256");f=fopen(path,"rb");char expected[66]={0};
        n=f?fread(expected,1,65,f):0;if(f)fclose(f);
        uint8_t parsed[32];
        if(n==65 && (expected[64]=='\n'||expected[64]=='\r'))expected[64]=0;
        if(!fm1_hash_parse(expected,parsed)){strcpy(p->check,"Needs SHA-256 reference");continue;}
        char normalized[65];fm1_hash_text(parsed,normalized);p->hash_match=strcmp(normalized,p->sha256)==0;
        strcpy(p->check,p->hash_match?"SHA-256 matches reference":"SHA-256 MISMATCH");
    }
    closedir(dir);qsort(files,count,sizeof(*files),package_order);
    unsigned backups=0;char latest[64]="";
    dir=opendir(BACKUPS);
    if(dir){while((entry=readdir(dir))) {
        unsigned id;char canonical[32];
        if(sscanf(entry->d_name,"voice-%5u",&id)==1) {
            snprintf(canonical,sizeof(canonical),"voice-%05u.json",id);
            if(strcmp(canonical,entry->d_name))continue;
            backups++;if(strcmp(entry->d_name,latest)>0)snprintf(latest,sizeof(latest),"%.63s",entry->d_name);
        }
    }closedir(dir);}
    xSemaphoreTake(guard,portMAX_DELAY);
    memcpy(state.files,files,FM1_LIBRARY_MAX*sizeof(*files));state.count=count;state.omitted=omitted;state.backups=backups;
    strcpy(state.last_backup,latest);xSemaphoreGive(guard);free(files);
    char text[120];snprintf(text,sizeof(text),"%u package%s on SD%s",count,count==1?"":"s",omitted?" / library limit reached":"");message(text,false);
}
static void backup_voice(void)
{
    if(upload)return;
    if(!state.mounted){message("Insert a readable SD card before backup.",false);return;}
    fm1_state_t s;fm1_host_snapshot(&s);
    if(!s.valid_voice||!s.can_edit||!s.connected||esp_timer_get_time()-s.last_read_us>=3000000) {
        message("Backup needs a fresh stock FM-1 voice.",false);return;
    }
    message("Saving current voice...",true);
    char path[180],final[180],name[64];int fd=-1;
    for(unsigned i=1;i<=99999;i++) {
        snprintf(name,sizeof(name),"voice-%05u.json",i);snprintf(final,sizeof(final),BACKUPS "/%s",name);
        snprintf(path,sizeof(path),BACKUPS "/%s.part",name);if(exists(final))continue;
        fd=open(path,O_WRONLY|O_CREAT|O_EXCL,0666);if(fd>=0)break;if(errno!=EEXIST)break;
    }
    if(fd<0){message("Could not create backup. Check SD space.",false);return;}
    cJSON *j=cJSON_CreateObject();char hex[311],sha[65];uint8_t hash[32];
    for(unsigned i=0;i<155;i++)snprintf(hex+i*2,3,"%02x",s.voice[i]);
    mbedtls_sha256(s.voice,155,hash,0);fm1_hash_text(hash,sha);
    cJSON_AddStringToObject(j,"format","tab5-fm1-current-voice-v1");
    cJSON_AddStringToObject(j,"identity",s.identity);cJSON_AddStringToObject(j,"voice_hex",hex);
    cJSON_AddStringToObject(j,"sha256",sha);cJSON_AddStringToObject(j,"scope","Current 155-byte voice only; excludes presets, effects and firmware");
    char *json=cJSON_PrintUnformatted(j);cJSON_Delete(j);bool ok=json!=NULL;
    FILE *f=fdopen(fd,"wb");
    if(!f){close(fd);ok=false;}else {if(ok)ok=fwrite(json,1,strlen(json),f)==strlen(json);if(fclose(f))ok=false;}
    if(ok) {
        char readback[1024];f=fopen(path,"rb");size_t n=f?fread(readback,1,sizeof(readback),f):0;
        if(f)fclose(f);
        ok=n==strlen(json)&&memcmp(readback,json,n)==0;
    }
    free(json);
    if(ok)ok=!exists(final)&&rename(path,final)==0;
    if(!ok){unlink(path);message("Backup failed SD readback check.",false);return;}
    xSemaphoreTake(guard,portMAX_DELAY);state.backups++;snprintf(state.last_backup,sizeof(state.last_backup),"%s",name);state.revision++;xSemaphoreGive(guard);
    char text[120];snprintf(text,sizeof(text),"Voice saved + verified: %s",name);record(text);message(text,false);
}
static void abort_upload(void)
{
    if(upload){fclose(upload);upload=NULL;unlink(upload_path);message("Import cancelled; existing files kept.",false);}
}
void fm1_storage_upload_abort(void)
{xSemaphoreTake(io,portMAX_DELAY);abort_upload();xSemaphoreGive(io);}
static bool upload_begin(const char *name,uint32_t size,const char *sha,unsigned kind)
{
    bool sample=kind==1,exchange=kind==2;
    fm1_state_t device;fm1_host_snapshot(&device);if(device.updating)return false;
    uint8_t hash[32];if(!(exchange?(fm1_exchange_name(name)&&size>=64&&size<=FM1_EXCHANGE_MAX+64):sample?(fm1_sample_name(name)&&size>=512&&size<=FM1_SAMPLE_CAPACITY):(fm1_package_name(name)&&size>960&&size<=FM1_PACKAGE_LIMIT))||!fm1_hash_parse(sha,hash))return false;
    xSemaphoreTake(io,portMAX_DELAY);bool ok=false;
    const char *folder=exchange?ROOT "/exchange":sample?ROOT "/samples":LIB;
    if(state.mounted&&!upload&&(kind||state.count<FM1_LIBRARY_MAX)&&(!kind||directory(folder))) {
        char final[180],sidecar[180];snprintf(final,sizeof(final),"%s/%.63s",folder,name);snprintf(sidecar,sizeof(sidecar),"%s/%.63s.sha256",folder,name);
        snprintf(upload_path,sizeof(upload_path),"%s/%s.part",folder,name);
        if(!exists(final)&&!exists(sidecar)) {
            int fd=open(upload_path,O_WRONLY|O_CREAT|O_EXCL,0666);
            if(fd>=0){upload=fdopen(fd,"wb");if(!upload){close(fd);unlink(upload_path);}}
            if(upload) {
                upload_kind=kind;strcpy(upload_name,name);fm1_hash_text(hash,upload_sha);upload_size=size;upload_written=0;
                upload_time=esp_timer_get_time();message("Importing package to SD...",true);ok=true;
            }
        }
    }
    xSemaphoreGive(io);return ok;
}
bool fm1_storage_upload_data(uint32_t offset,const uint8_t *bytes,size_t n)
{
    xSemaphoreTake(io,portMAX_DELAY);
    bool ok=upload&&n&&n<=FM1_UPLOAD_CHUNK&&offset==upload_written&&n<=upload_size-upload_written;
    if(ok){ok=fwrite(bytes,1,n,upload)==n;if(ok){upload_written+=n;upload_time=esp_timer_get_time();}else abort_upload();}
    xSemaphoreGive(io);return ok;
}
bool fm1_storage_upload_end(void)
{
    xSemaphoreTake(io,portMAX_DELAY);bool ok=false;
    if(upload && upload_written==upload_size) {
        ok=fclose(upload)==0;upload=NULL;char digest[65];
        ok=ok&&digest_file(upload_path,digest)&&strcmp(digest,upload_sha)==0;
        uint8_t header[960];char identity[21];FILE *f=fopen(upload_path,"rb");size_t n=f?fread(header,1,960,f):0;if(f)fclose(f);
        if(upload_kind) {
            uint8_t *bank=heap_caps_malloc(upload_size,MALLOC_CAP_SPIRAM|MALLOC_CAP_8BIT);
            f=bank?fopen(upload_path,"rb"):NULL;
            unsigned object;
            bool valid=f&&fread(bank,1,upload_size,f)==upload_size;
            if(valid)valid=upload_kind==1?fm1_sample_validate(bank,upload_size):
                (memchr(bank+8,0,32)&&fm1_adapter_profile((const char *)bank+8)->id!=FW_UNKNOWN&&fm1_exchange_validate(bank,upload_size,(const char *)bank+8,&object));
            if(f)fclose(f);
            free(bank);ok=ok&&valid;
        } else ok=ok&&fm1_package_identity(header,n,identity);
        const char *folder=upload_kind==2?ROOT "/exchange":upload_kind==1?ROOT "/samples":LIB;
        char final[180],ref[180];snprintf(final,sizeof(final),"%s/%s",folder,upload_name);snprintf(ref,sizeof(ref),"%s/%s.sha256",folder,upload_name);
        bool created=false;
        if(ok) {
            int fd=open(ref,O_WRONLY|O_CREAT|O_EXCL,0666);ok=fd>=0;created=ok;
            if(ok){ok=write(fd,upload_sha,64)==64;if(close(fd))ok=false;}
        }
        if(ok)ok=!exists(final)&&rename(upload_path,final)==0;
        if(!ok){unlink(upload_path);if(created)unlink(ref);message("Import rejected: check file, model or SHA-256.",false);}
        else {char text[120];snprintf(text,sizeof(text),"Imported: %s",upload_name);record(text);rescan_needed=true;message("Import saved; refreshing library...",true);}
    } else abort_upload();
    xSemaphoreGive(io);return ok;
}
bool fm1_storage_request(bool backup)
{
    fm1_state_t device;fm1_host_snapshot(&device);if(device.updating)return false;
    return requests&&xQueueSend(requests,&backup,0)==pdTRUE;
}
bool fm1_storage_backup_now(char name[64])
{
    xSemaphoreTake(io,portMAX_DELAY);unsigned before=state.backups;
    if(!upload)backup_voice();
    bool ok=state.backups==before+1;
    if(ok)snprintf(name,64,"%s",state.last_backup);
    xSemaphoreGive(io);return ok;
}
bool fm1_storage_load_package(const char *name,uint8_t **bytes,size_t *size,fm1_package_t *meta)
{
    *bytes=NULL;*size=0;if(!fm1_package_name(name))return false;
    xSemaphoreTake(io,portMAX_DELAY);bool ok=false;
    char path[180];snprintf(path,sizeof(path),LIB "/%.63s",name);struct stat info;
    if(state.mounted&&!upload&&stat(path,&info)==0&&S_ISREG(info.st_mode)&&info.st_size>960&&info.st_size<=FM1_PACKAGE_LIMIT) {
        message("Loading and verifying firmware...",true);
        uint8_t *data=heap_caps_malloc(info.st_size,MALLOC_CAP_SPIRAM|MALLOC_CAP_8BIT);
        FILE *f=data?fopen(path,"rb"):NULL;
        if(f) {
            size_t n=fread(data,1,info.st_size,f);ok=n==(size_t)info.st_size&&!ferror(f)&&fgetc(f)==EOF;fclose(f);
            memset(meta,0,sizeof(*meta));snprintf(meta->name,sizeof(meta->name),"%s",name);meta->size=n;
            uint8_t digest[32];ok=ok&&fm1_package_identity(data,n,meta->identity)&&mbedtls_sha256(data,n,digest,0)==0;
            if(ok){fm1_hash_text(digest,meta->sha256);*bytes=data;*size=n;}
        }
        if(!ok)free(data);
    }
    message(ok?"Firmware loaded from SD":"Unable to load firmware from SD",false);
    xSemaphoreGive(io);return ok;
}
bool fm1_storage_update_record(const char *name,const char *phase,const char *detail)
{
    if(!fm1_package_name(name))return false;
    xSemaphoreTake(io,portMAX_DELAY);bool ok=false;
    if(state.mounted&&!upload) {
        cJSON *j=cJSON_CreateObject();cJSON_AddStringToObject(j,"package",name);cJSON_AddStringToObject(j,"phase",phase);cJSON_AddStringToObject(j,"detail",detail);
        char *json=cJSON_PrintUnformatted(j);cJSON_Delete(j);
        FILE *f=json?fopen(ROOT "/last-update.json","wb"):NULL;
        if(f){ok=fwrite(json,1,strlen(json),f)==strlen(json);if(fclose(f))ok=false;}
        if(ok) {
            char check[512];f=fopen(ROOT "/last-update.json","rb");size_t n=f?fread(check,1,sizeof(check),f):0;if(f)fclose(f);
            ok=n==strlen(json)&&memcmp(check,json,n)==0;
        }
        free(json);
        if(ok){char text[120];snprintf(text,sizeof(text),"%s: %.63s",phase,name);record(text);}
    }
    xSemaphoreGive(io);return ok;
}
unsigned fm1_storage_stack_min(void){return sd_task?uxTaskGetStackHighWaterMark(sd_task):0;}
bool fm1_storage_read_backup(const char *name,char *out,size_t capacity)
{
    unsigned id;char canonical[32];
    if(capacity<2||sscanf(name,"voice-%5u",&id)!=1||id<1)return false;
    snprintf(canonical,sizeof(canonical),"voice-%05u.json",id);if(strcmp(name,canonical))return false;
    xSemaphoreTake(io,portMAX_DELAY);
    char path[180];snprintf(path,sizeof(path),BACKUPS "/%s",canonical);FILE *f=state.mounted?fopen(path,"rb"):NULL;
    bool ok=false;
    if(f){size_t n=fread(out,1,capacity-1,f);out[n]=0;ok=n>0&&!ferror(f)&&fgetc(f)==EOF;fclose(f);}
    xSemaphoreGive(io);return ok;
}
static void worker(void *arg)
{
    (void)arg;xSemaphoreTake(io,portMAX_DELAY);scan();xSemaphoreGive(io);
    for(;;) {
        bool backup;
        if(xQueueReceive(requests,&backup,pdMS_TO_TICKS(200))==pdTRUE) {
            fm1_state_t device;fm1_host_snapshot(&device);
            xSemaphoreTake(io,portMAX_DELAY);if(!device.updating){if(backup)backup_voice();else scan();}xSemaphoreGive(io);
        } else {
            xSemaphoreTake(io,portMAX_DELAY);
            if(upload&&esp_timer_get_time()-upload_time>15000000)abort_upload();
            if(rescan_needed&&!upload){rescan_needed=false;scan();}
            xSemaphoreGive(io);
        }
    }
}
void fm1_storage_start(void)
{
    guard=xSemaphoreCreateMutex();io=xSemaphoreCreateMutex();requests=xQueueCreate(2,sizeof(bool));
    if(!guard||!io||!requests)abort();
    strcpy(state.message,"Starting SD card...");
    if(xTaskCreate(worker,"fm1_sd",12288,NULL,2,&sd_task)!=pdPASS)abort();
}

bool fm1_storage_upload_begin(const char *n,uint32_t s,const char *h){return upload_begin(n,s,h,false);}
bool fm1_storage_sample_upload_begin(const char *n,uint32_t s,const char *h){return upload_begin(n,s,h,true);}
bool fm1_storage_exchange_upload_begin(const char *n,uint32_t s,const char *h){return upload_begin(n,s,h,2);}
bool fm1_storage_external_begin(void)
{
    if(!io||xSemaphoreTake(io,pdMS_TO_TICKS(2000))!=pdTRUE)return false;
    fm1_state_t device;fm1_host_snapshot(&device);
    if(!state.mounted||upload||device.updating){xSemaphoreGive(io);return false;}
    message("Sample library in use...",true);return true;
}
void fm1_storage_external_end(void){message("Sample library ready",false);xSemaphoreGive(io);}
