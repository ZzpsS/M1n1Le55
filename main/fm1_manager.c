#include "fm1_ui.h"
#include "m1_theme.h"
#include "m1_firmware_icons.h"
#include "fm1_storage.h"
#include "fm1_host.h"
#include "fm1_ble.h"
#include "fm1_samples.h"
#include "fm1_update.h"
#include "fm1_package.h"
#include "esp_timer.h"
#include <stdio.h>
#include <string.h>

static lv_obj_t *work_label(lv_obj_t *p,const char *s,int x,int y,int w,const lv_font_t *font,uint32_t color)
{return m1_label(p,s,m1_wide(x),y,m1_wide(w),font,color);}
static lv_obj_t *work_button(lv_obj_t *p,const char *s,int x,int y,int w,int h,lv_event_cb_t cb,uintptr_t data,bool primary)
{return m1_button(p,s,m1_wide(x),y,m1_wide(w),h,cb,data,primary);}
static lv_obj_t *work_panel(lv_obj_t *p,int x,int y,int w,int h,bool scroll)
{return m1_panel(p,m1_wide(x),y,m1_wide(w),h,scroll);}
static void work_rule(lv_obj_t *p,int x,int y,int w){m1_rule(p,m1_wide(x),y,m1_wide(w));}

static lv_obj_t *panel,*list,*progress_bar,*progress_label,*detail_label;
static fm1_storage_state_t sd;
static fm1_state_t device;
static fm1_ble_state_t ble;
static fm1_samples_state_t samples;
static fm1_update_state_t update;
static unsigned section=3,selected,slot;
static fm1_manager_mode_t mode=FM1_MANAGER_DEVICE;
static bool remote,library,technical,experimental,reset_scroll,scan_requested,reveal_selected,sample_full_readback,recovery_file_selection;
static char selected_name[64];
static uint32_t sample_token,install_token,last_key,dismissed_error,completed_token;
static int list_scroll;
enum { A_SCAN=1,A_CONNECT,A_DISCONNECT,A_TEST,A_FWSCAN,A_PREPARE,A_CONFIRM,A_CANCEL,A_DISMISS,
    A_VOICE_BACKUP,A_LOCAL,A_REMOTE,A_FILES,A_BACK,A_SAMPLE_SCAN,A_SAMPLE_PREPARE,A_SAMPLE_CONFIRM,
    A_SAMPLE_CANCEL,A_SAMPLE_BACKUP,A_SAMPLE_PLAY,A_RESTORE,A_TECH,A_EXPERIMENT,A_SAMPLES,A_ERROR_BACK,A_SAMPLE_READBACK,A_RECOVERY_FILES,A_RECOVERY_LAST,
    A_TAB=100,A_BLE=200,A_FILE=300,A_FW=400,A_SLOT=500 };
