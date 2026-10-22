#include "fm1_host.h"
#include "fm1_params.h"
#include "fm1_adapter_wire.h"
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "bsp/esp-bsp.h"
#include "freertos/FreeRTOS.h"
#include "freertos/queue.h"
#include "freertos/semphr.h"
#include "freertos/task.h"
#include "usb/usb_host.h"
#include "esp_log.h"
#include "esp_timer.h"

static const char *TAG="fm1_host";
static SemaphoreHandle_t state_mutex;
static QueueHandle_t commands;
static fm1_state_t state;
static usb_host_client_handle_t client;
static usb_device_handle_t device;
static usb_transfer_t *rx, *tx;
static bool rx_busy, tx_busy, closing, claimed;
static uint8_t open_address, in_ep, out_ep, interface_number;
static uint16_t in_mps;
static fm1_stream_t stream;
static uint8_t app_reply[192];
static unsigned app_reply_size;
static uint32_t app_reply_sequence;
typedef struct {
    uint32_t ticket,connection_id;
    int64_t deadline;
    char identity[32];
    uint16_t size;
    uint8_t prefix[8],prefix_size;
    uint8_t data[FM1_FRAME_MAX];
} remote_command_t;
typedef struct {uint32_t ticket;int size;uint8_t data[FM1_FRAME_MAX];} remote_result_t;
static QueueHandle_t remote_requests,remote_results;
static SemaphoreHandle_t remote_guard;
static remote_command_t remote_active;
static uint32_t remote_ticket;
static int64_t remote_quiet_until;
static bool tx_is_remote;
static void remote_complete(const uint8_t *data,int size)
{
    if(!remote_active.ticket)return;
    remote_result_t result={.ticket=remote_active.ticket,.size=size};
    if(size>0&&data)memcpy(result.data,data,size);
    xQueueOverwrite(remote_results,&result);remote_active.ticket=0;
}
int fm1_host_exchange(const char *identity,uint32_t connection_id,const uint8_t *data,unsigned size,
    const uint8_t *prefix,unsigned prefix_size,uint8_t *reply,unsigned capacity,unsigned timeout_ms)
{
    if(!remote_guard||!identity||!data||!size||size>FM1_FRAME_MAX||prefix_size>8||
       (!prefix&&prefix_size)||timeout_ms<100||timeout_ms>15000)return -1;
    const fm1_adapter_profile_t *profile=fm1_adapter_profile(identity);uint8_t midi[4];
    if(profile->id==FW_UNKNOWN)return -1;
    if(data[0]==0xf0){if(!fm1_adapter_sysex_allowed(profile->id,data,size))return -1;}
    else if(prefix_size||!fm1_midi_packet(data,size,midi))return -1;
    if(xSemaphoreTake(remote_guard,0)!=pdTRUE)return -1;
    fm1_state_t current;fm1_host_snapshot(&current);
    if(current.updating||!current.connected||current.connection_id!=connection_id||strcmp(identity,current.identity)){
        xSemaphoreGive(remote_guard);return -1;
    }
    remote_command_t cmd={.ticket=++remote_ticket,.connection_id=connection_id,.size=size,.prefix_size=prefix_size,
        .deadline=esp_timer_get_time()+(int64_t)timeout_ms*1000};
    if(!cmd.ticket)cmd.ticket=++remote_ticket;
    snprintf(cmd.identity,sizeof(cmd.identity),"%s",identity);memcpy(cmd.data,data,size);
    if(prefix_size)memcpy(cmd.prefix,prefix,prefix_size);
    xQueueReset(remote_results);
    int result=-1;
    if(xQueueSend(remote_requests,&cmd,0)==pdTRUE){
        remote_result_t received;
        if(xQueueReceive(remote_results,&received,pdMS_TO_TICKS(timeout_ms+100))==pdTRUE&&received.ticket==cmd.ticket){
            result=received.size;
            if(result>0){if(!reply||(unsigned)result>capacity)result=-1;else memcpy(reply,received.data,result);}
        }
    }
    xSemaphoreGive(remote_guard);return result;
}
unsigned fm1_host_app_last(uint8_t *data,unsigned capacity,uint32_t *sequence)
{
    if(!state_mutex){*sequence=0;return 0;}
    xSemaphoreTake(state_mutex,portMAX_DELAY);unsigned n=app_reply_size;
    *sequence=app_reply_sequence;if(n>capacity)n=0;if(n)memcpy(data,app_reply,n);
    xSemaphoreGive(state_mutex);return n;
}
static int64_t next_poll, next_send;
static bool need_identity, pending;
static uint16_t pending_parameter;
static uint8_t pending_value;
static int64_t pending_since;
typedef struct { bool read,fx; uint16_t parameter; uint8_t value,channel,scale; uint32_t generation; } command_t;
static bool tx_is_fx;
static command_t tx_fx;
static SemaphoreHandle_t ota_mutex;
static fm1_ota_t ota;
static bool tx_is_ota,ota_start_pending;
static const uint8_t *ota_image;
static size_t ota_size;
static char ota_source[32],ota_target[21];
static uint32_t ota_expected_connection,ota_verified_connection;
static esp_timer_handle_t ota_wake_timer;
static int64_t clock_next;
bool fm1_host_clock(uint32_t connection_id,unsigned bpm,bool enabled)
{
    if(!state_mutex||bpm<30||bpm>300)return false;
    xSemaphoreTake(state_mutex,portMAX_DELAY);
    bool ok=state.connected&&!state.updating&&state.connection_id==connection_id&&fm1_adapter_profile(state.identity)->id!=FW_UNKNOWN;
    if(ok){state.clock_enabled=enabled;state.clock_bpm=bpm;clock_next=esp_timer_get_time();}
    xSemaphoreGive(state_mutex);if(ok&&client)usb_host_client_unblock(client);return ok;
}

