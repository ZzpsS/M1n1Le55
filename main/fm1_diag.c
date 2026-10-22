#include "fm1_host.h"
#include "m1_version.h"
#include "fm1_console.h"
#include "fm1_adapter.h"
#include "fm1_object.h"
#include "fm1_ble.h"
#include "fm1_storage.h"
#include "fm1_samples.h"
#include "fm1_package.h"
#include "fm1_update.h"
#include "esp_system.h"
#include "bsp/esp-bsp.h"
#include "driver/usb_serial_jtag.h"
#include "driver/usb_serial_jtag_vfs.h"
#include "esp_timer.h"
#include "esp_heap_caps.h"
#include "cJSON.h"
#include "mbedtls/base64.h"
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

bool fm1_ui_page(unsigned view,unsigned op);
bool fm1_ui_extension_view(unsigned page);
bool fm1_ui_edit(unsigned parameter,unsigned value);
unsigned fm1_ui_view(void);
unsigned fm1_ui_operator(void);
bool fm1_manager_section(unsigned view);
bool fm1_manager_select(unsigned index);

static void sample_report(void)
{
    static fm1_samples_state_t s;fm1_samples_snapshot(&s);cJSON *j=cJSON_CreateObject();
    cJSON_AddBoolToObject(j,"busy",s.busy);cJSON_AddBoolToObject(j,"prepared",s.prepared);cJSON_AddBoolToObject(j,"verified",s.verified);
    cJSON_AddBoolToObject(j,"completed",s.completed);cJSON_AddBoolToObject(j,"full_readback",s.full_readback);
    cJSON_AddBoolToObject(j,"result_completed",s.result_completed);cJSON_AddBoolToObject(j,"result_verified",s.result_verified);
    cJSON_AddNumberToObject(j,"phase",s.phase);cJSON_AddNumberToObject(j,"failed_phase",s.failed_phase);
    cJSON_AddBoolToObject(j,"confirmation_valid",s.confirmation_valid);cJSON_AddBoolToObject(j,"device_known",s.device_known);
    cJSON_AddBoolToObject(j,"device_verified",s.device_verified);
    cJSON_AddStringToObject(j,"device_name",s.device_name);cJSON_AddStringToObject(j,"result",s.result);cJSON_AddStringToObject(j,"result_backup",s.result_backup);
    cJSON_AddNumberToObject(j,"omitted",s.omitted);cJSON_AddNumberToObject(j,"connection_id",s.connection_id);
    cJSON_AddNumberToObject(j,"token",s.token);cJSON_AddNumberToObject(j,"bytes",s.bytes);cJSON_AddNumberToObject(j,"done",s.done);cJSON_AddNumberToObject(j,"total",s.total);
    cJSON_AddNumberToObject(j,"elapsed_ms",s.elapsed_ms);cJSON_AddNumberToObject(j,"write_ms",s.write_ms);cJSON_AddNumberToObject(j,"verify_ms",s.verify_ms);cJSON_AddNumberToObject(j,"retries",s.retries);
    cJSON_AddStringToObject(j,"message",s.message);cJSON_AddStringToObject(j,"name",s.name);cJSON_AddStringToObject(j,"backup",s.backup);
    cJSON *files=cJSON_AddArrayToObject(j,"files");for(unsigned i=0;i<s.count;i++)cJSON_AddItemToArray(files,cJSON_CreateString(s.files[i]));
    cJSON *slots=cJSON_AddArrayToObject(j,"slots");for(unsigned i=0;i<8;i++)cJSON_AddItemToArray(slots,cJSON_CreateString(s.slots[i]));
    char *json=cJSON_PrintUnformatted(j);if(json){printf("@FM1SAMPLES %s\n",json);free(json);}cJSON_Delete(j);
}

