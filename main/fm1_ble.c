#include "fm1_ble.h"
#include "fm1_ble_midi.h"
#include "fm1_host.h"
#include "sdkconfig.h"
#ifndef CONFIG_BT_NIMBLE_GATT_SERVER
#error "IDF 5.4 NimBLE needs GATT server support to receive ATT notifications"
#endif
#include "bsp/esp-bsp.h"
#include "esp_hosted.h"
#include "esp_event.h"
#include "esp_timer.h"
#include "esp_log.h"
#include "esp_heap_caps.h"
#include "nvs_flash.h"
#include "nimble/nimble_port.h"
#include "nimble/nimble_port_freertos.h"
#include "host/ble_hs.h"
#include "host/util/util.h"
#include "freertos/FreeRTOS.h"
#include "freertos/queue.h"
#include "freertos/semphr.h"
#include <stdio.h>
#include <string.h>

static const ble_uuid128_t service_uuid=BLE_UUID128_INIT(0x00,0xc7,0xc4,0x4e,0xe3,0x6c,0x51,0xa7,0x33,0x4b,0xe8,0xed,0x5a,0x0e,0xb8,0x03);
static const ble_uuid128_t midi_uuid=BLE_UUID128_INIT(0xf3,0x6b,0x10,0x9d,0x66,0xf2,0xa9,0xa1,0x12,0x41,0x68,0x38,0xdb,0xe5,0x72,0x77);
static SemaphoreHandle_t guard;
static SemaphoreHandle_t sysex_signal;
static QueueHandle_t commands;
static fm1_ble_state_t state;
static ble_addr_t addresses[FM1_BLE_DEVICES];
static bool synced,started,update_reserved;
static TaskHandle_t session_owner;
static uint8_t own_type;
/* All GATT discovery fields belong to the NimBLE host task. Worker only takes a
 * connection/value snapshot under guard when sending a test message. */
static uint16_t connection=BLE_HS_CONN_HANDLE_NONE,service_start,service_end,value_handle,cccd,chr_end;
static uint8_t properties;
static unsigned security_attempts;
static int64_t discovery_deadline;
static fm1_ble_sysex_rx_t sysex_rx;
static uint8_t sysex_last[FM1_BLE_SYSEX_MAX];
static unsigned sysex_size;
static uint32_t sysex_sequence;
static bool link_check_pending;
typedef struct {unsigned kind;ble_addr_t address;char name[32];unsigned size;uint8_t data[FM1_BLE_SYSEX_MAX];} command_t;
static int gap_event(struct ble_gap_event *event,void *arg);
static void subscribe(void);