static void ota_wake(void *arg)
{
    (void)arg;
    // Task-dispatched timer: wake the USB event loop, never send from a timer callback.
    usb_host_client_unblock(client);
}

static void wait_host_events(void)
{
    esp_timer_stop(ota_wake_timer);
    xSemaphoreTake(ota_mutex,portMAX_DELAY);
    int64_t due=fm1_ota_wakeup(&ota);
    xSemaphoreGive(ota_mutex);
    int64_t due_us=due<0?-1:due*1000;
    xSemaphoreTake(state_mutex,portMAX_DELAY);
    if(state.clock_enabled&&!state.updating&&(due_us<0||clock_next<due_us))due_us=clock_next;
    xSemaphoreGive(state_mutex);
    TickType_t wait=pdMS_TO_TICKS(10);
    if(due_us>=0) {
        int64_t delay_us=due_us-esp_timer_get_time();
        if(delay_us<=0)wait=tx_busy?1:0;
        else {
            ESP_ERROR_CHECK(esp_timer_start_once(ota_wake_timer,(uint64_t)delay_us));
            wait=portMAX_DELAY;
        }
    }
    // USB callbacks wake immediately; queued OTA data wakes exactly at its pacing deadline.
    usb_host_client_handle_events(client,wait);
}

void fm1_host_update_lock(bool lock)
{
    xSemaphoreTake(state_mutex,portMAX_DELAY);state.updating=lock;if(lock)state.clock_enabled=false;state.revision++;xSemaphoreGive(state_mutex);
    if(lock)xQueueReset(commands);
}
bool fm1_host_ota_begin(const uint8_t *image,size_t size,const char *source,const char *target,uint32_t expected_connection)
{
    fm1_state_t current;fm1_host_snapshot(&current);
    if(!current.connected||current.connection_id!=expected_connection||strcmp(current.identity,source))return false;
    xSemaphoreTake(ota_mutex,portMAX_DELAY);
    bool ok=!fm1_ota_active(&ota)&&!ota_start_pending;
    if(ok){ota_image=image;ota_size=size;snprintf(ota_source,sizeof(ota_source),"%s",source);snprintf(ota_target,sizeof(ota_target),"%s",target);
        ota_expected_connection=expected_connection;ota_verified_connection=0;
        ota.phase=OTA_IDLE;ota_start_pending=true;}
    xSemaphoreGive(ota_mutex);return ok;
}
void fm1_host_ota_snapshot(fm1_ota_t *out,uint32_t *verified_connection)
{xSemaphoreTake(ota_mutex,portMAX_DELAY);fm1_ota_sample(&ota,esp_timer_get_time()/1000);*out=ota;if(verified_connection)*verified_connection=ota_verified_connection;xSemaphoreGive(ota_mutex);}