static void adapter_report(void)
{
    static fm1_adapter_state_t s;fm1_adapter_snapshot(&s);cJSON *j=cJSON_CreateObject();
    cJSON_AddStringToObject(j,"identity",s.identity);cJSON_AddStringToObject(j,"profile",fm1_adapter_at(s.profile)->name);
    cJSON_AddStringToObject(j,"gap",fm1_adapter_at(s.profile)->gap);cJSON_AddStringToObject(j,"message",s.message);
    cJSON_AddBoolToObject(j,"connected",s.connected);cJSON_AddBoolToObject(j,"busy",s.busy);cJSON_AddBoolToObject(j,"ready",s.ready);
    cJSON_AddNumberToObject(j,"connection_id",s.connection_id);cJSON_AddNumberToObject(j,"page_revision",s.page_revision);
    cJSON_AddBoolToObject(j,"native_view",s.native_view);cJSON_AddNumberToObject(j,"scope",s.scope);
    cJSON_AddNumberToObject(j,"selected_track",s.selected);cJSON_AddNumberToObject(j,"engine",s.engine);
    cJSON_AddNumberToObject(j,"parameters",s.parameters);cJSON_AddNumberToObject(j,"globals",s.globals);
    cJSON_AddBoolToObject(j,"file_prepared",s.file_prepared);cJSON_AddNumberToObject(j,"file_token",s.file_token);
    cJSON_AddNumberToObject(j,"file_progress",s.file_progress);cJSON_AddNumberToObject(j,"file_total",s.file_total);
    cJSON_AddStringToObject(j,"file_backup",s.file_backup);cJSON_AddStringToObject(j,"file_name",s.file_name);
    cJSON *rows=cJSON_AddArrayToObject(j,"rows");for(unsigned i=0;i<s.count;i++){
      cJSON *r=cJSON_CreateObject();cJSON_AddItemToArray(rows,r);cJSON_AddStringToObject(r,"name",s.rows[i].name);
      cJSON_AddNumberToObject(r,"id",s.rows[i].id);cJSON_AddNumberToObject(r,"value",s.rows[i].value);cJSON_AddBoolToObject(r,"known",s.rows[i].known);
      cJSON_AddNumberToObject(r,"scope",s.rows[i].scope);cJSON_AddNumberToObject(r,"index",s.rows[i].index);
    }
    char *json=cJSON_PrintUnformatted(j);if(json){printf("@FM1ADAPTER %s\n",json);free(json);}cJSON_Delete(j);
}

static void update_report(void)
{
    fm1_update_state_t s;fm1_update_snapshot(&s);cJSON *j=cJSON_CreateObject();
    cJSON_AddNumberToObject(j,"stage",s.stage);cJSON_AddNumberToObject(j,"token",s.token);
    cJSON_AddNumberToObject(j,"source_connection",s.source_connection);
    cJSON_AddStringToObject(j,"package",s.name);cJSON_AddStringToObject(j,"source",s.source);cJSON_AddStringToObject(j,"target",s.target);
    cJSON_AddNumberToObject(j,"step",s.step);cJSON_AddNumberToObject(j,"steps",s.steps);
    cJSON_AddStringToObject(j,"active_target",s.active_target);
    cJSON_AddStringToObject(j,"title",s.title);cJSON_AddStringToObject(j,"sha256",s.sha256);cJSON_AddStringToObject(j,"backup",s.backup);
    cJSON_AddStringToObject(j,"message",s.message);cJSON_AddNumberToObject(j,"requests",s.requests);cJSON_AddNumberToObject(j,"bytes",s.bytes);
    cJSON_AddNumberToObject(j,"elapsed_ms",s.elapsed_ms);cJSON_AddNumberToObject(j,"check_ms",s.check_ms);
    cJSON_AddNumberToObject(j,"write_ms",s.write_ms);cJSON_AddNumberToObject(j,"wait_ms",s.elapsed_ms-s.check_ms-s.write_ms);
    cJSON_AddNumberToObject(j,"progress_percent",s.progress);cJSON_AddBoolToObject(j,"progress_estimated",s.stage!=UPDATE_DONE);
    cJSON_AddNumberToObject(j,"phase",s.phase);cJSON_AddBoolToObject(j,"fast_transfer",s.fast_transfer);
    char *json=cJSON_PrintUnformatted(j);if(json){printf("@FM1UPDATE %s\n",json);free(json);}cJSON_Delete(j);
}