static void render(void);
static void action(lv_event_t *e);
static lv_obj_t *text(const char *s,int x,int y,int width,uint32_t color)
{return work_label(panel,s,x,y,width,NULL,color);}
static lv_obj_t *button(const char *s,int x,int y,int width,int height,unsigned id,bool primary)
{return work_button(panel,s,x,y,width,height,action,id,primary);}
static void accepted(bool ok)
{fm1_ui_notice(ok?"请求已受理。":"请求未受理：请检查连接、SD 或正在进行的任务。");}
static bool installing(void){return update.stage>=UPDATE_PREPARING&&update.stage<=UPDATE_RUNNING;}
static bool recovery_file(void){return recovery_file_selection||!strncmp(samples.name,"backup-",7);}
static bool backup_ready(void)
{return !device.updating&&device.connected&&device.can_edit&&device.valid_voice&&esp_timer_get_time()-device.last_read_us<3000000;}
static void snapshots(void)
{fm1_storage_snapshot(&sd);fm1_host_snapshot(&device);fm1_ble_snapshot(&ble);fm1_samples_snapshot(&samples);fm1_update_snapshot(&update);}
static uint32_t key(void)
{
    /* Packet counters and byte progress must never recreate the object tree. */
    uint32_t values[]={sd.revision,update.stage,update.phase,update.step,update.token,samples.revision,
        ble.phase,ble.connection_id,ble.scan_id,ble.count,device.connected,device.connection_id,device.can_edit,backup_ready(),
        (!samples.busy&&!installing())?(ble.pending|ble.testing<<1|ble.reserved<<2):0,
        samples.confirmation_valid,samples.device_known};
    uint32_t h=2166136261u;for(unsigned i=0;i<sizeof(values)/sizeof(values[0]);i++)h=(h^values[i])*16777619u;
    for(unsigned i=0;device.identity[i];i++)h=(h^(uint8_t)device.identity[i])*16777619u;
    return h;
}
static void action(lv_event_t *e)
{
    unsigned a=(uintptr_t)lv_event_get_user_data(e);bool ok=true,feedback=false;
    if(a>=A_SLOT&&a<A_SLOT+8){slot=a-A_SLOT;render();return;}
    if(a>=A_FW&&a<A_FW+FM1_LIBRARY_MAX){selected=a-A_FW;if(selected<sd.count)snprintf(selected_name,sizeof(selected_name),"%s",sd.files[selected].name);technical=false;render();return;}
    if(a>=A_FILE&&a<A_FILE+FM1_SAMPLE_FILES){unsigned i=a-A_FILE;if(i<samples.count){ok=fm1_samples_preview(samples.files[i]);if(ok){library=false;remote=false;}accepted(ok);}return;}
    if(a>=A_BLE&&a<A_BLE+FM1_BLE_DEVICES){accepted(fm1_ble_connect(a-A_BLE,ble.scan_id));return;}
    if(a>=A_TAB&&a<A_TAB+4){technical=false;reset_scroll=true;fm1_manager_section(a-A_TAB);return;}
    switch(a){
    case A_SCAN:ok=fm1_ble_scan();feedback=true;break;
    case A_DISCONNECT:ok=fm1_ble_disconnect();feedback=true;break;
    case A_TEST:ok=fm1_ble_test_note();feedback=true;break;
    case A_CONNECT:fm1_manager_section(3);return;
    case A_FWSCAN:ok=fm1_storage_request(false);feedback=true;break;
    case A_PREPARE:ok=selected<sd.count&&fm1_update_install(sd.files[selected].name);feedback=true;break;
    case A_CONFIRM:
        fm1_host_snapshot(&device);fm1_update_snapshot(&update);
        ok=device.connected&&device.connection_id==update.source_connection&&!strcmp(device.identity,update.source)&&fm1_update_confirm(install_token);feedback=true;break;
    case A_CANCEL:ok=fm1_update_cancel(install_token);feedback=true;break;
    case A_DISMISS:ok=fm1_update_dismiss();feedback=true;break;
    case A_VOICE_BACKUP:ok=fm1_storage_request(true);feedback=true;break;
    case A_LOCAL:remote=false;library=false;render();return;
    case A_REMOTE:remote=true;library=false;render();return;
    case A_FILES:recovery_file_selection=false;library=true;reset_scroll=true;render();return;
    case A_RECOVERY_FILES:recovery_file_selection=true;dismissed_error=samples.revision;remote=false;library=true;reset_scroll=true;fm1_ui_page(7,0);render();return;
    case A_RECOVERY_LAST:ok=samples.result_backup[0]&&fm1_samples_preview(samples.result_backup);if(ok){recovery_file_selection=true;remote=false;library=false;}feedback=true;break;
    case A_BACK:library=false;render();return;
    case A_SAMPLE_SCAN:ok=fm1_samples_scan();feedback=true;break;
    case A_SAMPLE_PREPARE:ok=samples.name[0]&&(recovery_file()?fm1_samples_prepare_restore(samples.name):fm1_samples_prepare(samples.name));feedback=true;break;
    case A_SAMPLE_CONFIRM:ok=fm1_samples_confirm_with_readback(sample_token,sample_full_readback);feedback=true;break;
    case A_SAMPLE_READBACK:sample_full_readback=!sample_full_readback;render();return;
    case A_SAMPLE_CANCEL:ok=fm1_samples_cancel(sample_token);feedback=true;break;
    case A_SAMPLE_BACKUP:ok=fm1_samples_backup();feedback=true;break;
    case A_SAMPLE_PLAY:ok=fm1_samples_play(slot);feedback=true;break;
    case A_RESTORE:fm1_samples_snapshot(&samples);ok=samples.result_backup_current&&fm1_samples_prepare_recent_restore(samples.result_backup_connection_id);if(ok){remote=false;library=false;fm1_ui_page(7,0);}feedback=true;break;
    case A_TECH:technical=!technical;render();return;
    case A_EXPERIMENT:experimental=!experimental;reset_scroll=true;render();return;
    case A_SAMPLES:fm1_ui_page(7,0);return;
    case A_ERROR_BACK:dismissed_error=samples.revision;render();return;
    default:return;
    }
    if(feedback)accepted(ok);
}
static const char *phase_name(fm1_sample_phase_t phase)
{
    switch(phase){case SAMPLE_LOADING:return "读取 SD 采样包";case SAMPLE_QUERY:return "检查设备与协议";
    case SAMPLE_BACKUP:return "备份设备旧库";case SAMPLE_WRITING:return "写入新采样库";case SAMPLE_VERIFYING:return "完整读回核验";
    case SAMPLE_PLAYING:return "发送设备试听";case SAMPLE_PREPARED:return "确认替换";case SAMPLE_ERROR:return "任务未完成";
    case SAMPLE_DONE:return "任务已完成";default:return "采样预览";}
}
static void bar(int top,uint32_t color)
{
    progress_bar=lv_bar_create(panel);lv_obj_set_pos(progress_bar,0,top);lv_obj_set_size(progress_bar,m1_wide(1036),16);lv_bar_set_range(progress_bar,0,100);
    lv_obj_set_style_radius(progress_bar,0,LV_PART_MAIN);lv_obj_set_style_radius(progress_bar,0,LV_PART_INDICATOR);
    lv_obj_set_style_bg_color(progress_bar,lv_color_hex(M1_SURFACE),LV_PART_MAIN);lv_obj_set_style_bg_color(progress_bar,lv_color_hex(color),LV_PART_INDICATOR);
    progress_label=text("",0,top+32,1036,M1_MUTED);
}
static void sample_task(void)
{
    if(samples.prepared&&!samples.busy){
        if(sample_token!=samples.token)sample_full_readback=false;
        sample_token=samples.token;work_label(panel,samples.recovery?"从文件恢复全部八槽。":"替换全部八槽。",0,100,1036,&m1_font_28,M1_FG);
        text(samples.name,0,164,1036,M1_YELLOW);char info[180];snprintf(info,sizeof(info),"目标蓝牙设备：%s",samples.peer[0]?samples.peer:"未知");text(info,0,214,1036,M1_MUTED);
        text(samples.recovery?"有效旧库仍备份；损坏库由所选文件恢复。":sample_full_readback?"先备份旧库，写入后逐字节读回。":"先备份旧库，设备提交成功后完成。",0,270,690,M1_MUTED);
        lv_obj_t *choice=button(sample_full_readback?"完整读回：开":"完整读回：关",700,254,336,72,A_SAMPLE_READBACK,false);m1_selected(choice,sample_full_readback);
        text(samples.confirmation_valid?"写入期间不可取消；中断不会自动回滚。":"连接已变化：请取消并重新准备，旧确认不可继续。",0,350,1036,M1_YELLOW);
        button("返回",0,448,240,88,A_SAMPLE_CANCEL,false);
        lv_obj_t *b=button(samples.recovery?"开始恢复":"开始替换",700,448,336,88,A_SAMPLE_CONFIRM,true);m1_enabled(b,samples.confirmation_valid&&sd.mounted);return;
    }
    work_label(panel,phase_name(samples.phase),0,100,1036,&m1_font_28,M1_FG);
    const char *steps[]={"01 准备","02 备份","03 写入",samples.full_readback?"04 完整读回":"04 提交确认"};
    unsigned current=samples.phase==SAMPLE_BACKUP?1:samples.phase==SAMPLE_WRITING?2:samples.phase==SAMPLE_VERIFYING?3:0;
    for(unsigned i=0;i<4;i++){text(steps[i],i*264,188,248,i==current?M1_FG:M1_DIM);work_rule(panel,i*264,232,240);}
    bar(278,M1_YELLOW);
    text("保持连接与供电。可以切换页面，任务仍在后台运行。",0,388,1036,M1_MUTED);
}
static void wave_draw(lv_event_t *e)
{
    lv_obj_t *obj=lv_event_get_target_obj(e);lv_layer_t *layer=lv_event_get_layer(e);lv_area_t bounds;lv_obj_get_coords(obj,&bounds);
    const uint8_t *p=remote?samples.device_peaks[slot]:samples.peaks[slot];
    bool known=remote?samples.device_known:samples.bytes>0;if(!known)return;
    lv_draw_rect_dsc_t d;lv_draw_rect_dsc_init(&d);d.bg_color=lv_color_hex(M1_YELLOW);d.bg_opa=LV_OPA_COVER;d.radius=0;
    for(unsigned i=0;i<64;i++){unsigned height=p[i]*8u/255u;for(int y=-(int)height;y<=(int)height;y++){
        lv_area_t r={bounds.x1+(int)i*(bounds.x2-bounds.x1+1)/64,bounds.y1+56+y*6,bounds.x1+(int)i*(bounds.x2-bounds.x1+1)/64+5,bounds.y1+60+y*6};lv_draw_rect(layer,&d,&r);
    }}
}
static void sample_library(void)
{
    button("返回预览",0,88,228,72,A_BACK,false);lv_obj_t *scan=button("刷新列表",796,88,240,72,A_SAMPLE_SCAN,false);m1_enabled(scan,!samples.busy&&!samples.prepared&&sd.mounted);
    char summary[160];snprintf(summary,sizeof(summary),"SD 采样包  /  %u 个%s",samples.count+samples.omitted,samples.omitted?" · 列表显示前 24 个；恢复时保留最近备份入口":"");text(summary,0,180,1036,M1_MUTED);
    list=work_panel(panel,0,232,1048,308,true);lv_obj_t *saved=panel;panel=list;
    unsigned row=0;
    if(recovery_file_selection&&samples.result_backup[0]){
        char caption[112];snprintf(caption,sizeof(caption),"%s / %s",samples.result_backup_current?"最近备份":"上次连接",samples.result_backup);
        lv_obj_t *b=button(caption,0,row++*88,1024,80,A_RECOVERY_LAST,true);m1_enabled(b,!samples.busy&&!samples.prepared&&sd.mounted);
    }
    for(unsigned i=0;i<samples.count;i++){if(row&&recovery_file_selection&&!strcmp(samples.files[i],samples.result_backup))continue;lv_obj_t *b=button(samples.files[i],0,row++*88,1024,80,A_FILE+i,false);m1_enabled(b,!samples.busy&&!samples.prepared);}
    if(!row){
        text(sd.mounted?"将 .fm1s 文件放入 SD 的 /fm1/samples/，然后刷新。":"关机后插入 FAT32 存储卡，再开机。",0,20,1000,M1_MUTED);
        text("格式转换与准备步骤见随包新手指南；不能直接放入 WAV。",0,96,1000,M1_MUTED);
    }
    panel=saved;lv_obj_scroll_to_y(list,list_scroll,LV_ANIM_OFF);
}
static void samples_panel(void)
{
    work_label(panel,"Open FM1S 采样",0,0,470,&m1_font_28,M1_FG);
    if(samples.prepared||samples.busy){sample_task();return;}
    if(samples.phase==SAMPLE_ERROR&&dismissed_error!=samples.revision){
        work_label(panel,"任务未完成",0,104,1036,&m1_font_28,M1_FG);
        text(phase_name(samples.failed_phase),0,164,1036,M1_YELLOW);
        text(samples.failed_phase==SAMPLE_WRITING||samples.failed_phase==SAMPLE_VERIFYING?"设备采样库可能不完整。可从备份恢复；重连后需重新选择恢复文件。":samples.failed_phase==SAMPLE_QUERY||samples.failed_phase==SAMPLE_PLAYING?"需要兼容 Open FM1S 的蓝牙采样协议。\n蓝牙连接不代表支持；原厂请使用音色编辑。":"请检查蓝牙、SD 和采样文件，再重新准备。",0,228,1036,M1_MUTED);
        text(samples.message,0,310,1036,M1_MUTED);
        text(samples.result_backup[0]?samples.result_backup:"本次没有可用的恢复备份。",0,402,1036,M1_YELLOW);
        button("返回采样",0,448,264,88,A_ERROR_BACK,false);
        if(samples.result_backup_current){lv_obj_t *restore=button("准备恢复备份",700,448,336,88,A_RESTORE,true);m1_enabled(restore,ble.phase==FM1_BLE_READY&&sd.mounted);}
        else if(sd.mounted)button("选择恢复文件",700,448,336,88,A_RECOVERY_FILES,true);
        else button("查看连接",700,448,336,88,A_CONNECT,true);
        return;
    }
    lv_obj_t *b=button("SD 采样包",500,0,252,64,A_LOCAL,false);m1_selected(b,!remote);
    b=button("设备槽位",776,0,260,64,A_REMOTE,false);m1_selected(b,remote);
    if(library){sample_library();return;}
    bool known=remote?samples.device_known:samples.bytes>0;
    text(remote?(known?(samples.device_verified?"已完整读回的设备库":"刚发送的采样内容 · 设备已确认提交"):"蓝牙连接不代表支持；需 Open FM1S 协议。\n读取并备份后可显示设备名称与波形。"):"本地文件预览 · 设备写入需 Open FM1S 蓝牙协议",0,90,770,M1_MUTED);
    button(remote?"SD 采样包":"选择文件",800,78,236,72,A_FILES,false);
    const char *name=remote?(known?samples.device_slots[slot]:"UNKNOWN"):samples.slots[slot];
    work_label(panel,known?(name[0]?name:"EMPTY"):"SELECT BANK",0,158,1036,&m1_pixel_48,M1_FG);
    lv_obj_t *wave=work_panel(panel,0,204,1036,112,false);lv_obj_add_event_cb(wave,wave_draw,LV_EVENT_DRAW_MAIN,NULL);
    char meta[180];unsigned frames=remote?samples.device_frames[slot]:samples.frames[slot];
    if(known)snprintf(meta,sizeof(meta),"%02u / 08    %.3f s    32 kHz    /    %s",slot+1,frames/32000.0,remote?samples.device_name:samples.name);
    else snprintf(meta,sizeof(meta),"%02u / 08    /    %s",slot+1,remote?"试听设备当前槽位":"选择 SD 采样包后预览");
    lv_obj_t *metadata=text(meta,0,324,1036,M1_MUTED);lv_obj_set_height(metadata,36);lv_label_set_long_mode(metadata,LV_LABEL_LONG_DOT);
    for(unsigned i=0;i<8;i++){
        char caption[48];const char *n=remote?samples.device_slots[i]:samples.slots[i];snprintf(caption,sizeof(caption),"%02u\n%.10s",i+1,known?n:"--");
        lv_obj_t *p=button(caption,i*132,366,116,80,A_SLOT+i,false);m1_selected(p,i==slot);lv_obj_set_style_text_font(lv_obj_get_child(p,0),&lv_font_montserrat_16,0);
    }
    if(remote){
        b=button("读取并备份",0,462,264,88,A_SAMPLE_BACKUP,false);m1_enabled(b,ble.phase==FM1_BLE_READY&&sd.mounted&&!installing());
        work_label(panel,"Open FM1S 协议限定\n蓝牙连接不代表支持",284,474,420,&m1_font_20,M1_MUTED);
        b=button(ble.phase==FM1_BLE_READY?"试听当前槽位":"连接蓝牙",728,462,308,88,ble.phase==FM1_BLE_READY?A_SAMPLE_PLAY:A_CONNECT,true);
        m1_enabled(b,!installing());
    }else{
        text(recovery_file()?"从所选文件恢复；确认目标设备后开始。":"替换需 Open FM1S 蓝牙协议；先备份旧库。",0,480,690,M1_MUTED);
        b=button(ble.phase==FM1_BLE_READY?(recovery_file()?"恢复设备采样库":"替换设备采样库"):"连接蓝牙",728,462,308,88,ble.phase==FM1_BLE_READY?A_SAMPLE_PREPARE:A_CONNECT,true);
        m1_enabled(b,!installing()&&(ble.phase!=FM1_BLE_READY||(known&&sd.mounted)));
    }
}
static void connections_panel(void)
{
    text("USB",0,100,260,M1_FG);text(device.connected?device.identity:"未连接",290,100,720,M1_MUTED);work_rule(panel,0,146,1036);
    text("蓝牙 MIDI",0,180,260,M1_FG);text(ble.peer[0]?ble.peer:"未连接",290,180,720,M1_MUTED);
    bool free=!ble.pending&&!ble.testing&&!ble.reserved&&!installing();
    lv_obj_t *b=button("扫描设备",0,240,232,80,A_SCAN,false);m1_enabled(b,free&&(ble.phase==FM1_BLE_OFF||ble.phase==FM1_BLE_IDLE||ble.phase==FM1_BLE_ERROR));
    b=button("断开 / 停止",252,240,232,80,A_DISCONNECT,false);m1_enabled(b,free&&ble.phase!=FM1_BLE_OFF&&ble.phase!=FM1_BLE_STARTING);
    b=button("测试音符",504,240,232,80,A_TEST,false);m1_enabled(b,free&&ble.phase==FM1_BLE_READY);
    button(technical?"隐藏详情":"技术详情",788,240,248,80,A_TECH,false);
    if(technical){
        char s[360];snprintf(s,sizeof(s),"%s\nC6: %s\nTX %lu  /  RX %lu  /  %lu bytes\n%s",ble.message,ble.coprocessor,(unsigned long)ble.tx_packets,(unsigned long)ble.rx_packets,(unsigned long)ble.rx_bytes,ble.last_rx);
        detail_label=text(s,0,352,1036,M1_MUTED);return;
    }
    list=work_panel(panel,0,348,1048,196,true);lv_obj_t *saved=panel;panel=list;
    for(unsigned i=0;i<ble.count;i++){
        char s[100];snprintf(s,sizeof(s),"%s   /   %d dBm%s",ble.devices[i].name,ble.devices[i].rssi,ble.devices[i].midi?"   MIDI":"");
        b=button(s,0,i*88,1024,80,A_BLE+i,false);m1_enabled(b,free&&(ble.phase==FM1_BLE_SCANNING||ble.phase==FM1_BLE_IDLE));
    }
    if(!ble.count)text(ble.phase==FM1_BLE_READY?"已连接。测试音符的声音由 FM-1 输出。\n蓝牙连接不代表支持采样库；固件安装使用 USB。":ble.phase==FM1_BLE_ERROR?"连接失败：先断开再扫描；仍失败时在 FM-1 重开蓝牙。":"开启 FM-1 蓝牙 MIDI，然后扫描并选择设备。",0,20,1000,M1_MUTED);
    panel=saved;lv_obj_scroll_to_y(list,list_scroll,LV_ANIM_OFF);
}
static bool experimental_package(const fm1_package_t *p)
{return !strcmp(p->identity,"FM-1_8000912")||!strcmp(p->identity,"FM-1_8000913");}
static void firmware_name(const char *id,char *out,size_t capacity)
{
    /* Display names only: installation eligibility still comes from the pinned
     * package profile, never this identity-to-label mapping. */
    static const struct {const char *id,*name;} names[]={
        {"FM-1_015","M-VAVE 官方 V15"},{"FM-1_916","Felucca 1.6.1"},
        {"FM-1_900","SLOOP 2.7"},{"FM-1_101","FM-1+VA 101"},
        {"FM-1_8000400","FoMni 0.4"},{"FM-1_9010100","X0X 1.1.0"},
        {"FM-1_910","Jangada 1.0"},{"FM-1_8000012","FM1 Open V12"},
        {"FM-1_8000912","BLE 实验 V912"},{"FM-1_8000913","BLE 诊断 V913"}
    };
    for(unsigned i=0;i<sizeof(names)/sizeof(names[0]);i++)if(!strcmp(id,names[i].id)){snprintf(out,capacity,"%s",names[i].name);return;}
    unsigned version;char extra;
    if(strlen(id)==8&&sscanf(id,"FM-1_%u%c",&version,&extra)==1&&version<=15)
        snprintf(out,capacity,"M-VAVE 原厂 V%u",version);
    else if(fm1_ota_loader(id))snprintf(out,capacity,"恢复加载器 / %s",id);
    else snprintf(out,capacity,"%s",id[0]?id:"等待识别");
}
static const char *install_block_text(const char *block)
{
    if(!block)return NULL;
    if(!strcmp(block,"Package has no supported install profile"))return "此文件没有受支持的安装配置。";
    if(!strcmp(block,"Connect an identified FM-1 first"))return "请先通过 USB 连接并识别 FM-1。";
    if(!strcmp(block,"Loader differs: finish with its matching firmware package"))return "恢复加载器不匹配，请选择兼容的恢复包。";
    if(!strcmp(block,"Current firmware has no supported install path"))return "当前固件没有受支持的安装路径。";
    if(!strcmp(block,"Official V15 is already running"))return "已运行官方 V15，无需重复安装。";
    return block;
}
static void install_panel(void)
{
    install_token=update.token;
    work_label(panel,update.title[0]?update.title:update.name,0,90,1036,&m1_font_28,M1_FG);
    char source[80],target[80],s[256];firmware_name(update.source,source,sizeof(source));firmware_name(update.target,target,sizeof(target));
    snprintf(s,sizeof(s),update.steps==2?"%s  >  官方 V15  >  %s":"%s  >  %s",source,target);
    if(update.source[0])work_label(panel,s,0,150,1036,&m1_font_20,M1_MUTED);
    if(update.stage==UPDATE_READY){
        bool same_device=device.connected&&device.connection_id==update.source_connection&&!strcmp(device.identity,update.source);
        text("手动准备已完成 · 等待安装",0,216,1036,M1_ORANGE);
        text(update.backup[0]?"当前原厂音色已保存；不包含预设库、效果或工程。":"没有当前原厂音色备份；自定义工程需提前自行导出。",0,268,1036,M1_MUTED);
        if(update.backup[0]){snprintf(s,sizeof(s),"音色副本：%s",update.backup);work_label(panel,s,0,318,1036,&m1_font_20,M1_MUTED);}
        text(!same_device?"USB 目标已变化，请取消准备并重新选择。":update.steps==2?"将自动经过 V15 安装所选固件。\n保持 USB 连接与供电。":"安装将重启 FM-1，请保持 USB 连接与供电。",0,366,1036,!same_device?M1_ORANGE:M1_MUTED);
        button("取消准备",0,462,264,88,A_CANCEL,false);
        lv_obj_t *b=button("安装并核对身份",676,462,360,88,A_CONFIRM,true);m1_enabled(b,same_device);
    }else if(update.stage==UPDATE_RUNNING){
        text(update.steps==2?(update.step==1?"正在安装 · 自动 V15 中转":"正在安装 · 所选固件"):"正在安装",0,216,1036,M1_ORANGE);bar(286,M1_ORANGE);
        text("安装后自动重启。保持 USB 连接与供电。",0,388,1036,M1_MUTED);
        text("可以切换页面；安装任务会继续。",0,454,1036,M1_MUTED);
    }else if(update.stage==UPDATE_PREPARING){
        text("正在准备安装",0,226,1036,M1_ORANGE);
        text(update.automatic?"检查完成后自动安装。请保持 USB 连接与供电。":"正在准备诊断请求；完成后可手动安装或取消。",0,294,1036,M1_MUTED);
    }else {
        text(update.stage==UPDATE_DONE?"安装完成":update.steps==2?(update.step==1?"V15 中转未完成 · 已停止":"目标固件未完成 · 已停止"):"安装未完成",0,216,1036,update.stage==UPDATE_DONE?M1_GREEN:M1_ORANGE);
        work_label(panel,update.stage==UPDATE_DONE?(strstr(update.message,"log failed")?"固件已启动；SD 安装记录未能保存。":"新固件已启动，可以进入工作台。"):update.message,0,274,1036,&m1_font_20,M1_MUTED);
        snprintf(s,sizeof(s),"用时 %.1f 秒",update.elapsed_ms/1000.0);
        work_label(panel,s,0,340,1036,&m1_font_20,M1_MUTED);
        text(update.stage==UPDATE_DONE?(update.backup[0]?"安装前已保存当前原厂音色。":""):"如设备停在加载器，请保持供电并使用兼容包重试。",0,396,1036,M1_MUTED);
        button("返回固件库",0,462,336,88,A_DISMISS,false);
    }
}
static void firmware_scroll_end(lv_event_t *e)
{
    /* Snap to complete tile rows without LVGL's extra last-row padding. */
    lv_obj_t *grid=lv_event_get_target_obj(e);int maximum=(uintptr_t)lv_event_get_user_data(e);
    int position=lv_obj_get_scroll_y(grid),target=LV_CLAMP(0,((position+59)/118)*118,maximum);
    if(position!=target)lv_obj_scroll_to_y(grid,target,LV_ANIM_OFF);
}
static void firmware_panel(void)
{
    work_label(panel,"固件管理",0,0,300,&m1_font_28,M1_FG);
    if(update.stage!=UPDATE_IDLE){install_panel();return;}
    lv_obj_t *b=button("刷新 SD",330,0,164,64,A_FWSCAN,false);m1_enabled(b,sd.mounted&&!sd.busy&&!samples.busy&&!samples.prepared);
    b=button(experimental?"收起实验固件":"显示实验固件",506,0,264,64,A_EXPERIMENT,false);m1_selected(b,experimental);
    lv_obj_t *install=button("安装该固件",782,0,254,64,A_PREPARE,true);m1_enabled(install,false);
    char current_name[80],s[256];firmware_name(device.identity,current_name,sizeof(current_name));
    if(!sd.count){
        snprintf(s,sizeof(s),"当前运行  /  %s",device.connected?current_name:"USB 未连接");text(s,0,104,1036,M1_MUTED);
        text(sd.mounted?"固件库为空：将 .fwsc 与同名 .sha256 放入 SD。":"SD 未挂载：关机后插入 FAT32 卡，再开机。",0,224,1036,M1_MUTED);
        text(sd.mounted?"路径：/fm1/firmware/  ·  操作见随包新手指南。":"无需 SD 也可连接 USB，在工作台使用当前固件。",0,294,1036,M1_MUTED);
        return;
    }
    unsigned first=sd.count;
    if(!selected_name[0]&&device.connected){
        for(unsigned i=0;i<sd.count;i++)if(!strcmp(device.identity,sd.files[i].identity)&&
            (experimental||!experimental_package(&sd.files[i]))){
            selected=i;snprintf(selected_name,sizeof(selected_name),"%s",sd.files[i].name);reveal_selected=true;break;
        }
    }
    for(unsigned i=0;i<sd.count;i++)if(experimental||!experimental_package(&sd.files[i])){
        if(first==sd.count)first=i;
        if(selected_name[0]&&!strcmp(selected_name,sd.files[i].name))selected=i;
    }
    if(first==sd.count){text("当前只有实验包。开启“显示实验固件”后查看。",0,224,1036,M1_MUTED);return;}
    if(selected>=sd.count||(!experimental&&experimental_package(&sd.files[selected])))selected=first;
    snprintf(selected_name,sizeof(selected_name),"%s",sd.files[selected].name);
    list=work_panel(panel,0,88,454,462,true);lv_obj_t *saved=panel,*chosen=NULL;panel=list;unsigned row=0;
    for(unsigned i=0;i<sd.count;i++){
        fm1_package_t *p=&sd.files[i];bool exp=experimental_package(p);
        if(exp&&!experimental)continue;
        const fm1_release_t *r=fm1_release_find(p->sha256,p->size,p->identity);
        bool installed=device.connected&&!strcmp(device.identity,p->identity);
        char name[80];if(r)firmware_name(p->identity,name,sizeof(name));else snprintf(name,sizeof(name),"%s",p->name);
        b=button(name,(row%2)*226,(row/2)*118,214,108,A_FW+i,false);row++;
        lv_obj_set_snappable(b,true);m1_selected(b,i==selected);
        lv_obj_set_style_bg_color(b,lv_color_hex(i==selected?0x142440:0x111114),0);
        lv_obj_set_style_border_width(b,i==selected?2:1,0);
        lv_obj_t *title=lv_obj_get_child(b,0);lv_obj_set_style_text_font(title,&m1_font_20,0);
        lv_obj_set_align(title,LV_ALIGN_TOP_LEFT);lv_obj_set_pos(title,8,66);lv_obj_set_width(title,m1_wide(214)-16);lv_obj_set_height(title,30);
        lv_obj_t *icon=m1_firmware_icon(b,r?p->identity:"",48);lv_obj_align(icon,LV_ALIGN_TOP_MID,0,10);
        if(installed){lv_obj_t *dot=m1_panel(b,m1_wide(214)-18,12,6,6,false);lv_obj_set_clickable(dot,false);lv_obj_set_style_bg_opa(dot,LV_OPA_COVER,0);lv_obj_set_style_bg_color(dot,lv_color_hex(M1_GREEN),0);}
        if(exp)m1_label(b,"EXP",10,10,40,&lv_font_montserrat_14,M1_ORANGE);
        if(i==selected){chosen=b;lv_obj_set_style_border_color(b,lv_color_hex(M1_BLUE),0);}
    }
    panel=saved;
    if(row>8)lv_obj_add_event_cb(list,firmware_scroll_end,LV_EVENT_SCROLL_END,(void *)(uintptr_t)(((row+1)/2-4)*118));
    if(row<=8)list_scroll=0;
    lv_obj_scroll_to_y(list,list_scroll,LV_ANIM_OFF);
    if(reveal_selected&&chosen){if(row>8)lv_obj_scroll_to_view(chosen,LV_ANIM_OFF);reveal_selected=false;}
    fm1_package_t *p=&sd.files[selected];const fm1_release_t *release=fm1_release_find(p->sha256,p->size,p->identity);
    snprintf(s,sizeof(s),"当前运行\n%s",device.connected?current_name:"USB 未连接");
    lv_obj_t *current_label=work_label(panel,s,486,88,550,&m1_font_20,device.connected?M1_FG:M1_MUTED);lv_obj_set_height(current_label,64);lv_label_set_long_mode(current_label,LV_LABEL_LONG_DOT);
    snprintf(s,sizeof(s),"SD 固件库  /  %u 个%s  /  USB 安装",row,sd.omitted?" · 已达列表上限":"");work_label(panel,s,486,158,550,&m1_font_20,M1_MUTED);
    work_rule(panel,486,202,550);
    lv_obj_t *title=work_label(panel,release?release->title:p->name,486,224,550,&m1_font_28,M1_FG);lv_obj_set_height(title,68);lv_label_set_long_mode(title,LV_LABEL_LONG_DOT);
    const char *block=fm1_release_block(release,device.identity);
    bool ble_busy=ble.pending||ble.testing||ble.reserved||(ble.phase>=FM1_BLE_STARTING&&ble.phase<=FM1_BLE_READY&&ble.phase!=FM1_BLE_IDLE);
    const char *reason=!sd.mounted?"需要挂载 SD 卡。":sd.busy?"SD 正在忙碌，请稍候。":!device.connected?"需要 USB 连接。":samples.busy||samples.prepared?"先完成或取消采样任务。":ble_busy?"安装前先停止蓝牙任务并断开连接。":!p->hash_match?"文件摘要未匹配参考值。":install_block_text(block);
    bool bridge=fm1_release_needs_bridge(release,device.identity);
    bool installed=device.connected&&!strcmp(device.identity,p->identity);
    if(technical){
        snprintf(s,sizeof(s),"%s / %lu bytes\nSHA-256\n%.32s\n%s",p->identity,(unsigned long)p->size,p->sha256,p->sha256+32);
        lv_obj_t *t=text(s,486,300,550,M1_MUTED);lv_obj_set_style_text_font(t,&lv_font_montserrat_16,0);
        work_label(panel,reason?reason:installed?"身份匹配不等于设备镜像的完整摘要核对。":"参考摘要匹配；不是发布者数字签名。",486,394,550,&m1_font_20,reason?M1_ORANGE:M1_MUTED);
    }else{
        text(bridge?"自动 V15 中转  /  两步安装":installed?"已安装  /  当前身份匹配":"直接安装  /  USB",486,300,550,bridge?M1_ORANGE:installed?M1_GREEN:M1_MUTED);
        work_label(panel,reason?reason:bridge?"点击安装后自动经过 V15 完成安装。":device.can_edit?"点击即开始；会先保存当前原厂音色。":"点击安装后自动完成，保持 USB 连接与供电。",486,358,550,&m1_font_20,reason?M1_ORANGE:M1_MUTED);
    }
    button(technical?"隐藏校验详情":"技术与校验详情",486,486,300,64,A_TECH,false);
    lv_label_set_text(lv_obj_get_child(install,0),installed?"重新安装":"安装该固件");m1_enabled(install,!reason);
}
static void backups_panel(void)
{
    text("原厂当前音色",0,104,1036,M1_FG);
    text("保存 6 算子与全局参数；不包含预设库、效果和固件。",0,154,1036,M1_MUTED);
    lv_obj_t *b=button("保存音色副本",0,216,320,80,A_VOICE_BACKUP,false);m1_enabled(b,backup_ready()&&sd.mounted&&!sd.busy&&!installing());
    text(sd.last_backup[0]?sd.last_backup:"尚无音色副本 / 暂无触屏恢复",352,238,684,M1_MUTED);work_rule(panel,0,328,1036);
    text("Open FM1S 蓝牙采样库 · 全部八槽",0,358,1036,M1_FG);
    text(samples.result_backup[0]?samples.result_backup:"需兼容采样协议；蓝牙连接不代表支持。",0,402,1036,M1_MUTED);
    b=button("读取并备份",0,462,320,80,A_SAMPLE_BACKUP,false);m1_enabled(b,ble.phase==FM1_BLE_READY&&sd.mounted&&!samples.busy&&!samples.prepared&&!installing());
    b=button(samples.result_backup_current?"准备恢复最近备份":"选择恢复文件",352,462,352,80,samples.result_backup_current?A_RESTORE:A_RECOVERY_FILES,false);m1_enabled(b,sd.mounted&&ble.phase==FM1_BLE_READY&&!samples.busy&&!samples.prepared&&!installing());
    button("采样库",736,462,300,80,A_SAMPLES,false);
}
static void activity_panel(void)
{
    list=work_panel(panel,0,100,1048,444,true);lv_obj_t *saved=panel;panel=list;int y=0;
    if(samples.result[0]){text(samples.result_completed?(samples.result_verified?"最近采样任务：完成并已核验":"最近采样任务：已完成"):
        "最近采样任务：未完成",0,y,1024,M1_FG);y+=48;text(samples.result,0,y,1024,M1_MUTED);y+=88;
        if(samples.result_backup[0]){text(samples.result_backup,0,y,1024,M1_YELLOW);y+=56;}}
    for(unsigned i=0;i<6;i++)if(sd.history[i][0]){text(sd.history[i],0,y,1024,M1_MUTED);y+=88;}
    if(!y)text("暂无记录。核验结果和备份范围会保留在这里。",0,0,1024,M1_MUTED);
    panel=saved;lv_obj_scroll_to_y(list,list_scroll,LV_ANIM_OFF);
}
static void render(void)
{
    if(!panel)return;
    if(list&&!reset_scroll)list_scroll=lv_obj_get_scroll_y(list);
    if(reset_scroll)list_scroll=0;
    reset_scroll=false;
    lv_obj_clean(panel);list=progress_bar=progress_label=detail_label=NULL;snapshots();last_key=key();
    if(mode==FM1_MANAGER_SAMPLES){
        if(samples.operation==3&&samples.phase==SAMPLE_DONE&&samples.completed&&samples.token!=completed_token){completed_token=samples.token;remote=true;library=false;}
        samples_panel();return;
    }
    if(mode==FM1_MANAGER_FIRMWARE){firmware_panel();return;}
    work_label(panel,"设备",0,0,150,&m1_font_28,M1_FG);
    const unsigned order[]={3,1,2};const char *names[]={"连接","备份","记录"};
    for(unsigned i=0;i<3;i++){lv_obj_t *b=button(names[i],380+i*220,0,216,64,A_TAB+order[i],false);m1_selected(b,section==order[i]);}
    if(section==3)connections_panel();else if(section==1)backups_panel();else activity_panel();
}
void fm1_manager_close(void){panel=NULL;list=progress_bar=progress_label=detail_label=NULL;}
void fm1_manager_open(lv_obj_t *parent,fm1_manager_mode_t next_mode)
{
    panel=parent;mode=next_mode;list_scroll=0;snapshots();
    if(mode==FM1_MANAGER_DEVICE&&(section==0||section>3))section=3;
    if(mode==FM1_MANAGER_SAMPLES&&!scan_requested&&!samples.count&&!samples.busy&&!samples.prepared&&sd.mounted&&!sd.busy){scan_requested=fm1_samples_scan();}
    render();
}
bool fm1_manager_section(unsigned view)
{
    if(view>4)return false;
    if(view==4)return fm1_ui_page(7,0);
    if(view==0){
        if(!panel||mode!=FM1_MANAGER_FIRMWARE)fm1_ui_route(0);else render();
        return panel&&mode==FM1_MANAGER_FIRMWARE;
    }
    section=view;
    if(!panel||mode!=FM1_MANAGER_DEVICE)fm1_ui_route(3);else render();
    return panel&&mode==FM1_MANAGER_DEVICE;
}
bool fm1_manager_select(unsigned index)
{
    snapshots();if(index>=sd.count||installing())return false;
    selected=index;snprintf(selected_name,sizeof(selected_name),"%s",sd.files[index].name);technical=false;
    if(experimental_package(&sd.files[index]))experimental=true;
    reveal_selected=true;
    if(!panel||mode!=FM1_MANAGER_FIRMWARE)fm1_ui_route(0);else render();
    return panel&&mode==FM1_MANAGER_FIRMWARE;
}
void fm1_manager_tick(void)
{
    if(!panel)return;
    snapshots();
    if(key()!=last_key)render();
    if(progress_bar){
        unsigned progress=mode==FM1_MANAGER_SAMPLES?(samples.total?(uint64_t)samples.done*100/samples.total:0):update.progress;
        lv_bar_set_value(progress_bar,progress,LV_ANIM_OFF);
        if(mode==FM1_MANAGER_SAMPLES)lv_label_set_text_fmt(progress_label,"当前阶段 %u%%   /   %lu / %lu bytes   /   %.1f s",progress,(unsigned long)samples.done,(unsigned long)samples.total,samples.elapsed_ms/1000.0);
        else lv_label_set_text_fmt(progress_label,"%s   /   估算 %u%%   /   %.1f s",update.phase>=OTA_WAIT_REBOOT?"核对重启":update.phase>=OTA_START_WRITE?"传输固件":"检查固件",update.progress,update.elapsed_ms/1000.0);
    }
}