static void status(const char *message)
{
    xSemaphoreTake(state_mutex,portMAX_DELAY);
    snprintf(state.status,sizeof(state.status),"%s",message);
    state.revision++;
    xSemaphoreGive(state_mutex);
    ESP_LOGI(TAG,"%s",message);
}

void fm1_host_snapshot(fm1_state_t *out)
{
    if(!state_mutex) {memset(out,0,sizeof(*out));return;}
    xSemaphoreTake(state_mutex,portMAX_DELAY); *out=state; xSemaphoreGive(state_mutex);
}

bool fm1_host_read(void)
{
    fm1_state_t s;fm1_host_snapshot(&s);if(s.updating)return false;
    command_t cmd={.read=true};
    return commands && xQueueSend(commands,&cmd,0)==pdTRUE;
}

bool fm1_host_parameter(uint16_t p,uint8_t v)
{
    fm1_state_t current; fm1_host_snapshot(&current);
    bool allowed=fm1_param_valid(p,v);
    if(current.updating || !current.connected || !current.valid_voice || !current.can_edit || !allowed ||
       esp_timer_get_time()-current.last_read_us>=3000000) return false;
    command_t cmd={.parameter=p,.value=v};
    return xQueueSend(commands,&cmd,0)==pdTRUE;
}

bool fm1_host_fx_config(uint8_t channel,uint8_t scale)
{
    if(channel>=16 || (scale!=10 && scale!=100))return false;
    xSemaphoreTake(state_mutex,portMAX_DELAY);
    state.fx_channel=channel;state.fx_scale=scale;state.fx_known=0;
    state.fx_config_revision++;state.revision++;
    xSemaphoreGive(state_mutex);return true;
}

bool fm1_host_effect(uint8_t cc,uint8_t value)
{
    fm1_state_t current;fm1_host_snapshot(&current);uint8_t packet[4];
    if(current.updating || !current.connected || !current.can_edit || !current.valid_voice ||
       esp_timer_get_time()-current.last_read_us>=3000000 ||
       !fm1_cc_packet(current.fx_channel,cc,value,current.fx_scale,packet))return false;
    command_t cmd={.fx=true,.parameter=cc,.value=value,.channel=current.fx_channel,
        .scale=current.fx_scale,.generation=current.fx_config_revision};
    return xQueueSend(commands,&cmd,0)==pdTRUE;
}