static void storage_report(void)
{
    fm1_storage_state_t *s=malloc(sizeof(*s));if(!s)return;fm1_storage_snapshot(s);
    cJSON *j=cJSON_CreateObject();cJSON_AddBoolToObject(j,"mounted",s->mounted);
    cJSON_AddBoolToObject(j,"busy",s->busy);cJSON_AddNumberToObject(j,"capacity_mb",s->capacity_mb);
    cJSON_AddStringToObject(j,"message",s->message);cJSON_AddNumberToObject(j,"backups",s->backups);
    cJSON_AddStringToObject(j,"last_backup",s->last_backup);cJSON_AddNumberToObject(j,"omitted",s->omitted);
    cJSON_AddBoolToObject(j,"installation_enabled",true);
    cJSON_AddNumberToObject(j,"upload_chunk_bytes",FM1_UPLOAD_CHUNK);
    fm1_state_t device;fm1_host_snapshot(&device);
    cJSON *files=cJSON_AddArrayToObject(j,"files");
    for(unsigned i=0;i<s->count;i++) {
        fm1_package_t *p=&s->files[i];cJSON *f=cJSON_CreateObject();cJSON_AddItemToArray(files,f);
        cJSON_AddStringToObject(f,"name",p->name);cJSON_AddStringToObject(f,"identity",p->identity);
        cJSON_AddStringToObject(f,"sha256",p->sha256);cJSON_AddStringToObject(f,"check",p->check);
        cJSON_AddNumberToObject(f,"size",p->size);cJSON_AddBoolToObject(f,"model_ok",p->model_ok);
        cJSON_AddBoolToObject(f,"hash_match",p->hash_match);
        const fm1_release_t *r=fm1_release_find(p->sha256,p->size,p->identity);
        const char *block=fm1_release_block(r,device.identity);
        cJSON_AddStringToObject(f,"title",r?r->title:p->name);
        cJSON_AddBoolToObject(f,"supported",r!=NULL);
        cJSON_AddBoolToObject(f,"via_v15",!block&&fm1_release_needs_bridge(r,device.identity));
        cJSON_AddBoolToObject(f,"can_install",r&&!block&&p->hash_match&&device.connected&&!device.updating&&!s->busy);
        cJSON_AddStringToObject(f,"install_block",block?block:"");
    }
    cJSON *log=cJSON_AddArrayToObject(j,"activity");
    for(unsigned i=0;i<6;i++)if(s->history[i][0])cJSON_AddItemToArray(log,cJSON_CreateString(s->history[i]));
    char *json=cJSON_PrintUnformatted(j);if(json){printf("@FM1SD %s\n",json);free(json);}cJSON_Delete(j);free(s);
}

