#include "fm1_update.h"
#include "fm1_storage.h"
#include "fm1_package.h"
#include "fm1_host.h"
#include "fm1_ble.h"
#include "fm1_operation.h"
#include "esp_timer.h"
#include "freertos/FreeRTOS.h"
#include "freertos/semphr.h"
#include "freertos/queue.h"
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

static SemaphoreHandle_t mutex;
static QueueHandle_t actions;
static TaskHandle_t task;
static fm1_update_state_t state;
typedef struct {unsigned kind;uint32_t token,connection;char name[64],source[32];} action_t;
static uint8_t *image;
static size_t image_size;
static uint8_t *bridge_image;
static size_t bridge_size;
static const char *stock_name="M-VAVE-FM-1-V15.fwsc";
static bool confirm_queued;
static bool busy(void){return state.stage>=UPDATE_PREPARING&&state.stage<=UPDATE_RUNNING;}
void fm1_update_snapshot(fm1_update_state_t *out)
{
    if(!mutex){memset(out,0,sizeof(*out));return;}
    xSemaphoreTake(mutex,portMAX_DELAY);*out=state;xSemaphoreGive(mutex);
}
static void set_stage(fm1_update_stage_t stage,const char *message)
{
    xSemaphoreTake(mutex,portMAX_DELAY);state.stage=stage;state.revision++;
    snprintf(state.message,sizeof(state.message),"%s",message);xSemaphoreGive(mutex);
}
static void release_image(void){free(image);image=NULL;image_size=0;free(bridge_image);bridge_image=NULL;bridge_size=0;fm1_host_update_lock(false);fm1_ble_release_update();fm1_operation_release(FM1_OPERATION_UPDATE);}
static void error(const char *message){release_image();set_stage(UPDATE_ERROR,message);}
static bool request(const char *name,bool automatic)
{
    if(!mutex||!fm1_package_name(name))return false;
    fm1_state_t device;fm1_host_snapshot(&device);
    if(!device.connected||device.updating||!fm1_ota_identity_valid(device.identity))return false;
    action_t action={.kind=automatic?4:1,.connection=device.connection_id};
    snprintf(action.name,sizeof(action.name),"%s",name);snprintf(action.source,sizeof(action.source),"%s",device.identity);
    xSemaphoreTake(mutex,portMAX_DELAY);
    bool ok=!busy()&&fm1_operation_acquire(FM1_OPERATION_UPDATE);
    if(ok&&!fm1_ble_reserve_update()){
        fm1_operation_release(FM1_OPERATION_UPDATE);ok=false;
    }
    if(ok) {
        uint32_t token=state.token+1,revision=state.revision+1;
        state=(fm1_update_state_t){.stage=UPDATE_PREPARING,.token=token,.revision=revision,
            .source_connection=device.connection_id,.automatic=automatic};
        action.token=token;strcpy(state.source,device.identity);
        strcpy(state.name,name);strcpy(state.message,"Checking firmware and saving current voice...");confirm_queued=false;
        fm1_host_update_lock(true);ok=xQueueSend(actions,&action,0)==pdTRUE;
        if(!ok){state.stage=UPDATE_ERROR;fm1_host_update_lock(false);fm1_ble_release_update();fm1_operation_release(FM1_OPERATION_UPDATE);}
    }
    xSemaphoreGive(mutex);return ok;
}
bool fm1_update_prepare(const char *name){return request(name,false);}
bool fm1_update_install(const char *name){return request(name,true);}
bool fm1_update_confirm(uint32_t token)
{
    if(!mutex)return false;
    action_t action={.kind=2,.token=token};
    fm1_state_t device;fm1_host_snapshot(&device);
    xSemaphoreTake(mutex,portMAX_DELAY);
    bool ok=state.stage==UPDATE_READY&&state.token==token&&!confirm_queued&&device.connected&&
        device.connection_id==state.source_connection&&!strcmp(device.identity,state.source);
    if(ok){ok=xQueueSend(actions,&action,0)==pdTRUE;if(ok)confirm_queued=true;}
    xSemaphoreGive(mutex);return ok;
}
bool fm1_update_cancel(uint32_t token)
{
    if(!mutex)return false;
    action_t action={.kind=3,.token=token};
    xSemaphoreTake(mutex,portMAX_DELAY);
    bool ok=state.stage==UPDATE_READY&&state.token==token&&!confirm_queued;
    if(ok){ok=xQueueSend(actions,&action,0)==pdTRUE;if(ok)confirm_queued=true;}
    xSemaphoreGive(mutex);return ok;
}
bool fm1_update_dismiss(void)
{
    xSemaphoreTake(mutex,portMAX_DELAY);bool ok=!busy();
    if(ok){state.stage=UPDATE_IDLE;state.revision++;}
    xSemaphoreGive(mutex);return ok;
}
static bool prepare(const action_t *action)
{
    fm1_state_t device;fm1_host_snapshot(&device);
    if(!device.connected||device.connection_id!=action->connection||strcmp(device.identity,action->source)){
        error("USB connection changed; select the firmware and try again");return false;
    }
    char backup[64]="";
    if(device.can_edit&&!fm1_storage_backup_now(backup)){error("Current voice backup failed; refresh voice and retry");return false;}
    fm1_package_t meta;
    if(!fm1_storage_load_package(action->name,&image,&image_size,&meta)){error("Could not load complete SD firmware file");return false;}
    const fm1_release_t *r=fm1_release_find(meta.sha256,image_size,meta.identity);
    const char *blocked=fm1_release_block(r,device.identity);
    if(blocked){error(blocked);return false;}
    bool bridge=fm1_release_needs_bridge(r,device.identity);
    if(bridge) {
        fm1_package_t stock;
        bool ok=fm1_storage_load_package(stock_name,&bridge_image,&bridge_size,&stock);
        const fm1_release_t *known=ok?fm1_release_find(stock.sha256,bridge_size,stock.identity):NULL;
        if(!known||known->custom){error("Keep verified M-VAVE-FM-1-V15.fwsc on SD first");return false;}
        bridge_size=fm1_package_logical(bridge_image,bridge_size);
        if(!bridge_size){error("Invalid V15 recovery image");return false;}
    }
    image_size=fm1_package_logical(image,image_size);
    if(!image_size){error("Invalid target firmware image");return false;}
    fm1_state_t latest;fm1_host_snapshot(&latest);
    if(!latest.connected||latest.connection_id!=device.connection_id||strcmp(latest.identity,device.identity)){
        error("USB connection changed during preparation; try again");return false;
    }
    xSemaphoreTake(mutex,portMAX_DELAY);
    strcpy(state.source,device.identity);strcpy(state.target,r->identity);strcpy(state.title,r->title);
    state.source_connection=device.connection_id;
    strcpy(state.sha256,meta.sha256);strcpy(state.backup,backup);state.custom=r->custom;
    state.steps=bridge?2:1;state.step=1;strcpy(state.active_target,bridge?"FM-1_015":r->identity);xSemaphoreGive(mutex);
    if(action->kind==1)set_stage(UPDATE_READY,bridge?"Ready: V15 bridge, then selected firmware. Confirm once for both steps.":"Ready. Confirm on Tab5 to replace FM-1 firmware.");
    return true;
}
static bool install_step(const uint8_t *bytes,size_t size,const char *source,const char *target,uint32_t expected_connection,unsigned step,fm1_ota_t *result,uint32_t *verified_connection)
{
    fm1_update_state_t base;fm1_update_snapshot(&base);
    xSemaphoreTake(mutex,portMAX_DELAY);state.step=step;state.phase=OTA_HANDSHAKE;
    strcpy(state.active_target,target);state.revision++;xSemaphoreGive(mutex);
    if(!fm1_host_ota_begin(bytes,size,source,target,expected_connection)){snprintf(result->error,sizeof(result->error),"USB connection changed or updater busy; prepare again");return false;}
    int64_t queued=esp_timer_get_time();fm1_ota_t ota;
    for(;;) {
        vTaskDelay(pdMS_TO_TICKS(100));fm1_host_ota_snapshot(&ota,verified_connection);
        if(ota.phase==OTA_IDLE) {
            // Only the USB host task owns the OTA state machine and image reader.
            if(esp_timer_get_time()-queued>10000000) {
                // Keep the buffer alive while a queued request could still consume it.
                set_stage(UPDATE_RUNNING,"Waiting for USB host; keep Tab5 powered");
            }
            continue;
        }
        xSemaphoreTake(mutex,portMAX_DELAY);
        state.requests=base.requests+ota.requests;state.bytes=base.bytes+ota.bytes;
        state.elapsed_ms=base.elapsed_ms+ota.elapsed_ms;state.check_ms=base.check_ms+ota.check_ms;state.write_ms=base.write_ms+ota.write_ms;
        state.progress=((step-1)*100+ota.progress)/base.steps;state.phase=ota.phase;state.fast_transfer=ota.fast_transfer;
        const char *message=ota.phase==OTA_ERROR?ota.error:fm1_ota_phase_name(ota.phase);
        if(strcmp(state.message,message)){snprintf(state.message,sizeof(state.message),"%s",message);state.revision++;}
        xSemaphoreGive(mutex);
        if(ota.phase==OTA_DONE||ota.phase==OTA_ERROR)break;
    }
    *result=ota;return ota.phase==OTA_DONE;
}
static void install(void)
{
    fm1_update_state_t initial;fm1_update_snapshot(&initial);
    fm1_state_t device;fm1_host_snapshot(&device);
    if(!device.connected||device.connection_id!=initial.source_connection||strcmp(device.identity,initial.source)){
        error("USB connection changed before installation; try again");return;
    }
    bool recorded=fm1_storage_update_record(initial.name,"Install started",initial.steps==2?"V15 bridge then target":initial.target);
    set_stage(UPDATE_RUNNING,"Checking connected FM-1 identity...");
    fm1_ota_t ota={0};bool ok=true;uint32_t verified_connection=0;
    if(initial.steps==2){
        ok=install_step(bridge_image,bridge_size,initial.source,"FM-1_015",initial.source_connection,1,&ota,&verified_connection);
        if(ok&&!fm1_storage_update_record(stock_name,"Bridge verified",initial.target))recorded=false;
    }
    // Keep both immutable snapshots and USB/BLE ownership until the final
    // identity is verified. A bridge failure never starts the selected image.
    if(ok)ok=install_step(image,image_size,initial.steps==2?"FM-1_015":initial.source,initial.target,
        initial.steps==2?verified_connection:initial.source_connection,initial.steps,&ota,&verified_connection);
    if(!fm1_storage_update_record(initial.name,ok?"Installed":"Incomplete",ok?initial.target:ota.error))recorded=false;
    release_image();
    set_stage(ok?UPDATE_DONE:UPDATE_ERROR,ok?(recorded?"Installed. New firmware identity verified.":"Installed and verified; SD result log failed."):ota.error);
}
static void worker(void *arg)
{
    (void)arg;action_t action;
    for(;;) {
        if(xQueueReceive(actions,&action,portMAX_DELAY)!=pdTRUE)continue;
        if(action.kind==1||action.kind==4){
            fm1_update_state_t current;fm1_update_snapshot(&current);
            if(current.stage!=UPDATE_PREPARING||current.token!=action.token)continue;
            if(prepare(&action)&&action.kind==4)install();
        }
        else {
            fm1_update_state_t current;fm1_update_snapshot(&current);
            if(current.stage!=UPDATE_READY||current.token!=action.token)continue;
            if(action.kind==2)install();
            else {release_image();set_stage(UPDATE_IDLE,"Installation cancelled; FM-1 was not flashed.");}
        }
    }
}
unsigned fm1_update_stack_min(void){return task?uxTaskGetStackHighWaterMark(task):0;}
void fm1_update_start(void)
{
    mutex=xSemaphoreCreateMutex();actions=xQueueCreate(2,sizeof(action_t));if(!mutex||!actions)abort();
    if(xTaskCreate(worker,"fm1_update",16384,NULL,3,&task)!=pdPASS)abort();
}