static void receive_frame(const uint8_t *frame,size_t n)
{
    if(remote_active.ticket&&remote_active.prefix_size&&n>=remote_active.prefix_size&&
       !memcmp(frame,remote_active.prefix,remote_active.prefix_size))remote_complete(frame,(int)n);
    uint8_t cmd,payload[256]; size_t size=sizeof(payload);
    xSemaphoreTake(state_mutex,portMAX_DELAY);
    state.rx_frames++;state.last_frame_size=n;
    if(n>=10&&n<=sizeof(app_reply)&&!memcmp(frame,(uint8_t[]){0xf0,0x7d,'F','M',2},5)&&frame[n-1]==0xf7) {
        memcpy(app_reply,frame,n);app_reply_size=n;app_reply_sequence++;
        xSemaphoreGive(state_mutex);return;
    }
    if(n>=6&&(!memcmp(frame,(uint8_t[]){0xf0,0x7d,'F','L'},4)||
       (fm1_adapter_profile(state.identity)->id==FW_VA&&frame[0]==0xf0&&frame[1]==0x7d))){
        xSemaphoreGive(state_mutex);return;
    }
    if(!fm1_decode(frame,n,&cmd,payload,&size)) {
        state.bad_frames++;xSemaphoreGive(state_mutex);return;
    }
    state.last_command=cmd;
    if(cmd==0x30) {
        xSemaphoreTake(ota_mutex,portMAX_DELAY);fm1_ota_request(&ota,payload,size,esp_timer_get_time()/1000);xSemaphoreGive(ota_mutex);
        xSemaphoreGive(state_mutex);return;
    }
    if(cmd==0x11 && size==27) {
        memset(state.identity,0,sizeof(state.identity));
        for(size_t i=0;i<size && payload[i];i++)
            state.identity[i]=(payload[i]>=32 && payload[i]<=126)?payload[i]:'?';
        // Read-only on unknown/custom identities until a dedicated adapter exists.
        const char *id=state.identity;
        state.can_edit=strlen(id)==8 && !strncmp(id,"FM-1_",5)
            && id[5]>='0' && id[5]<='9' && id[6]>='0' && id[6]<='9'
            && id[7]>='0' && id[7]<='9' && atoi(id+5)<=15;
        if(state.can_edit)state.fx_scale=atoi(id+5)>=15?100:10;
        else snprintf(state.status,sizeof(state.status),"%s",fm1_ota_loader(id)?
            "Update loader connected / select package in FIRMWARE":"Firmware connected / use FIRMWARE for installation");
        ESP_LOGI(TAG,"Identity: %s; editing %s",id,state.can_edit?"enabled":"disabled");
        state.revision++;
        xSemaphoreTake(ota_mutex,portMAX_DELAY);
        bool was_done=ota.phase==OTA_DONE;
        fm1_ota_identity(&ota,id,esp_timer_get_time()/1000);
        if(!was_done&&ota.phase==OTA_DONE)ota_verified_connection=state.connection_id;
        xSemaphoreGive(ota_mutex);
    }
    const uint8_t voice_head[]={5,0,0,0,0,155,0,0};
    if(cmd==0x23 && size==163 && !memcmp(payload,voice_head,8)) {
        bool valid=true; for(size_t i=8;i<size;i++) if(payload[i]>127) valid=false;
        if(valid) {
            bool changed=!state.valid_voice || memcmp(state.voice,payload+8,155)!=0;
            if(changed)state.fx_known=0; // A new voice may load different stored effects.
            memcpy(state.voice,payload+8,155);state.valid_voice=true;
            state.last_read_us=esp_timer_get_time();state.revision++;
            const char *message="Current voice received";
            if(pending) {
                message=state.voice[pending_parameter]==pending_value?
                    "Edit verified by readback":"Edit not confirmed - check MIDI channel";
                if(state.voice[pending_parameter]==pending_value)state.writes_verified++;
                else state.writes_failed++;
                pending=false;
                ESP_LOGI(TAG,"%s: parameter %u = %u",message,pending_parameter,state.voice[pending_parameter]);
            }
            if(changed) ESP_LOGI(TAG,"Voice: %.10s; algorithm %u; feedback %u",
                (const char *)state.voice+145,state.voice[134]+1,state.voice[135]);
            snprintf(state.status,sizeof(state.status),"%s",message);
        }
    }
    xSemaphoreGive(state_mutex);
}