static void ble_report(void)
{
    fm1_ble_state_t s;fm1_ble_snapshot(&s);cJSON *j=cJSON_CreateObject();if(!j)return;
    cJSON_AddNumberToObject(j,"phase",s.phase);cJSON_AddNumberToObject(j,"scan_id",s.scan_id);
    cJSON_AddBoolToObject(j,"pending",s.pending);cJSON_AddBoolToObject(j,"testing",s.testing);
    cJSON_AddStringToObject(j,"message",s.message);cJSON_AddStringToObject(j,"peer",s.peer);
    cJSON_AddStringToObject(j,"coprocessor",s.coprocessor);cJSON_AddStringToObject(j,"last_rx_hex",s.last_rx);
    cJSON_AddNumberToObject(j,"rx_notifications",s.rx_packets);cJSON_AddNumberToObject(j,"rx_bytes",s.rx_bytes);
    cJSON_AddNumberToObject(j,"rx_invalid_headers",s.rx_invalid);cJSON_AddNumberToObject(j,"tx_queued",s.tx_packets);
    cJSON *devices=cJSON_AddArrayToObject(j,"devices");
    for(unsigned i=0;i<s.count;i++) {
        cJSON *d=cJSON_CreateObject();cJSON_AddItemToArray(devices,d);
        cJSON_AddStringToObject(d,"name",s.devices[i].name);cJSON_AddStringToObject(d,"address",s.devices[i].address);
        cJSON_AddNumberToObject(d,"rssi",s.devices[i].rssi);cJSON_AddBoolToObject(d,"advertises_midi",s.devices[i].midi);
    }
    char *json=cJSON_PrintUnformatted(j);if(json){printf("@FM1BLE %s\n",json);free(json);}cJSON_Delete(j);
}
static void report(void)
{
    fm1_state_t s;fm1_host_snapshot(&s);
    cJSON *j=cJSON_CreateObject();if(!j) return;
    cJSON_AddNumberToObject(j,"protocol",4);
    cJSON_AddStringToObject(j,"app_name","M1n1Le55");
    cJSON_AddStringToObject(j,"app_version",M1_VERSION);
    cJSON_AddStringToObject(j,"app_revision",M1_REVISION);
    cJSON_AddNumberToObject(j,"console_page",fm1_console_mode());
    cJSON_AddNumberToObject(j,"reset_reason",esp_reset_reason());
    cJSON_AddNumberToObject(j,"diag_stack_min",uxTaskGetStackHighWaterMark(NULL));
    cJSON_AddNumberToObject(j,"sd_stack_min",fm1_storage_stack_min());
    cJSON_AddNumberToObject(j,"update_stack_min",fm1_update_stack_min());
    cJSON_AddBoolToObject(j,"updating",s.updating);
    cJSON_AddBoolToObject(j,"connected",s.connected);
    cJSON_AddBoolToObject(j,"valid_voice",s.valid_voice);
    cJSON_AddNumberToObject(j,"connection_id",s.connection_id);
    cJSON_AddBoolToObject(j,"can_edit",s.can_edit);
    cJSON_AddStringToObject(j,"identity",s.identity);
    cJSON_AddStringToObject(j,"status",s.status);
    cJSON_AddNumberToObject(j,"uptime_ms",esp_timer_get_time()/1000);
    cJSON_AddNumberToObject(j,"voice_age_ms",s.valid_voice?(esp_timer_get_time()-s.last_read_us)/1000:-1);
    cJSON_AddNumberToObject(j,"free_heap",esp_get_free_heap_size());
    cJSON_AddNumberToObject(j,"rx_packets",s.rx_packets);
    cJSON_AddNumberToObject(j,"rx_frames",s.rx_frames);
    cJSON_AddNumberToObject(j,"bad_frames",s.bad_frames);
    cJSON_AddNumberToObject(j,"tx_frames",s.tx_frames);
    cJSON_AddNumberToObject(j,"last_frame_size",s.last_frame_size);
    cJSON_AddNumberToObject(j,"last_command",s.last_command);
    cJSON_AddNumberToObject(j,"writes_verified",s.writes_verified);
    cJSON_AddBoolToObject(j,"clock_enabled",s.clock_enabled);cJSON_AddNumberToObject(j,"clock_bpm",s.clock_bpm);
    cJSON_AddNumberToObject(j,"clock_pulses",s.clock_pulses);cJSON_AddNumberToObject(j,"clock_missed",s.clock_missed);
    cJSON_AddNumberToObject(j,"writes_failed",s.writes_failed);
    cJSON_AddNumberToObject(j,"fx_channel",s.fx_channel+1);
    cJSON_AddNumberToObject(j,"fx_scale",s.fx_scale);
    cJSON_AddNumberToObject(j,"fx_known",s.fx_known);
    cJSON *fx=cJSON_AddArrayToObject(j,"fx_values");
    for(unsigned i=0;i<24;i++)cJSON_AddItemToArray(fx,cJSON_CreateNumber(s.fx_values[i]));
    char hex[311];for(unsigned i=0;i<155;i++) snprintf(hex+i*2,3,"%02x",s.voice[i]);
    cJSON_AddStringToObject(j,"voice_hex",s.valid_voice?hex:"");
    bsp_display_lock(0);
    cJSON_AddNumberToObject(j,"view",fm1_ui_view());cJSON_AddNumberToObject(j,"operator",fm1_ui_operator()+1);
    cJSON_AddNumberToObject(j,"width",lv_display_get_horizontal_resolution(NULL));
    cJSON_AddNumberToObject(j,"height",lv_display_get_vertical_resolution(NULL));
    bsp_display_unlock();
    char *json=cJSON_PrintUnformatted(j);
    if(json){printf("@FM1DIAG %s\n",json);free(json);}cJSON_Delete(j);fflush(stdout);
}

