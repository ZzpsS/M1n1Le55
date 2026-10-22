#include "fm1_samples.h"
#include "fm1_sample_format.h"
#include "m1_waveform.h"
#include "fm1_ble.h"
#include "fm1_storage.h"
#include "bsp/esp-bsp.h"
#include "esp_timer.h"
#include "esp_heap_caps.h"
#include "freertos/FreeRTOS.h"
#include "freertos/semphr.h"
#include "freertos/queue.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <dirent.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <errno.h>

#define LIB BSP_SD_MOUNT_POINT "/fm1/samples"
typedef struct {unsigned kind,value;uint32_t connection;bool full_readback,recovery;char name[64];} request_t;
static SemaphoreHandle_t guard;
static QueueHandle_t requests;
static fm1_samples_state_t state;
static uint8_t *source;
static unsigned source_size,token;
static int64_t started;
static uint32_t active_connection;
static void lock(void){xSemaphoreTake(guard,portMAX_DELAY);}
static void unlock(void){xSemaphoreGive(guard);}
static void stage(fm1_sample_phase_t phase,const char *message,unsigned total)
{lock();state.phase=phase;snprintf(state.message,sizeof(state.message),"%s",message);state.done=0;state.total=total;state.revision++;unlock();}
static void progress(unsigned n){lock();state.done=n;state.elapsed_ms=(esp_timer_get_time()-started)/1000;unlock();}
void fm1_samples_snapshot(fm1_samples_state_t *out)
{
    if(!guard){memset(out,0,sizeof(*out));return;}
    fm1_ble_state_t ble;fm1_ble_snapshot(&ble);lock();*out=state;unlock();
    out->confirmation_valid=out->prepared&&ble.phase==FM1_BLE_READY&&ble.connection_id==out->connection_id;
    out->device_known=out->device_known&&ble.phase==FM1_BLE_READY&&ble.connection_id==out->device_connection_id;
    out->device_verified=out->device_verified&&out->device_known;
    /* A peer name is not a stable device identity. Never imply that an old
     * session's automatic recovery backup belongs to a newly connected device. */
    out->result_backup_current=out->result_backup[0]&&ble.phase==FM1_BLE_READY&&
        ble.connection_id==out->result_backup_connection_id;
}
static bool same_connection(void)
{fm1_ble_state_t ble;fm1_ble_snapshot(&ble);return ble.phase==FM1_BLE_READY&&ble.connection_id==active_connection;}
static bool exists(const char *p){struct stat s;return stat(p,&s)==0;}
static bool library(void){struct stat s;return stat(LIB,&s)==0?S_ISDIR(s.st_mode):mkdir(LIB,0775)==0;}
static int order(const void *a,const void *b){return strcmp(a,b);}
static bool scan(void)
{
    if(!library())return false;
    DIR *dir=opendir(LIB);if(!dir)return false;
    char (*names)[64]=calloc(FM1_SAMPLE_FILES,64);if(!names){closedir(dir);return false;}
    struct dirent *e;unsigned n=0,total=0;
    while((e=readdir(dir)))if(fm1_sample_name(e->d_name)) {
        char p[180];snprintf(p,sizeof(p),LIB "/%.63s",e->d_name);struct stat s;
        if(!stat(p,&s)&&S_ISREG(s.st_mode)){
            total++;
            if(n<FM1_SAMPLE_FILES)snprintf(names[n++],64,"%.63s",e->d_name);
            else {qsort(names,n,64,order);if(strcmp(e->d_name,names[n-1])<0)snprintf(names[n-1],64,"%.63s",e->d_name);}
        }
    }
    closedir(dir);qsort(names,n,64,order);
    lock();memcpy(state.files,names,FM1_SAMPLE_FILES*64);state.count=n;state.omitted=total-n;state.revision++;unlock();free(names);return true;
}
static bool query(unsigned cmd,const uint8_t *data,unsigned size,uint8_t *reply,unsigned expected)
{
    uint8_t frame[192],rx[192],decoded[65];token=(token+1)&16383;
    unsigned n=fm1_sample_encode(frame,cmd,token,data,size);if(!n)return false;
    uint32_t seq;fm1_ble_sysex_last(rx,sizeof(rx),&seq);
    /* Retries keep the exact command and token so FM1 can return its cached result. */
    for(unsigned attempt=0;attempt<3;attempt++) {
        if(attempt){lock();state.retries++;unlock();}
        int64_t deadline=esp_timer_get_time()+1000000;
        for(;;) {
            if(!same_connection())return false;
            if(fm1_ble_sysex_send(frame,n))break;
            if(esp_timer_get_time()>deadline)return false;
            vTaskDelay(pdMS_TO_TICKS(10));
        }
        deadline=esp_timer_get_time()+3000000;
        while(esp_timer_get_time()<deadline) {
            if(!same_connection())return false;
            uint32_t current;unsigned count=fm1_ble_sysex_last(rx,sizeof(rx),&current);
            if(current!=seq) {
                seq=current;int length=fm1_sample_decode(rx,count,cmd,token,decoded);
                if(length>0){if(decoded[0]||length!=(int)expected+1||!same_connection())return false;if(expected)memcpy(reply,decoded+1,expected);return true;}
            }
            fm1_ble_state_t s;fm1_ble_snapshot(&s);if(s.phase!=FM1_BLE_READY)return false;
            fm1_ble_sysex_wait(20);
        }
    }
    return false;
}
static bool info(unsigned *size,unsigned *status)
{
    uint8_t app[14],bank[9];
    if(!query(0,NULL,0,app,14)||app[0]!=4||!query(11,NULL,0,bank,9))return false;
    *status=bank[0];*size=fm1_sample_u32(bank+1);
    return *status<=4&&fm1_sample_u32(bank+5)==FM1_SAMPLE_CAPACITY&&(*status==2?(*size>=512&&*size<=FM1_SAMPLE_CAPACITY):!*size);
}
static uint8_t *read_bank(unsigned size)
{
    uint8_t *p=heap_caps_malloc(size,MALLOC_CAP_SPIRAM|MALLOC_CAP_8BIT);if(!p)return NULL;
    for(unsigned offset=0;offset<size;) {
        unsigned n=size-offset;if(n>64)n=64;
        uint8_t args[5];fm1_sample_put32(args,offset);args[4]=n;
        if(!query(15,args,5,p+offset,n)){free(p);return NULL;}offset+=n;progress(offset);
    }
    if(!fm1_sample_validate(p,size)){free(p);return NULL;}return p;
}
static bool save_backup(const uint8_t *p,unsigned size)
{
    char path[180],final[180],name[64];int fd=-1;
    for(unsigned i=1;i<=99999;i++) {
        snprintf(name,sizeof(name),"backup-%05u.fm1s",i);snprintf(final,sizeof(final),LIB "/%s",name);
        snprintf(path,sizeof(path),LIB "/%s.part",name);if(exists(final))continue;
        fd=open(path,O_WRONLY|O_CREAT|O_EXCL,0666);if(fd>=0||errno!=EEXIST)break;
    }
    if(fd<0)return false;
    FILE *f=fdopen(fd,"wb");bool ok=f!=NULL;
    if(f){ok=fwrite(p,1,size,f)==size;if(fflush(f)||fsync(fileno(f)))ok=false;if(fclose(f))ok=false;}else close(fd);
    if(ok) {
        f=fopen(path,"rb");ok=f!=NULL;uint8_t part[1024];unsigned offset=0;
        while(ok&&offset<size){unsigned n=size-offset;if(n>sizeof(part))n=sizeof(part);ok=fread(part,1,n,f)==n&&!memcmp(part,p+offset,n);offset+=n;}
        if(f){ok=ok&&fgetc(f)==EOF&&!ferror(f);if(fclose(f))ok=false;}
    }
    if(ok)ok=!exists(final)&&rename(path,final)==0;
    if(!ok){unlink(path);return false;}
    lock();strcpy(state.backup,name);strcpy(state.result_backup,name);
    state.result_backup_connection_id=active_connection;state.revision++;unlock();return true;
}
static bool prepare(const char *name,bool confirm,bool recovery)
{
    char path[180];snprintf(path,sizeof(path),LIB "/%s",name);struct stat st;
    if(stat(path,&st)||!S_ISREG(st.st_mode)||st.st_size<512||(size_t)st.st_size>FM1_SAMPLE_CAPACITY)return false;
    uint8_t *p=heap_caps_malloc(st.st_size,MALLOC_CAP_SPIRAM|MALLOC_CAP_8BIT);FILE *f=p?fopen(path,"rb"):NULL;
    bool ok=f&&fread(p,1,st.st_size,f)==(size_t)st.st_size&&fgetc(f)==EOF&&!ferror(f)&&fm1_sample_validate(p,st.st_size);
    if(f)fclose(f);
    if(!ok){free(p);return false;}
    fm1_ble_state_t ble;fm1_ble_snapshot(&ble);
    if(confirm&&(ble.phase!=FM1_BLE_READY||ble.connection_id!=active_connection)){free(p);return false;}
    uint8_t peaks[8][64];uint32_t frames[8];
    if(!m1_sample_peaks(p,st.st_size,peaks,frames)){free(p);return false;}
    free(source);source=p;source_size=st.st_size;
    lock();strcpy(state.name,name);state.bytes=source_size;state.token++;state.prepared=confirm;state.recovery=confirm&&recovery;
    state.connection_id=ble.connection_id;snprintf(state.peer,sizeof(state.peer),"%s",ble.peer);
    memcpy(state.peaks,peaks,sizeof(peaks));memcpy(state.frames,frames,sizeof(frames));
    for(unsigned i=0;i<8;i++)memcpy(state.slots[i],p+44+i*28,16);
    unlock();return true;
}
static void remember_device(const uint8_t *p,unsigned size,const char *name,uint32_t connection,bool verified)
{
    uint8_t peaks[8][64];uint32_t frames[8];if(!m1_sample_peaks(p,size,peaks,frames))return;
    lock();state.device_bytes=size;state.device_known=true;state.device_verified=verified;state.device_connection_id=connection;
    snprintf(state.device_name,sizeof(state.device_name),"%s",name);
    memcpy(state.device_peaks,peaks,sizeof(peaks));memcpy(state.device_frames,frames,sizeof(frames));
    for(unsigned i=0;i<8;i++)memcpy(state.device_slots[i],p+44+i*28,16);
    unlock();
}
static bool transfer(bool writing,bool full_readback,bool recovery)
{
    fm1_ble_state_t ble;fm1_ble_snapshot(&ble);
    if(!same_connection()||(writing&&(!source||ble.connection_id!=state.connection_id)))return false;
    uint32_t connection=ble.connection_id;
    stage(SAMPLE_QUERY,"Checking Bluetooth sample protocol and device bank...",0);
    unsigned size,status;if(!info(&size,&status))return false;
    /* Validated recovery sources can replace an incomplete bank. The device's
     * BEGIN restarts receiving from any state; a readable old bank is still
     * backed up first. Never overwrite a bank still validating at boot (1). */
    if(status!=2&&!(writing&&(status==0||(recovery&&(status==3||status==4)))))return false;
    if(status==2) {
        stage(SAMPLE_BACKUP,"Backing up FM-1 to SD before any replacement...",size);
        uint8_t *old=read_bank(size);if(!old)return false;
        bool ok=save_backup(old,size)&&same_connection();if(ok)remember_device(old,size,state.backup,connection,true);free(old);if(!ok)return false;
    }
    if(!writing)return true;
    fm1_ble_snapshot(&ble);if(ble.phase!=FM1_BLE_READY||ble.connection_id!=connection)return false;
    lock();state.device_known=false;unlock();
    stage(SAMPLE_WRITING,"Uploading sample bank over Bluetooth...",source_size);
    uint8_t payload[132];fm1_sample_put32(payload,source_size);
    int64_t start=esp_timer_get_time();
    if(!query(12,payload,4,NULL,0)){query(16,NULL,0,NULL,0);return false;}
    for(unsigned offset=0;offset<source_size;) {
        unsigned n=source_size-offset;if(n>128)n=128;
        fm1_sample_put32(payload,offset);memcpy(payload+4,source+offset,n);
        if(!query(13,payload,n+4,NULL,0)){query(16,NULL,0,NULL,0);return false;}offset+=n;progress(offset);
    }
    if(!query(14,NULL,0,NULL,0)){query(16,NULL,0,NULL,0);return false;}
    lock();state.write_ms=(esp_timer_get_time()-start)/1000;unlock();
    if(!info(&size,&status)||status!=2||size!=source_size||!same_connection())return false;
    if(!full_readback){remember_device(source,size,state.name,connection,false);return true;}
    stage(SAMPLE_VERIFYING,"Reading back every byte from FM-1...",source_size);start=esp_timer_get_time();
    uint8_t *after=read_bank(size);bool ok=after&&!memcmp(source,after,size)&&same_connection();
    if(ok)remember_device(after,size,state.name,connection,true);
    free(after);
    lock();state.verify_ms=(esp_timer_get_time()-start)/1000;state.verified=ok;unlock();return ok;
}
static bool play(unsigned slot)
{
    static const uint8_t notes[]={36,38,37,41,47,48,42,46};uint8_t app[14];
    if(!query(0,NULL,0,app,14)||app[0]!=4)return false;
    unsigned track;for(track=0;track<8&&app[6+track]!=5;track++){}
    if(track==8)return false;
    int64_t deadline=esp_timer_get_time()+2000000;
    while(!fm1_ble_play_note(track==7?9:track,notes[slot])){if(esp_timer_get_time()>deadline)return false;vTaskDelay(pdMS_TO_TICKS(10));}
    do {fm1_ble_state_t s;fm1_ble_snapshot(&s);if(!s.pending&&!s.testing)return s.phase==FM1_BLE_READY;vTaskDelay(pdMS_TO_TICKS(20));}while(esp_timer_get_time()<deadline);
    return false;
}
static void worker(void *arg)
{
    (void)arg;request_t r;
    for(;;)if(xQueueReceive(requests,&r,portMAX_DELAY)==pdTRUE) {
        started=esp_timer_get_time();active_connection=r.connection;bool sd=false,ble=false,ok=false;
        if(r.kind==5){free(source);source=NULL;source_size=0;lock();state.prepared=state.recovery=false;unlock();ok=true;}
        else {
            if(r.kind==3||r.kind==4||r.kind==6){ble=same_connection()&&fm1_ble_session_begin();if(!ble){lock();state.failed_phase=state.phase;unlock();stage(SAMPLE_ERROR,"Bluetooth session changed or unavailable. Reconnect and prepare again.",0);goto finish;}}
            if(r.kind!=6){sd=fm1_storage_external_begin();if(!sd){lock();state.failed_phase=state.phase;unlock();stage(SAMPLE_ERROR,"SD unavailable or busy. Try again.",0);goto finish;}}
            if(r.kind==1)ok=scan();
            else if(r.kind==2||r.kind==7||r.kind==8||r.kind==9)ok=prepare(r.name,r.kind!=7,r.kind==8||r.kind==9);
            else if(r.kind==3||r.kind==4){ok=library()&&transfer(r.kind==3,r.full_readback,r.recovery);scan();}
            else if(r.kind==6)ok=play(r.value);
        }
        lock();if(!ok)state.failed_phase=state.phase;unlock();
        stage(ok?(r.kind==2||r.kind==8||r.kind==9?SAMPLE_PREPARED:r.kind==7?SAMPLE_PREVIEW:SAMPLE_DONE):SAMPLE_ERROR,
            ok?(r.kind==8||r.kind==9?"Recovery ready for this connection. Replaces all 8 slots; readable banks are backed up first.":r.kind==2?"Ready: replaces all 8 slots. Existing bank is backed up first.":r.kind==3?(r.full_readback?"Complete: all bytes read back and matched.":"Complete: device committed the bank and confirmed its size. Full readback skipped."):r.kind==4?"Backup saved and verified on SD.":r.kind==6?"Sample note sent to the existing sampler track.":r.kind==7?"SD preview only. Device contents unchanged.":"Choose a .fm1s sample bank from SD."):"Operation failed. Check the failed phase and recovery backup.",0);
finish:
        if(r.kind==3){free(source);source=NULL;source_size=0;lock();state.prepared=state.recovery=false;unlock();}
        if(sd)fm1_storage_external_end();
        if(ble)fm1_ble_session_end();
        lock();
        if(r.kind==3||r.kind==4){
            if(ok)snprintf(state.result,sizeof(state.result),"%s",r.kind==3?(r.full_readback?"采样替换完成：全部字节读回一致。":"采样替换完成：设备已提交并确认大小，未完整读回。"):
                "采样库备份已写入 SD 并读回校验。");
            else snprintf(state.result,sizeof(state.result),"失败阶段 %u：%.96s",state.failed_phase,state.message);
            /* The last durable backup survives failures before a new backup is
             * made. Its connection binding is updated only by save_backup(). */
            state.completed=state.result_completed=ok;state.result_verified=ok&&(r.kind==4||state.verified);
        }
        state.busy=false;state.elapsed_ms=(esp_timer_get_time()-started)/1000;state.revision++;unlock();
    }
}
static bool submit(unsigned kind,unsigned value,const char *name,bool full_readback)
{
    if(!guard)return false;
    request_t r={.kind=kind,.value=value,.full_readback=full_readback};if(name)snprintf(r.name,sizeof(r.name),"%s",name);
    fm1_ble_state_t ble;fm1_ble_snapshot(&ble);
    r.connection=ble.connection_id;
    lock();bool ok=!state.busy;
    if(kind==3||kind==5)ok=ok&&state.prepared&&value==state.token;
    if(kind==3)ok=ok&&ble.phase==FM1_BLE_READY&&ble.connection_id==state.connection_id;
    if(kind==1||kind==2||kind==4||kind==6||kind==7||kind==8||kind==9)ok=ok&&!state.prepared;
    if(kind==9){
        ok=ok&&ble.phase==FM1_BLE_READY&&ble.connection_id==value&&
            state.result_backup_connection_id==value&&fm1_sample_name(state.result_backup);
        if(ok)snprintf(r.name,sizeof(r.name),"%s",state.result_backup);
    }
    if(ok){r.recovery=kind==3&&state.recovery;state.busy=true;state.operation=kind;state.done=state.total=state.elapsed_ms=0;state.retries=0;state.failed_phase=SAMPLE_IDLE;state.phase=kind==6?SAMPLE_PLAYING:kind==2||kind==7||kind==8||kind==9?SAMPLE_LOADING:SAMPLE_QUERY;if(kind==3||kind==4){state.completed=state.verified=false;state.full_readback=full_readback;state.write_ms=state.verify_ms=0;}strcpy(state.message,"Working...");state.revision++;ok=xQueueSend(requests,&r,0)==pdTRUE;if(!ok)state.busy=false;}
    unlock();return ok;
}
bool fm1_samples_scan(void){return submit(1,0,NULL,false);}
bool fm1_samples_prepare(const char *name){return fm1_sample_name(name)&&submit(2,0,name,false);}
bool fm1_samples_prepare_restore(const char *name){return fm1_sample_name(name)&&submit(8,0,name,false);}
bool fm1_samples_prepare_recent_restore(uint32_t expected_connection){return submit(9,expected_connection,NULL,false);}
bool fm1_samples_preview(const char *name){return fm1_sample_name(name)&&submit(7,0,name,false);}
bool fm1_samples_confirm(uint32_t token){return fm1_samples_confirm_with_readback(token,true);}
bool fm1_samples_confirm_with_readback(uint32_t token,bool full_readback){return submit(3,token,NULL,full_readback);}
bool fm1_samples_cancel(uint32_t token){return submit(5,token,NULL,false);}
bool fm1_samples_backup(void){return submit(4,0,NULL,true);}
bool fm1_samples_play(unsigned slot){return slot<8&&submit(6,slot,NULL,false);}
void fm1_samples_start(void)
{
    guard=xSemaphoreCreateMutex();requests=xQueueCreate(1,sizeof(request_t));if(!guard||!requests)abort();
    strcpy(state.message,"Tap RESCAN to read /fm1/samples/ on SD.");token=esp_timer_get_time()&16383;
    if(xTaskCreate(worker,"fm1_samples",12288,NULL,3,NULL)!=pdPASS)abort();
}