static void rx_done(usb_transfer_t *t)
{
    rx_busy=false;
    if(t->status==USB_TRANSFER_STATUS_COMPLETED && !closing) {
        xSemaphoreTake(state_mutex,portMAX_DELAY);state.rx_packets+=t->actual_num_bytes/4;xSemaphoreGive(state_mutex);
        for(int i=0;i+3<t->actual_num_bytes;i+=4) {
            size_t size=fm1_stream_packet(&stream,t->data_buffer+i);
            if(size) receive_frame(stream.data,size);
        }
        t->num_bytes=in_mps;
        if(usb_host_transfer_submit(t)==ESP_OK) rx_busy=true;
        else closing=true;
    } else if(!closing) {
        status("USB receive stopped; reconnect FM-1");closing=true;
    }
}

static void tx_done(usb_transfer_t *t)
{
    tx_busy=false;
    if(tx_is_remote){
        if(t->status!=USB_TRANSFER_STATUS_COMPLETED||closing)remote_complete(NULL,-1);
        else if(!remote_active.prefix_size)remote_complete(NULL,0);
        tx_is_remote=false;
    }
    if(tx_is_ota) {
        xSemaphoreTake(ota_mutex,portMAX_DELAY);fm1_ota_sent(&ota,t->status==USB_TRANSFER_STATUS_COMPLETED,esp_timer_get_time()/1000);xSemaphoreGive(ota_mutex);
        tx_is_ota=false;
    }
    if(tx_is_fx && t->status==USB_TRANSFER_STATUS_COMPLETED && !closing) {
        xSemaphoreTake(state_mutex,portMAX_DELAY);
        if(tx_fx.generation==state.fx_config_revision) {
            state.fx_values[tx_fx.parameter]=tx_fx.value;state.fx_known|=1u<<tx_fx.parameter;state.revision++;
        }
        xSemaphoreGive(state_mutex);
    }
    tx_is_fx=false;
    if(t->status!=USB_TRANSFER_STATUS_COMPLETED && !closing)
        status("USB send failed; read the voice again");
}

static void device_event(const usb_host_client_event_msg_t *event,void *arg)
{
    (void)arg;
    if(event->event==USB_HOST_CLIENT_EVENT_NEW_DEV && !device)
        open_address=event->new_dev.address;
    if(event->event==USB_HOST_CLIENT_EVENT_DEV_GONE && event->dev_gone.dev_hdl==device)
        closing=true;
}

static bool find_midi(const usb_config_desc_t *config)
{
    const uint8_t *d=(const uint8_t *)config; size_t total=config->wTotalLength;
    bool midi=false; uint8_t candidate=0,ie=0,oe=0; uint16_t mps=0;
    for(size_t pos=0;pos+2<=total;) {
        uint8_t len=d[pos],type=d[pos+1]; if(len<2 || pos+len>total) break;
        const uint8_t *p=d+pos;
        if(type==4 && len>=9) {
            midi=p[3]==0 && p[5]==1 && p[6]==3; // Audio class, MIDIStreaming, alternate 0.
            candidate=p[2];ie=oe=0;mps=0;
        } else if(midi && type==5 && len>=7 && (p[3]&3)==2) {
            if(p[2]&0x80) {ie=p[2];mps=(uint16_t)p[4]|(uint16_t)p[5]<<8;}
            else oe=p[2];
            if(ie && oe && mps>=4 && mps<=512 && mps%4==0) {
                interface_number=candidate;in_ep=ie;out_ep=oe;in_mps=mps;return true;
            }
        }
        pos+=len;
    }
    return false;
}