static void screenshot(void)
{
    bsp_display_lock(0);
    uint32_t width=lv_display_get_horizontal_resolution(NULL),height=lv_display_get_vertical_resolution(NULL);
    uint32_t stride=lv_draw_buf_width_to_stride(width,LV_COLOR_FORMAT_RGB565);
    size_t capacity=(size_t)stride*height;
    uint8_t *pixels=heap_caps_aligned_alloc(64,capacity,MALLOC_CAP_SPIRAM|MALLOC_CAP_8BIT);
    lv_draw_buf_t storage;lv_draw_buf_t *buf=&storage;
    bool ok=pixels && lv_draw_buf_init(buf,width,height,LV_COLOR_FORMAT_RGB565,stride,pixels,capacity)==LV_RESULT_OK;
    if(ok) ok=lv_snapshot_take_to_draw_buf(lv_screen_active(),LV_COLOR_FORMAT_RGB565,buf)==LV_RESULT_OK;
    bsp_display_unlock();
    if(!ok){free(pixels);puts("@FM1SCREEN ERROR allocation_or_render");return;}
    size_t total=buf->header.stride*buf->header.h;
    printf("@FM1SCREEN BEGIN %u %u %u %u\n",(unsigned)buf->header.w,(unsigned)buf->header.h,
           (unsigned)buf->header.stride,(unsigned)total);
    for(size_t offset=0;offset<total;offset+=768) {
        size_t n=total-offset;if(n>768)n=768;
        unsigned char encoded[1025];size_t written;
        if(mbedtls_base64_encode(encoded,sizeof(encoded),&written,buf->data+offset,n)!=0) break;
        encoded[written]=0;
        printf("@FM1SCREEN DATA %u %s\n",(unsigned)offset,encoded);fflush(stdout);
        vTaskDelay(1);
    }
    puts("@FM1SCREEN END");fflush(stdout);
    free(pixels);
}

/* One bounded request/reply on BLE. No USB MIDI fallback, no implicit resend.
 * Caller owns retry policy and must reuse exactly the same token and payload. */
static void ble_query(const uint8_t *request,unsigned size)
{
    uint8_t response[192];uint32_t before,seq;
    (void)fm1_ble_sysex_last(response,sizeof(response),&before);
    if(!fm1_ble_sysex_send(request,size)){puts("@FM1BLEQUERY BUSY_OR_INVALID");return;}
    int64_t deadline=esp_timer_get_time()+3000000;
    while(esp_timer_get_time()<deadline) {
        unsigned n=fm1_ble_sysex_last(response,sizeof(response),&seq);
        if(seq!=before&&n>=12&&!memcmp(response,request,5)&&
           response[5]==(request[5]|64)&&response[6]==request[6]&&response[7]==request[7]) {
            char hex[385];for(unsigned i=0;i<n;i++)snprintf(hex+i*2,3,"%02x",response[i]);
            printf("@FM1BLEQUERY %s\n",hex);return;
        }
        fm1_ble_state_t state;fm1_ble_snapshot(&state);
        if(state.phase!=FM1_BLE_READY){puts("@FM1BLEQUERY DISCONNECTED");return;}
        fm1_ble_sysex_wait(50);
    }
    puts("@FM1BLEQUERY TIMEOUT");
}