static void message(fm1_ble_phase_t phase,const char *s)
{
    xSemaphoreTake(guard,portMAX_DELAY);
    if(phase==FM1_BLE_READY&&state.phase!=FM1_BLE_READY)state.connection_id++;
    state.phase=phase;
    snprintf(state.message,sizeof(state.message),"%s",s);state.revision++;
    xSemaphoreGive(guard);ESP_LOGI("fm1_ble","%s",s);
}
static void failed(const char *operation,int rc)
{
    char s[128];snprintf(s,sizeof(s),"%s failed (%d). Disconnect / rescan to retry.",operation,rc);
    message(FM1_BLE_ERROR,s);
    xSemaphoreTake(guard,portMAX_DELAY);uint16_t handle=connection;discovery_deadline=0;xSemaphoreGive(guard);
    if(handle!=BLE_HS_CONN_HANDLE_NONE)ble_gap_terminate(handle,BLE_ERR_REM_USER_CONN_TERM);
    else if(synced)ble_gap_conn_cancel();
}
void fm1_ble_snapshot(fm1_ble_state_t *out)
{
    if(!guard){memset(out,0,sizeof(*out));strcpy(out->message,"Bluetooth not started");return;}
    xSemaphoreTake(guard,portMAX_DELAY);*out=state;xSemaphoreGive(guard);
}
static bool submit(unsigned kind,unsigned index,uint32_t scan)
{
    if(!guard||!commands)return false;
    command_t c={.kind=kind};xSemaphoreTake(guard,portMAX_DELAY);
    bool ok=(!session_owner||session_owner==xTaskGetCurrentTaskHandle())&&!state.pending&&!state.testing&&!update_reserved&&(!link_check_pending||kind==3);
    if(kind==1)ok=ok&&connection==BLE_HS_CONN_HANDLE_NONE&&
        (state.phase==FM1_BLE_OFF||state.phase==FM1_BLE_IDLE||state.phase==FM1_BLE_ERROR);
    if(kind==2) {
        ok=ok&&synced&&scan==state.scan_id&&index<state.count&&
            (state.phase==FM1_BLE_SCANNING||state.phase==FM1_BLE_IDLE);
        if(ok){c.address=addresses[index];strcpy(c.name,state.devices[index].name);}
    }
    if(kind==3)ok=ok&&synced&&state.phase!=FM1_BLE_STARTING;
    if(kind==4){ok=ok&&state.phase==FM1_BLE_READY&&index<128&&scan<16;c.data[0]=index;c.data[1]=scan;}
    if(kind==6||kind==7||kind==8)ok=ok&&state.phase==FM1_BLE_READY;
    if(ok){state.pending=true;state.revision++;ok=xQueueSend(commands,&c,0)==pdTRUE;if(!ok)state.pending=false;}
    xSemaphoreGive(guard);return ok;
}
bool fm1_ble_scan(void){return submit(1,0,0);}
bool fm1_ble_connect(unsigned i,uint32_t scan){return submit(2,i,scan);}
bool fm1_ble_disconnect(void){return submit(3,0,0);}
bool fm1_ble_test_note(void){return submit(4,60,0);}
bool fm1_ble_play_note(unsigned channel,unsigned note){return submit(4,note,channel);}
bool fm1_ble_link_check(bool exchange_mtu){return submit(exchange_mtu?6:7,0,0);}
bool fm1_ble_fast_interval(void){return submit(8,0,0);}
void fm1_ble_sysex_wait(unsigned ms)
{if(sysex_signal)xSemaphoreTake(sysex_signal,pdMS_TO_TICKS(ms));}
bool fm1_ble_sysex_send(const uint8_t *data,unsigned size)
{
    if(!guard||!commands||!data||size<10||size>FM1_BLE_SYSEX_MAX||
       memcmp(data,(uint8_t[]){0xf0,0x7d,'F','M',2},5)||data[size-1]!=0xf7||
       (data[5]!=0&&(data[5]<11||data[5]>16)&&(data[5]<32||data[5]>35)))return false;
    unsigned sum=0;for(unsigned i=1;i+1<size;i++){if(data[i]&128)return false;sum+=data[i];}
    if(sum&127)return false;
    command_t c={.kind=5,.size=size};memcpy(c.data,data,size);
    xSemaphoreTake(guard,portMAX_DELAY);
    bool ok=(!session_owner||session_owner==xTaskGetCurrentTaskHandle())&&!state.pending&&!state.testing&&!update_reserved&&!link_check_pending&&state.phase==FM1_BLE_READY;
    if(ok){state.pending=true;state.revision++;ok=xQueueSend(commands,&c,0)==pdTRUE;if(!ok)state.pending=false;}
    xSemaphoreGive(guard);return ok;
}
unsigned fm1_ble_sysex_last(uint8_t *data,unsigned capacity,uint32_t *sequence)
{
    if(!guard){*sequence=0;return 0;}
    xSemaphoreTake(guard,portMAX_DELAY);unsigned n=sysex_size;
    *sequence=sysex_sequence;if(n>capacity)n=0;if(n)memcpy(data,sysex_last,n);
    xSemaphoreGive(guard);return n;
}
static void sysex_received(const uint8_t *data,size_t size,void *context)
{
    (void)context; // Called with guard held, from NimBLE host only.
    memcpy(sysex_last,data,size);sysex_size=size;sysex_sequence++;
    if(sysex_signal)xSemaphoreGive(sysex_signal);
}
bool fm1_ble_reserve_update(void)
{
    if(!guard)return true;
    xSemaphoreTake(guard,portMAX_DELAY);
    bool ok=!session_owner&&!state.pending&&!state.testing&&connection==BLE_HS_CONN_HANDLE_NONE&&
        (state.phase==FM1_BLE_OFF||state.phase==FM1_BLE_IDLE||state.phase==FM1_BLE_ERROR);
    if(ok)update_reserved=true;
    xSemaphoreGive(guard);return ok;
}
void fm1_ble_release_update(void)
{if(guard){xSemaphoreTake(guard,portMAX_DELAY);update_reserved=false;xSemaphoreGive(guard);}}