static void close_device(void)
{
    xSemaphoreTake(state_mutex,portMAX_DELAY);
    state.connected=false;state.clock_enabled=false;state.valid_voice=false;state.can_edit=false;state.identity[0]=0;state.revision++;
    state.fx_known=0;state.fx_config_revision++;
    state.connection_id++;
    xSemaphoreGive(state_mutex);
    // Flush first, then keep pumping callbacks until no transfer owns a DMA buffer.
    if(claimed) {
        usb_host_endpoint_halt(device,in_ep);usb_host_endpoint_flush(device,in_ep);
        usb_host_endpoint_halt(device,out_ep);usb_host_endpoint_flush(device,out_ep);
    }
    while(rx_busy || tx_busy) usb_host_client_handle_events(client,pdMS_TO_TICKS(20));
    xSemaphoreTake(ota_mutex,portMAX_DELAY);fm1_ota_connection(&ota,false,esp_timer_get_time()/1000);xSemaphoreGive(ota_mutex);
    if(claimed) usb_host_interface_release(client,device,interface_number);
    if(rx) usb_host_transfer_free(rx);
    if(tx) usb_host_transfer_free(tx);
    rx=tx=NULL;
    usb_host_device_close(client,device);device=NULL;
    claimed=closing=pending=false;stream=(fm1_stream_t){0};
    xQueueReset(commands);status("Connect FM-1 to the Tab5 USB-A port");
    remote_complete(NULL,-1);xQueueReset(remote_requests);
}

static void open_device(uint8_t address)
{
    if(usb_host_device_open(client,address,&device)!=ESP_OK) {device=NULL;return;}
    const usb_device_desc_t *descriptor;
    const usb_config_desc_t *config;
    if(usb_host_get_device_descriptor(device,&descriptor)!=ESP_OK) {
        usb_host_device_close(client,device);device=NULL;status("USB descriptor unavailable");return;
    }
    ESP_LOGI("fm1_host","USB VID:PID %04x:%04x",descriptor->idVendor,descriptor->idProduct);
    if(!((descriptor->idVendor==0x4c4a&&(descriptor->idProduct==0xc755||descriptor->idProduct==0x4155)) ||
         (descriptor->idVendor==0x4d4a&&descriptor->idProduct==0x4155) ||
         (descriptor->idVendor==0x3654&&descriptor->idProduct==0x7756) || // FM1 Open V12; identity still required
         (descriptor->idVendor==0x1209&&(descriptor->idProduct==1||descriptor->idProduct==2)))) {
        usb_host_device_close(client,device);device=NULL;
        status("Unsupported USB device; connect FM-1");return;
    }
    if(usb_host_get_active_config_descriptor(device,&config)!=ESP_OK || !find_midi(config) ||
       usb_host_interface_claim(client,device,interface_number,0)!=ESP_OK) {
        closing=true;return;
    }
    claimed=true;
    if(usb_host_transfer_alloc(in_mps,0,&rx)!=ESP_OK ||
       usb_host_transfer_alloc(1536,0,&tx)!=ESP_OK) {closing=true;return;}
    rx->device_handle=device;rx->bEndpointAddress=in_ep;rx->callback=rx_done;rx->num_bytes=in_mps;
    tx->device_handle=device;tx->bEndpointAddress=out_ep;tx->callback=tx_done;
    if(usb_host_transfer_submit(rx)!=ESP_OK) {closing=true;return;}
    rx_busy=true;need_identity=true;next_poll=next_send=0;
    xSemaphoreTake(ota_mutex,portMAX_DELAY);fm1_ota_connection(&ota,true,esp_timer_get_time()/1000);xSemaphoreGive(ota_mutex);
    xSemaphoreTake(state_mutex,portMAX_DELAY);state.connected=true;state.connection_id++;state.revision++;xSemaphoreGive(state_mutex);
    status("FM-1 connected; reading identity and voice");
}

static bool send_frame(const uint8_t *frame,size_t n)
{
    tx_is_fx=false;
    size_t size=fm1_usb_packets(frame,n,tx->data_buffer,1536);
    if(!size) return false;
    tx->num_bytes=size;
    esp_err_t err=usb_host_transfer_submit(tx);
    if(err!=ESP_OK) {status("USB send failed");return false;}
    xSemaphoreTake(state_mutex,portMAX_DELAY);state.tx_frames++;xSemaphoreGive(state_mutex);
    tx_busy=true;next_send=esp_timer_get_time()+80000;return true;
}