static void diag_task(void *arg)
{
    (void)arg;
    // Single diagnostic task owns these buffers; keep large import lines off its stack.
    static char line[FM1_UPLOAD_CHUNK*2+32];
    static uint8_t upload_bytes[FM1_UPLOAD_CHUNK];
    size_t used=0;bool overflow=false;
    for(;;) {
        char incoming[256];
        int received=usb_serial_jtag_read_bytes(incoming,sizeof(incoming),pdMS_TO_TICKS(100));
        for(int i=0;i<received;i++) {
        char c=incoming[i];
        if(c=='\r')continue;
        if(c!='\n') {if(used<sizeof(line)-1)line[used++]=c;else overflow=true;continue;}
        line[used]=0;
        if(!overflow) {
            unsigned p,v;char extra;
            if(!strcmp(line,"STATUS")) report();
            else if(sscanf(line,"CLOCK %u %u %c",&p,&v,&extra)==2 && v<=1){fm1_state_t h;fm1_host_snapshot(&h);printf("@FM1ACK %d\n",fm1_host_clock(h.connection_id,p,v));}
            else if(!strcmp(line,"BLESTATUS"))ble_report();
            else if(!strcmp(line,"BLESCAN"))printf("@FM1ACK %d\n",fm1_ble_scan());
            else if(sscanf(line,"BLECONNECT %u %u %c",&p,&v,&extra)==2)printf("@FM1ACK %d\n",fm1_ble_connect(p,v));
            else if(!strcmp(line,"BLEDISCONNECT"))printf("@FM1ACK %d\n",fm1_ble_disconnect());
            else if(!strcmp(line,"BLETEST"))printf("@FM1ACK %d\n",fm1_ble_test_note());
            else if(sscanf(line,"BLEPLAY %u %u %c",&p,&v,&extra)==2&&p>=1&&p<=16&&v<128)
                printf("@FM1ACK %d\n",fm1_ble_play_note(p-1,v));
            else if(!strcmp(line,"BLEMTU"))printf("@FM1ACK %d\n",fm1_ble_link_check(true));
            else if(!strcmp(line,"BLEFAST"))printf("@FM1ACK %d\n",fm1_ble_fast_interval());
            else if(!strcmp(line,"BLECCCD"))printf("@FM1ACK %d\n",fm1_ble_link_check(false));
            else if(!strncmp(line,"BLESYSEX ",9)||!strncmp(line,"BLEQUERY ",9)) {
                uint8_t data[192];size_t chars=strlen(line+9);bool ok=chars>=20&&chars<=384&&!(chars&1);
                for(unsigned i=0;ok&&i<chars/2;i++) {
                    char hex[3]={line[9+i*2],line[10+i*2],0},*end;
                    if(!strchr("0123456789abcdefABCDEF",hex[0])||!strchr("0123456789abcdefABCDEF",hex[1])){ok=false;break;}
                    data[i]=strtoul(hex,&end,16);if(*end)ok=false;
                }
                if(!strncmp(line,"BLEQUERY ",9)) {
                    if(ok)ble_query(data,chars/2);else puts("@FM1BLEQUERY INVALID");
                } else printf("@FM1ACK %d\n",ok&&fm1_ble_sysex_send(data,chars/2));
            }
            else if(!strcmp(line,"BLERX")||!strcmp(line,"USBAPPRX")) {
                uint8_t data[192];char hex[385];uint32_t seq;bool bluetooth=!strcmp(line,"BLERX");
                unsigned n=bluetooth?fm1_ble_sysex_last(data,sizeof(data),&seq):fm1_host_app_last(data,sizeof(data),&seq);
                hex[0]=0;for(unsigned i=0;i<n;i++)snprintf(hex+i*2,3,"%02x",data[i]);
                printf("@FM1%s %lu %s\n",bluetooth?"BLERX":"USBAPPRX",(unsigned long)seq,hex);
            }
            else if(!strcmp(line,"FWSTATUS")) update_report();
            else if(!strncmp(line,"FWPREP ",7)) {
                char name[64];bool ok=sscanf(line,"FWPREP %63s %c",name,&extra)==1&&fm1_update_prepare(name);
                printf("@FM1ACK %d\n",ok);
            }
            else if(sscanf(line,"FWCONFIRM %u %c",&p,&extra)==1)printf("@FM1ACK %d\n",fm1_update_confirm(p));
            else if(sscanf(line,"FWCANCEL %u %c",&p,&extra)==1)printf("@FM1ACK %d\n",fm1_update_cancel(p));
            else if(!strcmp(line,"FWDISMISS"))printf("@FM1ACK %d\n",fm1_update_dismiss());
            else if(!strcmp(line,"SAMPLESTATUS"))sample_report();
            else if(!strcmp(line,"SAMPLERESCAN"))printf("@FM1ACK %d\n",fm1_samples_scan());
            else if(!strcmp(line,"SAMPLEBACKUP"))printf("@FM1ACK %d\n",fm1_samples_backup());
            else if(!strncmp(line,"SAMPLEPREVIEW ",14)) {
                char name[64];bool ok=sscanf(line,"SAMPLEPREVIEW %63s %c",name,&extra)==1&&fm1_samples_preview(name);printf("@FM1ACK %d\n",ok);
            }
            else if(!strncmp(line,"SAMPLEPREP ",11)) {
                char name[64];bool ok=sscanf(line,"SAMPLEPREP %63s %c",name,&extra)==1&&fm1_samples_prepare(name);printf("@FM1ACK %d\n",ok);
            }
            else if(sscanf(line,"SAMPLECONFIRM %u %c",&p,&extra)==1)printf("@FM1ACK %d\n",fm1_samples_confirm(p));
            else if(sscanf(line,"SAMPLECANCEL %u %c",&p,&extra)==1)printf("@FM1ACK %d\n",fm1_samples_cancel(p));
            else if(sscanf(line,"SAMPLEPLAY %u %c",&p,&extra)==1)printf("@FM1ACK %d\n",p>=1&&p<=8&&fm1_samples_play(p-1));
            else if(!strncmp(line,"SAMPLEPUT ",10)) {
                char name[64],sha[65];bool ok=sscanf(line,"SAMPLEPUT %63s %u %64s %c",name,&p,sha,&extra)==3;
                printf("@FM1ACK %d\n",ok&&fm1_storage_sample_upload_begin(name,p,sha));
            }
            else if(!strcmp(line,"ADAPTER"))adapter_report();
            else if(sscanf(line,"ADAPTERUI %u %c",&p,&extra)==1){bsp_display_lock(0);bool ok=fm1_ui_extension_view(p);bsp_display_unlock();printf("@FM1ACK %d\n",ok);}
            else if(sscanf(line,"CONSOLEUI %u %c",&p,&extra)==1){bsp_display_lock(0);bool ok=fm1_console_page(p);bsp_display_unlock();printf("@FM1ACK %d\n",ok);}
            else if(sscanf(line,"ADAPTEROBJ %u %c",&p,&extra)==1)printf("@FM1ACK %d\n",fm1_adapter_object_read(p));
            else if(sscanf(line,"ADAPTERREAD %u %c",&p,&extra)==1)printf("@FM1ACK %d\n",fm1_adapter_file_read(p));
            else if(sscanf(line,"ADAPTERVIEW %u %u %c",&p,&v,&extra)==2)printf("@FM1ACK %d\n",fm1_adapter_view(p,v,255));
            else if(!strncmp(line,"EXCHANGEPUT ",12)) {
                char name[64],sha[65];bool ok=sscanf(line,"EXCHANGEPUT %63s %u %64s %c",name,&p,sha,&extra)==3;
                printf("@FM1ACK %d\n",ok&&fm1_storage_exchange_upload_begin(name,p,sha));
            }
            else if(!strcmp(line,"SDSTATUS")) storage_report();
            else if(sscanf(line,"SDVIEW %u %c",&p,&extra)==1) {
                bsp_display_lock(0);bool ok=fm1_manager_section(p);bsp_display_unlock();
                printf("@FM1ACK %d\n",ok);
            }
            else if(sscanf(line,"SDSELECT %u %c",&p,&extra)==1) {
                bsp_display_lock(0);bool ok=fm1_manager_select(p);bsp_display_unlock();
                printf("@FM1ACK %d\n",ok);
            }
            else if(!strncmp(line,"SDGETBACKUP ",12)) {
                char name[64],json[1024];
                bool ok=sscanf(line,"SDGETBACKUP %63s %c",name,&extra)==1&&fm1_storage_read_backup(name,json,sizeof(json));
                cJSON *j=ok?cJSON_Parse(json):NULL;char *result=j?cJSON_PrintUnformatted(j):NULL;
                if(result){printf("@FM1BACKUP %s\n",result);free(result);}else puts("@FM1ACK 0");cJSON_Delete(j);
            }
            else if(!strcmp(line,"SDRESCAN")) printf("@FM1ACK %d\n",fm1_storage_request(false));
            else if(!strcmp(line,"SDBACKUP")) printf("@FM1ACK %d\n",fm1_storage_request(true));
            else if(!strcmp(line,"SDABORT")){fm1_storage_upload_abort();puts("@FM1ACK 1");}
            else if(!strcmp(line,"SDEND")) printf("@FM1ACK %d\n",fm1_storage_upload_end());
            else if(!strncmp(line,"SDPUT ",6)) {
                char name[64],sha[65];
                bool ok=sscanf(line,"SDPUT %63s %u %64s %c",name,&p,sha,&extra)==3;
                printf("@FM1ACK %d\n",ok&&fm1_storage_upload_begin(name,p,sha));
            }
            else if(!strncmp(line,"SDDATA ",7)) {
                int start=0;
                bool ok=sscanf(line,"SDDATA %u %n",&p,&start)==1&&start>0;
                size_t n=ok?fm1_hex_decode(line+start,upload_bytes,sizeof(upload_bytes)):0;
                printf("@FM1ACK %d\n",n&&fm1_storage_upload_data(p,upload_bytes,n));
            }
            else if(!strcmp(line,"SCREEN")) screenshot();
            else if(!strcmp(line,"READ")) printf("@FM1ACK %d\n",fm1_host_read());
            else if(sscanf(line,"SET %u %u %c",&p,&v,&extra)==2 && p<155 && v<128)
                printf("@FM1ACK %d\n",fm1_host_parameter(p,v));
            else if(sscanf(line,"UI %u %u %c",&p,&v,&extra)==2) {
                bsp_display_lock(0);bool ok=fm1_ui_page(p,v);bsp_display_unlock();printf("@FM1ACK %d\n",ok);
            }
            else if(sscanf(line,"UIEDIT %u %u %c",&p,&v,&extra)==2) {
                bsp_display_lock(0);bool ok=fm1_ui_edit(p,v);bsp_display_unlock();printf("@FM1ACK %d\n",ok);
            }
            else if(sscanf(line,"FX %u %u %c",&p,&v,&extra)==2 && p<24 && v<128)
                printf("@FM1ACK %d\n",fm1_host_effect(p,v));
            else if(sscanf(line,"FXCONFIG %u %u %c",&p,&v,&extra)==2 && p<16 && (v==10 || v==100))
                printf("@FM1ACK %d\n",fm1_host_fx_config(p,v));
            else puts("@FM1ACK 0");
        } else puts("@FM1ACK 0");
        used=0;overflow=false;fflush(stdout);
        }
    }
}

void fm1_diag_start(void)
{
    usb_serial_jtag_driver_config_t cfg={.tx_buffer_size=8192,.rx_buffer_size=4096};
    ESP_ERROR_CHECK(usb_serial_jtag_driver_install(&cfg));
    usb_serial_jtag_vfs_use_driver();
    if(xTaskCreate(diag_task,"fm1_diag",12288,NULL,3,NULL)!=pdPASS) abort();
}