static bool current(uint16_t handle)
{xSemaphoreTake(guard,portMAX_DELAY);bool ok=handle==connection&&connection!=BLE_HS_CONN_HANDLE_NONE&&state.phase==FM1_BLE_DISCOVERING;xSemaphoreGive(guard);return ok;}
static int checked_mtu(uint16_t handle,const struct ble_gatt_error *error,uint16_t mtu,void *arg)
{
    (void)arg;
    ESP_LOGI("fm1_ble","MTU diagnostic handle=%u status=%d mtu=%u",handle,error->status,mtu);
    xSemaphoreTake(guard,portMAX_DELAY);if(handle==connection)link_check_pending=false;xSemaphoreGive(guard);
    return 0;
}
static int checked_cccd(uint16_t handle,const struct ble_gatt_error *error,struct ble_gatt_attr *attr,void *arg)
{
    (void)arg;uint8_t data[2]={0};int value=-1;
    if(!error->status&&attr&&attr->om&&OS_MBUF_PKTLEN(attr->om)==2&&!os_mbuf_copydata(attr->om,0,2,data))value=data[0]|data[1]<<8;
    ESP_LOGI("fm1_ble","CCCD diagnostic handle=%u status=%d value=%d MTU=%u",handle,error->status,value,ble_att_mtu(handle));
    xSemaphoreTake(guard,portMAX_DELAY);if(handle==connection)link_check_pending=false;xSemaphoreGive(guard);
    return 0;
}
static int subscribed(uint16_t handle,const struct ble_gatt_error *error,struct ble_gatt_attr *attr,void *arg)
{
    (void)attr;(void)arg;if(!current(handle))return 0;
    if(error->status) {
        if(!security_attempts&&(error->status==BLE_HS_ATT_ERR(BLE_ATT_ERR_INSUFFICIENT_AUTHEN)||
            error->status==BLE_HS_ATT_ERR(BLE_ATT_ERR_INSUFFICIENT_ENC))) {
            security_attempts++;int rc=ble_gap_security_initiate(handle);if(rc)failed("Pairing",rc);
        } else failed("MIDI subscription",error->status);
        return 0;
    }
    xSemaphoreTake(guard,portMAX_DELAY);discovery_deadline=0;xSemaphoreGive(guard);
    message(FM1_BLE_READY,"BLE MIDI ready. Test note: channel 1 / C4 / 300 ms.");
    return 0;
}
static void subscribe(void)
{ESP_LOGI("fm1_ble","MIDI value=%u CCCD=%u MTU=%u",value_handle,cccd,ble_att_mtu(connection));uint8_t on[]={1,0};int rc=ble_gattc_write_flat(connection,cccd,on,sizeof(on),subscribed,NULL);if(rc)failed("Subscribe",rc);}
static int descriptor(uint16_t handle,const struct ble_gatt_error *error,uint16_t chr,
                      const struct ble_gatt_dsc *dsc,void *arg)
{
    (void)chr;(void)arg;if(!current(handle))return 0;
    if(!error->status){if(ble_uuid_u16(&dsc->uuid.u)==0x2902)cccd=dsc->handle;return 0;}
    if(error->status!=BLE_HS_EDONE||!cccd){failed("MIDI notify descriptor",error->status);return 0;}
    subscribe();return 0;
}
static int characteristic(uint16_t handle,const struct ble_gatt_error *error,
                          const struct ble_gatt_chr *chr,void *arg)
{
    (void)arg;if(!current(handle))return 0;
    if(!error->status) {
        if(value_handle&&chr->def_handle>value_handle&&chr->def_handle-1<chr_end)chr_end=chr->def_handle-1;
        if(!ble_uuid_cmp(&chr->uuid.u,&midi_uuid.u)) {
            xSemaphoreTake(guard,portMAX_DELAY);value_handle=chr->val_handle;properties=chr->properties;xSemaphoreGive(guard);
        }
        return 0;
    }
    if(error->status!=BLE_HS_EDONE||!value_handle||!(properties&BLE_GATT_CHR_PROP_NOTIFY)||
        !(properties&BLE_GATT_CHR_PROP_WRITE_NO_RSP)){failed("MIDI characteristic",error->status);return 0;}
    int rc=ble_gattc_disc_all_dscs(handle,value_handle,chr_end,descriptor,NULL);if(rc)failed("Descriptor discovery",rc);return 0;
}
static int service(uint16_t handle,const struct ble_gatt_error *error,const struct ble_gatt_svc *svc,void *arg)
{
    (void)arg;if(!current(handle))return 0;
    if(!error->status){service_start=svc->start_handle;service_end=svc->end_handle;return 0;}
    if(error->status!=BLE_HS_EDONE||!service_start){failed("BLE MIDI service",error->status);return 0;}
    chr_end=service_end;
    int rc=ble_gattc_disc_all_chrs(handle,service_start,service_end,characteristic,NULL);if(rc)failed("Characteristic discovery",rc);return 0;
}
static void discovered(const struct ble_gap_disc_desc *d)
{
    bool midi=false;char name[32];
    if(!fm1_ble_advert(d->data,d->length_data,&midi,name))return;
    /* Retain named connectable devices too: some FM-1 builds put the service
     * in the scan response or omit it. GATT discovery remains mandatory. */
    if(d->event_type!=BLE_HCI_ADV_RPT_EVTYPE_ADV_IND&&d->event_type!=BLE_HCI_ADV_RPT_EVTYPE_SCAN_RSP)return;
    xSemaphoreTake(guard,portMAX_DELAY);
    if(state.phase!=FM1_BLE_SCANNING){xSemaphoreGive(guard);return;}
    unsigned i;for(i=0;i<state.count;i++)if(addresses[i].type==d->addr.type&&!memcmp(addresses[i].val,d->addr.val,6))break;
    bool changed=i==state.count;
    if(i==state.count) {
        if(i==FM1_BLE_DEVICES||(!midi&&!name[0])){xSemaphoreGive(guard);return;}
        state.count++;addresses[i]=d->addr;
        snprintf(state.devices[i].address,18,"%02X:%02X:%02X:%02X:%02X:%02X",d->addr.val[5],d->addr.val[4],d->addr.val[3],d->addr.val[2],d->addr.val[1],d->addr.val[0]);
    }
    fm1_ble_device_t *dev=&state.devices[i];
    changed|=(midi&&!dev->midi)||(name[0]&&strcmp(dev->name,name));
    dev->midi|=midi;dev->rssi=d->rssi;
    if(name[0])strcpy(dev->name,name);else if(!dev->name[0])strcpy(dev->name,dev->address);
    if(changed)state.revision++;
    xSemaphoreGive(guard);
}
static int gap_event(struct ble_gap_event *e,void *arg)
{
    (void)arg;
    switch(e->type) {
    case BLE_GAP_EVENT_DISC:discovered(&e->disc);break;
    case BLE_GAP_EVENT_DISC_COMPLETE:
        xSemaphoreTake(guard,portMAX_DELAY);bool scanning=state.phase==FM1_BLE_SCANNING;xSemaphoreGive(guard);
        if(scanning)message(FM1_BLE_IDLE,"Scan finished. Select FM-1; its BLE MIDI service will be checked.");
        break;
    case BLE_GAP_EVENT_CONNECT:
        xSemaphoreTake(guard,portMAX_DELAY);bool wanted=state.phase==FM1_BLE_CONNECTING;xSemaphoreGive(guard);
        if(!wanted){if(!e->connect.status)ble_gap_terminate(e->connect.conn_handle,BLE_ERR_REM_USER_CONN_TERM);break;}
        if(e->connect.status){failed("Connection",e->connect.status);break;}
        xSemaphoreTake(guard,portMAX_DELAY);connection=e->connect.conn_handle;
        service_start=service_end=value_handle=cccd=properties=0;security_attempts=0;
        discovery_deadline=esp_timer_get_time()+15000000;xSemaphoreGive(guard);
        message(FM1_BLE_DISCOVERING,"Connected. Checking MIDI service and enabling notifications...");
        {int rc=ble_gattc_disc_svc_by_uuid(connection,&service_uuid.u,service,NULL);if(rc)failed("Service discovery",rc);}break;
    case BLE_GAP_EVENT_DISCONNECT:
        xSemaphoreTake(guard,portMAX_DELAY);connection=BLE_HS_CONN_HANDLE_NONE;value_handle=0;discovery_deadline=0;
        memset(&sysex_rx,0,sizeof(sysex_rx));sysex_size=0;link_check_pending=false;
        bool error=state.phase==FM1_BLE_ERROR;state.peer[0]=0;state.last_rx[0]=0;state.connection_id++;state.revision++;xSemaphoreGive(guard);
        if(!error)message(FM1_BLE_IDLE,"BLE disconnected. Scan to reconnect.");
        break;
    case BLE_GAP_EVENT_ENC_CHANGE:
        if(current(e->enc_change.conn_handle)){if(e->enc_change.status)failed("Encryption",e->enc_change.status);else subscribe();}break;
    case BLE_GAP_EVENT_CONN_UPDATE: {
        struct ble_gap_conn_desc desc;
        int found=ble_gap_conn_find(e->conn_update.conn_handle,&desc);
        ESP_LOGI("fm1_ble","Interval diagnostic status=%d interval_units=%u latency=%u",
            e->conn_update.status,found?0:desc.conn_itvl,found?0:desc.conn_latency);
        xSemaphoreTake(guard,portMAX_DELAY);
        if(e->conn_update.conn_handle==connection)link_check_pending=false;
        xSemaphoreGive(guard);break;
    }
    case BLE_GAP_EVENT_NOTIFY_RX: {
        xSemaphoreTake(guard,portMAX_DELAY);
        ESP_LOGD("fm1_ble","Notification conn=%u expected=%u attr=%u expected=%u bytes=%u",
            e->notify_rx.conn_handle,connection,e->notify_rx.attr_handle,value_handle,
            (unsigned)OS_MBUF_PKTLEN(e->notify_rx.om));
        if(e->notify_rx.conn_handle==connection&&e->notify_rx.attr_handle==value_handle) {
            uint8_t data[512];unsigned size=OS_MBUF_PKTLEN(e->notify_rx.om);unsigned n=size<sizeof(data)?size:sizeof(data);
            if(os_mbuf_copydata(e->notify_rx.om,0,n,data)==0) {
                state.rx_packets++;state.rx_bytes+=size;
                if(size>sizeof(data)||!fm1_ble_sysex_receive(&sysex_rx,data,n,sysex_received,NULL))state.rx_invalid++;
                for(unsigned i=0;i<n&&i<24;i++)snprintf(state.last_rx+i*3,4,"%02X ",data[i]);
                state.revision++;
            }
        }
        xSemaphoreGive(guard);break;
    }
    default:break;
    }
    return 0;
}
static void on_sync(void)
{
    int rc=ble_hs_util_ensure_addr(0);if(!rc)rc=ble_hs_id_infer_auto(0,&own_type);
    xSemaphoreTake(guard,portMAX_DELAY);synced=!rc;xSemaphoreGive(guard);
    if(rc)failed("BLE address",rc);
}
static void on_reset(int reason)
{
    xSemaphoreTake(guard,portMAX_DELAY);synced=false;connection=BLE_HS_CONN_HANDLE_NONE;value_handle=0;discovery_deadline=0;link_check_pending=false;xSemaphoreGive(guard);
    failed("Bluetooth host reset",reason);
}
static void host_task(void *arg)
{(void)arg;nimble_port_run();nimble_port_freertos_deinit();}
static void transport_event(void *arg,esp_event_base_t base,int32_t id,void *data)
{
    (void)arg;(void)base;(void)data;
    if(id==ESP_HOSTED_EVENT_TRANSPORT_FAILURE||id==ESP_HOSTED_EVENT_TRANSPORT_DOWN) {
        xSemaphoreTake(guard,portMAX_DELAY);synced=false;discovery_deadline=0;xSemaphoreGive(guard);
        message(FM1_BLE_ERROR,"C6 communication failed. Restart Tab5 before retrying Bluetooth.");
    }
}
static bool initialize(void)
{
    if(started){xSemaphoreTake(guard,portMAX_DELAY);bool ready=synced;xSemaphoreGive(guard);if(!ready)message(FM1_BLE_ERROR,"C6 / BLE unavailable. Restart Tab5 before retrying.");return ready;}
    started=true;message(FM1_BLE_STARTING,"Starting C6 Bluetooth controller...");
    esp_err_t rc=nvs_flash_init(); // Never erase the user's NVS on an init error.
    if(rc){failed("NVS initialization",rc);return false;}
    rc=esp_event_loop_create_default();if(rc!=ESP_OK&&rc!=ESP_ERR_INVALID_STATE){failed("Event loop",rc);return false;}
    rc=esp_event_handler_register(ESP_HOSTED_EVENT,ESP_EVENT_ANY_ID,transport_event,NULL);
    if(rc){failed("C6 event handler",rc);return false;}
    rc=bsp_feature_enable(BSP_FEATURE_WIFI,true);if(rc){failed("C6 power",rc);return false;}
    esp_io_expander_handle_t io=bsp_io_expander_init();
    if(!io){failed("Antenna control",-1);return false;}
    rc=esp_io_expander_set_dir(io,IO_EXPANDER_PIN_NUM_0,IO_EXPANDER_OUTPUT);
    if(!rc)rc=esp_io_expander_set_level(io,IO_EXPANDER_PIN_NUM_0,0);
    if(rc){failed("Antenna control",rc);return false;}
    vTaskDelay(pdMS_TO_TICKS(200));
    ESP_LOGI("fm1_ble","DMA heap: %u free, %u largest",(unsigned)heap_caps_get_free_size(MALLOC_CAP_DMA),
        (unsigned)heap_caps_get_largest_free_block(MALLOC_CAP_DMA));
    // Hosted uses assertions for some allocation failures. Reject startup
    // before entering it if display/other tasks consumed the working reserve.
    if(heap_caps_get_free_size(MALLOC_CAP_INTERNAL)<100*1024||
       heap_caps_get_largest_free_block(MALLOC_CAP_INTERNAL)<16*1024) {
        message(FM1_BLE_ERROR,"Not enough internal RAM to start Bluetooth. Restart Tab5.");return false;
    }
    rc=esp_hosted_init();if(!rc)rc=esp_hosted_connect_to_slave();
    if(rc){failed("C6 SDIO",rc);return false;}
    esp_hosted_coprocessor_fwver_t version;
    rc=esp_hosted_get_coprocessor_fwversion(&version);
    if(rc) {
        message(FM1_BLE_ERROR,"C6 SDIO link unavailable. Restart Tab5 before retrying Bluetooth.");return false;
    } else {
        xSemaphoreTake(guard,portMAX_DELAY);snprintf(state.coprocessor,sizeof(state.coprocessor),"%lu.%lu.%lu",
            (unsigned long)version.major1,(unsigned long)version.minor1,(unsigned long)version.patch1);state.revision++;xSemaphoreGive(guard);
    }
    // Before Hosted 2.5.2 the controller starts automatically and the newer
    // FeatureControl RPC is unavailable (including Tab5 factory C6 1.4.1).
    bool explicit_controller=version.major1>2||(version.major1==2&&
        (version.minor1>5||(version.minor1==5&&version.patch1>=2)));
    if(explicit_controller) {
        rc=esp_hosted_bt_controller_init();if(!rc)rc=esp_hosted_bt_controller_enable();
        if(rc){message(FM1_BLE_ERROR,"C6 Bluetooth controller failed. Restart Tab5 before retrying.");return false;}
    } else ESP_LOGI("fm1_ble","Legacy C6: using controller enabled at boot");
    rc=nimble_port_init();if(rc){failed("NimBLE initialization",rc);return false;}
    // Per-packet INFO output competes with the diagnostic data on USB serial.
    esp_log_level_set("NimBLE",ESP_LOG_WARN);
    ble_hs_cfg.sync_cb=on_sync;ble_hs_cfg.reset_cb=on_reset;
    ble_hs_cfg.sm_io_cap=BLE_HS_IO_NO_INPUT_OUTPUT;ble_hs_cfg.sm_sc=1;
    nimble_port_freertos_init(host_task);
    for(unsigned i=0;i<80;i++) {
        xSemaphoreTake(guard,portMAX_DELAY);bool ready=synced;xSemaphoreGive(guard);
        if(ready)return true;
        vTaskDelay(pdMS_TO_TICKS(100));
    }
    failed("BLE synchronization",BLE_HS_ETIMEOUT);return false;
}
static int short_message(uint16_t handle,uint16_t value,const uint8_t midi[3])
{
    uint8_t packet[5];size_t n=fm1_ble_midi_packet(esp_timer_get_time()/1000,midi,3,packet,sizeof(packet));
    int rc=ble_gattc_write_no_rsp_flat(handle,value,packet,n);
    if(!rc){xSemaphoreTake(guard,portMAX_DELAY);state.tx_packets++;state.revision++;xSemaphoreGive(guard);}return rc;
}
static void worker(void *arg)
{
    (void)arg;command_t c;
    for(;;) {
        if(xQueueReceive(commands,&c,pdMS_TO_TICKS(100))!=pdTRUE) {
            xSemaphoreTake(guard,portMAX_DELAY);bool expired=discovery_deadline&&esp_timer_get_time()>discovery_deadline;discovery_deadline=expired?0:discovery_deadline;xSemaphoreGive(guard);
            if(expired)failed("BLE connection/discovery timeout",BLE_HS_ETIMEOUT);
            continue;
        }
        int rc=0;
        if(c.kind==1&&initialize()) {
            xSemaphoreTake(guard,portMAX_DELAY);state.count=0;state.scan_id++;memset(state.devices,0,sizeof(state.devices));xSemaphoreGive(guard);
            message(FM1_BLE_SCANNING,"Scanning for 10 seconds. Enable FM-1 Bluetooth MIDI.");
            struct ble_gap_disc_params params={.passive=0,.itvl=160,.window=80,.filter_duplicates=0};
            rc=ble_gap_disc(own_type,10000,&params,gap_event,NULL);if(rc)failed("Scan",rc);
        } else if(c.kind==2) {
            message(FM1_BLE_CONNECTING,"Connecting to selected device...");ble_gap_disc_cancel();
            xSemaphoreTake(guard,portMAX_DELAY);strcpy(state.peer,c.name);discovery_deadline=esp_timer_get_time()+15000000;xSemaphoreGive(guard);
            rc=ble_gap_connect(own_type,&c.address,10000,NULL,gap_event,NULL);if(rc)failed("Connection",rc);
        } else if(c.kind==3) {
            ble_gap_disc_cancel();ble_gap_conn_cancel();
            xSemaphoreTake(guard,portMAX_DELAY);uint16_t handle=connection;xSemaphoreGive(guard);
            if(handle!=BLE_HS_CONN_HANDLE_NONE){rc=ble_gap_terminate(handle,BLE_ERR_REM_USER_CONN_TERM);if(rc)failed("Disconnect",rc);}
            else {xSemaphoreTake(guard,portMAX_DELAY);discovery_deadline=0;state.peer[0]=0;xSemaphoreGive(guard);message(FM1_BLE_IDLE,"BLE disconnected. Scan to reconnect.");}
        } else if(c.kind==6||c.kind==7) {
            xSemaphoreTake(guard,portMAX_DELAY);uint16_t handle=connection,descriptor_handle=cccd;
            link_check_pending=state.phase==FM1_BLE_READY;bool ready=link_check_pending;xSemaphoreGive(guard);
            if(ready) {
                rc=c.kind==6?ble_gattc_exchange_mtu(handle,checked_mtu,NULL):ble_gattc_read(handle,descriptor_handle,checked_cccd,NULL);
                if(rc){xSemaphoreTake(guard,portMAX_DELAY);link_check_pending=false;xSemaphoreGive(guard);}
                ESP_LOGI("fm1_ble","Link diagnostic kind=%u started rc=%d",c.kind,rc);
            }
        } else if(c.kind==8) {
            xSemaphoreTake(guard,portMAX_DELAY);uint16_t handle=connection;
            link_check_pending=state.phase==FM1_BLE_READY;bool ready=link_check_pending;xSemaphoreGive(guard);
            struct ble_gap_conn_desc desc;
            if(ready&&!ble_gap_conn_find(handle,&desc)&&desc.conn_itvl==6&&!desc.conn_latency) {
                ESP_LOGI("fm1_ble","Interval diagnostic status=0 interval_units=6 latency=0");
                xSemaphoreTake(guard,portMAX_DELAY);link_check_pending=false;xSemaphoreGive(guard);
            } else if(ready) {
                const struct ble_gap_upd_params params={.itvl_min=6,.itvl_max=6,.latency=0,.supervision_timeout=400};
                rc=ble_gap_update_params(handle,&params);
                if(rc) {
                    ESP_LOGI("fm1_ble","Interval diagnostic status=%d interval_units=0 latency=0",rc);
                    xSemaphoreTake(guard,portMAX_DELAY);link_check_pending=false;xSemaphoreGive(guard);
                }
            }
        } else if(c.kind==5) {
            xSemaphoreTake(guard,portMAX_DELAY);uint16_t handle=connection,value=value_handle;xSemaphoreGive(guard);
            size_t offset=0;uint16_t mtu=ble_att_mtu(handle);
            uint8_t packet[FM1_BLE_SYSEX_MAX+3];
            size_t capacity=mtu>=23?mtu-3:20;
            if(capacity>sizeof(packet))capacity=sizeof(packet);
            while(offset<c.size) {
                size_t next=offset,n=fm1_ble_sysex_packet(esp_timer_get_time()/1000,c.data,c.size,&next,packet,capacity);
                if(!n){rc=BLE_HS_EINVAL;break;}
                for(unsigned attempt=0;attempt<10;attempt++) {
                    xSemaphoreTake(guard,portMAX_DELAY);bool same=connection==handle&&state.phase==FM1_BLE_READY;xSemaphoreGive(guard);
                    if(!same){rc=BLE_HS_ENOTCONN;break;}
                    rc=ble_gattc_write_no_rsp_flat(handle,value,packet,n);
                    if(!rc)break;
                    if(rc!=BLE_HS_ENOMEM&&rc!=BLE_HS_EBUSY)break;
                    vTaskDelay(pdMS_TO_TICKS(30));
                }
                if(rc)break;
                offset=next;xSemaphoreTake(guard,portMAX_DELAY);state.tx_packets++;state.revision++;xSemaphoreGive(guard);
                // The application reply paces transactions. Only back off when
                // the host queue is full; do not sleep after every ATT packet.
            }
            if(rc)failed("App SysEx transmission",rc);
        } else if(c.kind==4) {
            xSemaphoreTake(guard,portMAX_DELAY);uint16_t handle=connection,value=value_handle;
            bool ready=state.phase==FM1_BLE_READY;state.testing=ready;xSemaphoreGive(guard);
            if(ready) {
                const uint8_t on[]={(uint8_t)(0x90|c.data[1]),c.data[0],80},off[]={(uint8_t)(0x80|c.data[1]),c.data[0],0};rc=short_message(handle,value,on);
                if(!rc) {
                    vTaskDelay(pdMS_TO_TICKS(300));
                    /* Block further commands until Note Off is queued. Retry a
                     * transient full TX queue, never replay after reconnection. */
                    for(unsigned attempt=0;attempt<10;attempt++) {
                        xSemaphoreTake(guard,portMAX_DELAY);bool same=connection==handle&&state.phase==FM1_BLE_READY;xSemaphoreGive(guard);
                        if(!same){rc=BLE_HS_ENOTCONN;break;}
                        rc=short_message(handle,value,off);if(!rc)break;vTaskDelay(pdMS_TO_TICKS(30));
                    }
                }
                if(rc)failed("Test note (Note Off not confirmed)",rc);
            }
            xSemaphoreTake(guard,portMAX_DELAY);state.testing=false;xSemaphoreGive(guard);
        }
        xSemaphoreTake(guard,portMAX_DELAY);state.pending=false;state.revision++;xSemaphoreGive(guard);
    }
}
void fm1_ble_start(void)
{
    guard=xSemaphoreCreateMutex();commands=xQueueCreate(1,sizeof(command_t));sysex_signal=xSemaphoreCreateBinary();
    if(!guard||!commands||!sysex_signal)return;
    strcpy(state.message,"Bluetooth idle. Scan to start; USB editing remains available.");
    if(xTaskCreate(worker,"fm1_ble",6144,NULL,4,NULL)!=pdPASS)message(FM1_BLE_ERROR,"Bluetooth worker could not start.");
}

bool fm1_ble_session_begin(void)
{
    if(!guard)return false;
    xSemaphoreTake(guard,portMAX_DELAY);
    bool ok=!session_owner&&!update_reserved&&!state.pending&&!state.testing&&!link_check_pending&&state.phase==FM1_BLE_READY;
    uint16_t handle=connection;
    if(ok){session_owner=xTaskGetCurrentTaskHandle();state.reserved=true;state.revision++;}
    xSemaphoreGive(guard);
    if(ok&&ble_att_mtu(handle)!=23){fm1_ble_session_end();return false;}
    return ok;
}
void fm1_ble_session_end(void)
{
    xSemaphoreTake(guard,portMAX_DELAY);
    if(session_owner==xTaskGetCurrentTaskHandle()){session_owner=NULL;state.reserved=false;state.revision++;}
    xSemaphoreGive(guard);
}