static void host_task(void *arg)
{
    (void)arg;
    usb_host_client_config_t cfg={.is_synchronous=false,.max_num_event_msg=8,
        .async={.client_event_callback=device_event,.callback_arg=NULL}};
    ESP_ERROR_CHECK(usb_host_client_register(&cfg,&client));
    const esp_timer_create_args_t timer={.callback=ota_wake,.name="fm1_ota_wake"};
    ESP_ERROR_CHECK(esp_timer_create(&timer,&ota_wake_timer));
    for(;;) {
        wait_host_events();
        if(remote_active.ticket&&esp_timer_get_time()>=remote_active.deadline){
            remote_complete(NULL,-1);remote_quiet_until=esp_timer_get_time()+2000000;
        }
        if(closing && device) close_device();
        if(open_address && !device) {uint8_t a=open_address;open_address=0;open_device(a);}
        uint8_t update_frame[FM1_FRAME_MAX];size_t update_n=0;
        /* Snapshot before taking ota_mutex, matching receive_frame's lock order.
         * Recheck the admitted session when the host consumes the queued start. */
        fm1_state_t start_device;fm1_host_snapshot(&start_device);
        xSemaphoreTake(ota_mutex,portMAX_DELAY);
        if(ota_start_pending&&!tx_busy) {
            ota_start_pending=false;pending=false;
            fm1_ota_begin(&ota,ota_image,ota_size,ota_source,ota_target,esp_timer_get_time()/1000);
            if(!device||closing||!start_device.connected||start_device.connection_id!=ota_expected_connection||strcmp(start_device.identity,ota_source))
                fm1_ota_fail(&ota,"USB connection changed before update started; prepare again");
        }
        bool update_active=fm1_ota_active(&ota);
        if(update_active)update_n=fm1_ota_next(&ota,update_frame,sizeof(update_frame),esp_timer_get_time()/1000);
        xSemaphoreGive(ota_mutex);
        if(update_n) {
            tx_is_ota=true;
            if(!device||closing||!send_frame(update_frame,update_n)) {
                tx_is_ota=false;xSemaphoreTake(ota_mutex,portMAX_DELAY);fm1_ota_sent(&ota,false,esp_timer_get_time()/1000);xSemaphoreGive(ota_mutex);
            }
        }
        if(update_active){remote_complete(NULL,-1);continue;}
        if(!device || closing || tx_busy) continue;
        fm1_state_t snapshot;fm1_host_snapshot(&snapshot);if(snapshot.updating){remote_complete(NULL,-1);continue;}
        int64_t now=esp_timer_get_time();
        xSemaphoreTake(state_mutex,portMAX_DELAY);
        bool pulse=state.clock_enabled&&now>=clock_next;
        if(pulse){int64_t period=2500000/state.clock_bpm;uint32_t late=(now-clock_next)/period;
          state.clock_missed+=late;clock_next+=(late+1)*period;}
        xSemaphoreGive(state_mutex);
        if(pulse){
          tx->data_buffer[0]=0x0f;tx->data_buffer[1]=0xf8;tx->data_buffer[2]=tx->data_buffer[3]=0;tx->num_bytes=4;
          bool sent=usb_host_transfer_submit(tx)==ESP_OK;
          xSemaphoreTake(state_mutex,portMAX_DELAY);if(sent){state.clock_pulses++;state.tx_frames++;}else{state.clock_enabled=false;state.clock_missed++;}xSemaphoreGive(state_mutex);
          if(sent)tx_busy=true;
          continue;
        }
        if(now<next_send) continue;
        uint8_t frame[32];size_t n;
        if(need_identity || (!snapshot.identity[0]&&now>=next_poll)) {
            n=fm1_identity_request(frame,sizeof(frame));need_identity=false;send_frame(frame,n);next_poll=now+1000000;continue;
        }
        if(remote_active.ticket)continue;
        if(!pending&&now>=remote_quiet_until&&xQueueReceive(remote_requests,&remote_active,0)==pdTRUE){
            if(remote_active.deadline<=now||remote_active.connection_id!=snapshot.connection_id||strcmp(remote_active.identity,snapshot.identity)){
                remote_complete(NULL,-1);continue;
            }
            bool sent=false;
            if(remote_active.data[0]==0xf0)sent=send_frame(remote_active.data,remote_active.size);
            else if(fm1_midi_packet(remote_active.data,remote_active.size,tx->data_buffer)){
                tx->num_bytes=4;
                sent=usb_host_transfer_submit(tx)==ESP_OK;
                if(sent){tx_busy=true;next_send=now+1000;xSemaphoreTake(state_mutex,portMAX_DELAY);state.tx_frames++;xSemaphoreGive(state_mutex);}
            }
            tx_is_remote=sent;if(!sent)remote_complete(NULL,-1);continue;
        }
        if(pending && now-pending_since>2000000) {
            pending=false;xSemaphoreTake(state_mutex,portMAX_DELAY);state.writes_failed++;xSemaphoreGive(state_mutex);
            status("Edit readback timed out");
        }
        command_t cmd;
        if(!pending && xQueueReceive(commands,&cmd,0)==pdTRUE) {
            if(cmd.read) next_poll=0;
            else {
                fm1_state_t current;fm1_host_snapshot(&current);
                if(!current.valid_voice || !current.can_edit || now-current.last_read_us>=3000000) continue;
                if(cmd.fx) {
                    if(cmd.generation!=current.fx_config_revision ||
                       !fm1_cc_packet(cmd.channel,cmd.parameter,cmd.value,cmd.scale,tx->data_buffer))continue;
                    tx->num_bytes=4;tx_is_fx=true;tx_fx=cmd;
                    if(usb_host_transfer_submit(tx)==ESP_OK) {
                        tx_busy=true;next_send=now+80000;
                        xSemaphoreTake(state_mutex,portMAX_DELAY);state.tx_frames++;xSemaphoreGive(state_mutex);
                        status("Effect sent - no device readback");
                    } else {tx_is_fx=false;status("Effect USB send failed");}
                    continue;
                }
                n=fm1_parameter_request(cmd.parameter,cmd.value,0,frame,sizeof(frame));
                if(send_frame(frame,n)) {
                    pending=true;pending_parameter=cmd.parameter;pending_value=cmd.value;
                    pending_since=now;next_poll=0;status("Sent edit; awaiting readback");
                }
                continue;
            }
        }
        if(now>=next_poll && snapshot.can_edit) {
            n=fm1_voice_request(frame,sizeof(frame));send_frame(frame,n);next_poll=now+1000000;
        }
    }
}

esp_err_t fm1_host_start(void)
{
    state_mutex=xSemaphoreCreateMutex();ota_mutex=xSemaphoreCreateMutex();commands=xQueueCreate(8,sizeof(command_t));
    remote_guard=xSemaphoreCreateMutex();remote_requests=xQueueCreate(1,sizeof(remote_command_t));remote_results=xQueueCreate(1,sizeof(remote_result_t));
    if(!state_mutex || !ota_mutex || !commands||!remote_guard||!remote_requests||!remote_results) return ESP_ERR_NO_MEM;
    state.fx_channel=1;state.fx_scale=10;
    status("Connect FM-1 to the Tab5 USB-A port");
    esp_err_t result=bsp_usb_host_start(BSP_USB_HOST_POWER_MODE_USB_DEV,true);
    if(result!=ESP_OK) return result;
    return xTaskCreate(host_task,"fm1_midi",12288,NULL,5,NULL)==pdPASS?ESP_OK:ESP_ERR_NO_MEM;
}
