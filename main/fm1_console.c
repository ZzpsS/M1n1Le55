#include "fm1_console.h"
#include "fm1_adapter.h"
#include "fm1_host.h"
#include "fm1_params.h"
#include "fm1_ui.h"
#include "m1_theme.h"
#include "esp_timer.h"
#include <stdio.h>
#include <string.h>

/* This surface contains musical controls. File replacement remains in the
 * existing, confirmed object/file workflows. Coordinates use the 1048 grid. */
typedef enum { C_LOCAL, C_CC, C_STOCK, C_REMOTE, C_MIX, C_MASTER } control_kind_t;
typedef struct {
    lv_obj_t *slider,*label,*choice,*minus,*plus;
    control_kind_t kind;
    unsigned id,channel;
    int minimum,maximum;
    uint32_t connection,page,open_context;
    fm1_firmware_t profile;
    uint8_t remote_id;
    char identity[32];
    bool dragging,known;
    char name[56];
} control_t;
static lv_obj_t *panel,*state_label,*detail_label,*clock_label;
static control_t controls[8];
static unsigned control_count,mode,channel,note_base=48,chord_type,velocity=96;
static unsigned selected_track,parameter_first;
static fm1_state_t host;
static fm1_adapter_state_t remote;
/* LVGL is single-threaded. Keep large snapshots off its 7 KiB task stack. */
static fm1_adapter_state_t checked_remote;
static fm1_firmware_t profile;
static uint32_t connection,drawn_page;
static char bound_identity[32];
static bool building,dirty,need_read;
static bool view_ready,view_waiting;
static uint32_t view_before;
static int cc_values[16][128];
static bool cc_known[16][128];
static unsigned bpm=120;
static uint32_t pending_revision,pending_connection;
static unsigned pending_command,pending_op;
static bool pending,pending_open;
static bool open_known;
static uint32_t open_generation;
static uint8_t open_engines[8];
static int64_t next_read;
static int mix_level[8];
static bool mix_mute[8],mix_known[8];
static unsigned mix_count;
static bool live_known;
static uint8_t live_flags,live_throw;
static int live_master;
static char notice[160];
static int64_t notice_until;
static bool waiting_feedback;
static uint32_t notice_revision;
static void render(void);
static void event(lv_event_t *e);
enum { E_MODE=100,E_TYPE=110,E_ROOT=130,E_NOTE=150,E_TRACK=180,
       E_OCT_DOWN=200,E_OCT_UP,E_CH_DOWN,E_CH_UP,E_BPM_DOWN,E_BPM_UP,
       E_START,E_CONTINUE,E_STOP,E_CLOCK,E_REFRESH,E_PREV,E_NEXT,
       E_ADVANCED,E_FILE,E_STOCK_OP=240,E_MUTE=260,E_LIVE_PLAY=280,
       E_LIVE_STOP,E_LIVE_REC,E_REVERB_ON,E_REVERB_OFF,E_ECHO_ON,E_ECHO_OFF,
       E_MASTER_RELEASE,E_MMC_PLAY,E_MMC_STOP,E_MMC_CONTINUE,E_MMC_REC,
       E_MMC_REC_END,E_MMC_PAUSE,E_VA_PREV,E_VA_NEXT,E_NUDGE=400 };
_Static_assert(E_NOTE+28<=E_TRACK,"Note actions must not overlap track actions");
_Static_assert(E_TRACK+8<=E_OCT_DOWN,"Track actions must not overlap commands");
static lv_obj_t *label(const char *s,int x,int y,int w,const lv_font_t *f,uint32_t c)
{return m1_label(panel,s,m1_wide(x),y,m1_wide(w),f,c);}
static lv_obj_t *button(const char *s,int x,int y,int w,int h,unsigned action,bool primary)
{return m1_button(panel,s,m1_wide(x),y,m1_wide(w),h,event,action,primary);}
static bool connected(void)
{return host.connected&&!host.updating&&profile!=FW_UNKNOWN&&host.connection_id==connection&&!strcmp(host.identity,bound_identity);}
static bool can_send(void)
{
    fm1_state_t h;fm1_host_snapshot(&h);fm1_adapter_snapshot(&checked_remote);
    return h.connected&&!h.updating&&profile!=FW_UNKNOWN&&h.connection_id==connection&&
        !strcmp(h.identity,bound_identity)&&fm1_adapter_profile(h.identity)->id==profile&&
        !checked_remote.file_prepared;
}
static bool fresh_stock(void)
{return can_send()&&host.can_edit&&host.valid_voice&&esp_timer_get_time()-host.last_read_us<3000000;}
static void tell(const char *s)
{snprintf(notice,sizeof(notice),"%s",s);notice_until=esp_timer_get_time()+3500000;fm1_ui_notice(s);if(state_label)lv_label_set_text(state_label,s);}
static void accepted(bool ok)
{fm1_adapter_snapshot(&checked_remote);notice_revision=checked_remote.revision;waiting_feedback=ok;tell(ok?"请求已受理；等待发送结果。":"未受理：任务忙碌、连接变化或当前操作不可用。");}
static bool midi(unsigned st,unsigned a,unsigned b,unsigned n,unsigned gate)
{uint8_t m[3]={st,a,b};bool ok=can_send()&&fm1_adapter_midi(m,n,gate);accepted(ok);return ok;}
static bool command(unsigned cmd,const uint8_t *data,unsigned size,unsigned op)
{
    if(!can_send()||pending||checked_remote.busy||checked_remote.connection_id!=connection||strcmp(checked_remote.identity,host.identity))return false;
    uint32_t rev=checked_remote.result_revision;
    if(!fm1_adapter_command(cmd,data,size))return false;
    pending=true;pending_open=false;pending_revision=rev;pending_connection=connection;pending_command=cmd;pending_op=op;
    return true;
}
static void live(unsigned op,unsigned value)
{uint8_t data[2]={op,value};accepted(profile==FW_SLOOP&&(op!=10||live_known)&&command(77,data,op==0?1:2,op));}
static bool open_info(void)
{if(!can_send()||pending||checked_remote.busy||checked_remote.connection_id!=connection||strcmp(checked_remote.identity,host.identity))return false;uint32_t rev=checked_remote.result_revision;if(!fm1_adapter_open_command(0,NULL,0))return false;pending=true;pending_open=true;pending_revision=rev;pending_connection=connection;pending_command=0;return true;}
static void invalidate_open(void)
{open_known=false;open_generation++;memset(cc_known,0,sizeof(cc_known));dirty=true;}
static bool binding_valid(control_t *c)
{
    fm1_state_t h;fm1_host_snapshot(&h);fm1_adapter_snapshot(&checked_remote);
    return h.connected&&!h.updating&&h.connection_id==c->connection&&h.connection_id==connection&&
        !strcmp(h.identity,c->identity)&&fm1_adapter_profile(h.identity)->id==c->profile&&!checked_remote.file_prepared&&
        (c->kind!=C_CC||!fm1_adapter_at(c->profile)->open||
         (open_known&&c->open_context==open_generation&&!checked_remote.busy&&!(pending&&pending_open)))&&
        (c->kind!=C_REMOTE||(!checked_remote.busy&&checked_remote.page_revision==c->page&&
         checked_remote.connection_id==c->connection&&checked_remote.ready&&c->id<checked_remote.count&&
         checked_remote.rows[c->id].id==c->remote_id&&checked_remote.rows[c->id].known&&checked_remote.rows[c->id].described));
}
static void submit(control_t *c,int value)
{
    if(value<c->minimum||value>c->maximum||!binding_valid(c)){dirty=true;tell("控件已过期；请重新读取当前设备。");return;}
    if(c->kind==C_LOCAL){bpm=value;fm1_state_t h;fm1_host_snapshot(&h);if(h.clock_enabled)accepted(fm1_host_clock(connection,bpm,true));dirty=true;return;}
    bool ok=false;
    if(c->kind==C_CC){uint8_t m[]={0xb0|c->channel,c->id,value};ok=fm1_adapter_midi(m,3,0);if(ok){cc_values[c->channel][c->id]=value;cc_known[c->channel][c->id]=true;}}
    else if(c->kind==C_STOCK)ok=fresh_stock()&&fm1_host_parameter(c->id,value);
    else if(c->kind==C_REMOTE)ok=view_ready&&c->known&&fm1_adapter_set(c->id,value,c->page);
    else if(c->kind==C_MIX&&mix_known[c->id]){uint8_t d[4]={c->id,0,0,mix_mute[c->id]};fm1_put_v14(d+1,value);ok=command(28,d,4,c->id);}
    else if(c->kind==C_MASTER&&live_known){uint8_t d[2]={11,value};ok=command(77,d,2,11);}
    if(c->kind==C_CC&&ok){c->known=true;lv_label_set_text_fmt(c->label,"%s   %d",c->name,value);}
    if(!ok)dirty=true;
    accepted(ok);
}
static void slider_event(lv_event_t *e)
{
    if(building)return;
    control_t *c=lv_event_get_user_data(e);lv_event_code_t code=lv_event_get_code(e);
    if(code==LV_EVENT_PRESSED)c->dragging=true;
    if(code==LV_EVENT_VALUE_CHANGED)lv_label_set_text_fmt(c->label,"%s   %d",c->name,(int)lv_slider_get_value(c->slider));
    if(code==LV_EVENT_RELEASED){c->dragging=false;submit(c,lv_slider_get_value(c->slider));}
    if(code==LV_EVENT_PRESS_LOST){c->dragging=false;dirty=true;}
}
static void option_event(lv_event_t *e)
{if(!building){control_t *c=lv_event_get_user_data(e);submit(c,c->minimum+lv_dropdown_get_selected(c->choice));}}
static void control(control_kind_t kind,unsigned id,const char *name,int lo,int hi,int value,bool known,const char *options,int x,int y,int w)
{
    if(control_count>=8)return;
    unsigned index=control_count++;control_t *c=&controls[index];
    *c=(control_t){.kind=kind,.id=id,.channel=channel,.minimum=lo,.maximum=hi,.known=known,.connection=connection,.page=remote.page_revision,.open_context=open_generation};
    c->profile=profile;snprintf(c->identity,sizeof(c->identity),"%s",host.identity);if(kind==C_REMOTE&&id<remote.count)c->remote_id=remote.rows[id].id;
    snprintf(c->name,sizeof(c->name),"%s",name);char text[100];if(known)snprintf(text,sizeof(text),"%s   %d",name,value);else snprintf(text,sizeof(text),"%s   --",name);
    c->label=label(text,x,y,w,&m1_font_20,M1_FG);
    lv_obj_set_height(c->label,30);lv_label_set_long_mode(c->label,LV_LABEL_LONG_DOT);
    bool enabled=can_send()&&(kind!=C_CC||!fm1_adapter_at(profile)->open||(open_known&&!remote.busy&&!(pending&&pending_open)))&&(kind!=C_STOCK||fresh_stock())&&(kind!=C_REMOTE||known)&&(kind!=C_MIX||known)&&(kind!=C_MASTER||known);
    if(options&&options[0]){
        c->choice=lv_dropdown_create(panel);lv_obj_set_pos(c->choice,m1_wide(x),y+38);lv_obj_set_size(c->choice,m1_wide(w),54);
        lv_dropdown_set_options(c->choice,options);lv_dropdown_set_selected(c->choice,known&&value>=lo?value-lo:0);
        lv_obj_set_style_bg_color(c->choice,lv_color_hex(M1_SURFACE),0);lv_obj_set_style_text_color(c->choice,lv_color_hex(M1_FG),0);
        lv_obj_t *list=lv_dropdown_get_list(c->choice);lv_obj_set_style_bg_color(list,lv_color_hex(M1_SURFACE),0);lv_obj_set_style_text_color(list,lv_color_hex(M1_FG),0);lv_obj_set_style_text_font(list,&m1_font_20,0);
        lv_obj_set_style_text_font(c->choice,&m1_font_20,0);lv_obj_add_event_cb(c->choice,option_event,LV_EVENT_VALUE_CHANGED,c);m1_enabled(c->choice,enabled);return;
    }
    c->slider=lv_slider_create(panel);lv_obj_set_pos(c->slider,m1_wide(x+52),y+64);lv_obj_set_size(c->slider,m1_wide(w-104),10);
    lv_slider_set_range(c->slider,lo,hi);lv_slider_set_value(c->slider,!known?lo:value<lo?lo:value>hi?hi:value,LV_ANIM_OFF);
    lv_obj_set_style_bg_color(c->slider,lv_color_hex(M1_LINE),LV_PART_MAIN);lv_obj_set_style_bg_color(c->slider,lv_color_hex(M1_BLUE),LV_PART_INDICATOR);
    lv_obj_set_style_bg_color(c->slider,lv_color_hex(M1_FG),LV_PART_KNOB);lv_obj_set_ext_click_area(c->slider,18);
    lv_obj_add_event_cb(c->slider,slider_event,LV_EVENT_ALL,c);m1_enabled(c->slider,enabled);
    c->minus=button("-",x,y+38,40,54,E_NUDGE+index*2,false);m1_enabled(c->minus,enabled);
    c->plus=button("+",x+w-40,y+38,40,54,E_NUDGE+index*2+1,false);m1_enabled(c->plus,enabled);
}
static void mode_buttons(const char *const *names,unsigned count)
{int w=(1048-(count-1)*12)/count;for(unsigned i=0;i<count;i++){lv_obj_t *b=button(names[i],i*(w+12),88,w,52,E_MODE+i,false);m1_selected(b,mode==i);lv_obj_set_style_text_font(lv_obj_get_child(b,0),count>7?&m1_font_20:&m1_font_24,0);}}
static void transport_footer(bool mmc)
{
    unsigned a=mmc?E_MMC_PLAY:E_START,b=mmc?E_MMC_CONTINUE:E_CONTINUE,c=mmc?E_MMC_STOP:E_STOP;
    m1_enabled(button(mmc?"播放":"开始节奏",0,480,248,64,a,true),can_send());
    m1_enabled(button("继续",264,480,248,64,b,false),can_send());
    m1_enabled(button(mmc?"停止":"停止节奏",528,480,248,64,c,false),can_send());
    button("深入编辑",792,480,256,64,E_ADVANCED,false);
}
#include "fm1_console_instruments.inc"
#include "fm1_console_fl.inc"
#include "fm1_console_open.inc"
static void stock(void);
static void render(void)
{
    if(!panel)return;
    building=true;lv_obj_clean(panel);memset(controls,0,sizeof(controls));control_count=0;clock_label=detail_label=NULL;
    const fm1_adapter_profile_t *p=fm1_adapter_at(profile);const char *title=profile==FW_FOMNI?"CHORD HARP":profile==FW_VA?"VA / SYNTH":profile==FW_X0X?"X0X / PANEL":profile==FW_SLOOP?"SLOOP / PANEL":p->open?"OPEN / TRACKS":profile==FW_STOCK?"FM / VOICE":"NATIVE / PANEL";
    label(title,0,0,680,&m1_pixel_32,M1_FG);label(p->name,704,8,344,&m1_font_24,p->open&&profile!=FW_OPEN?M1_ORANGE:M1_BLUE);
    state_label=label(notice_until>esp_timer_get_time()?notice:!connected()?"连接并识别 USB 固件后启用控制。":remote.file_prepared?"文件待确认；先完成或取消文件操作。":remote.message,0,49,1048,&m1_font_20,M1_MUTED);
    lv_obj_set_height(state_label,30);lv_label_set_long_mode(state_label,LV_LABEL_LONG_DOT);
    if(profile==FW_UNKNOWN){label(host.updating?"固件任务进行中":"等待 FM-1",0,188,1048,&m1_font_28,M1_FG);label("身份确认前不开放控制。",0,250,1048,&m1_font_24,M1_MUTED);}
    else if(profile==FW_FOMNI)fomni();else if(profile==FW_VA)va();else if(profile==FW_X0X)x0x();else if(p->open)open_panel();else if(profile==FW_STOCK)stock();else fl_panel();
    drawn_page=remote.page_revision;building=false;dirty=false;
}
static unsigned stock_op;
static void stock(void)
{
    const char *names[]={"EDIT / OP","ENV","ALGORITHM","LFO","FX"};mode_buttons(names,5);
    label("原厂 V15 · 按已确认的音色结构分组；面板完整页序尚未确认。",0,153,1048,&m1_font_20,M1_MUTED);
    if(mode<2){
        for(unsigned i=0;i<6;i++){char t[16];snprintf(t,sizeof(t),"OP %u",i+1);m1_selected(button(t,i*176,196,168,48,500+i,false),stock_op==i);}
        unsigned base=(5-stock_op)*21;
        for(unsigned i=0;i<4;i++){
            const unsigned tune[]={18,19,20,16};unsigned id=base+(mode==0?tune[i]:i);
            const fm1_param_t *d=fm1_param_definition(id);char t[80];if(mode==1)snprintf(t,sizeof(t),"RATE %u",i+1);else snprintf(t,sizeof(t),"K%u  %s",i+1,d->name);
            control(C_STOCK,id,t,0,d->maximum,host.voice[id],fresh_stock(),NULL,i*264,272,248);
        }
        label(mode==1?"RATE 越大阶段越短；LEVEL 四折点在完整包络页编辑。":"选择算子后保持同一 OP；完整编辑含键盘缩放与灵敏度。",0,404,1048,&m1_font_20,M1_MUTED);
        button(mode==1?"完整包络图":"完整算子编辑",0,480,336,64,mode==1?507:506,true);
    }else if(mode==2||mode==3){
        const unsigned ids[2][4]={{134,135,136,144},{137,138,139,140}};
        for(unsigned i=0;i<4;i++){unsigned id=ids[mode-2][i];const fm1_param_t *d=fm1_param_definition(id);control(C_STOCK,id,d->name,0,d->maximum,host.voice[id],fresh_stock(),NULL,i*264,262,248);}
        label(mode==2?"全局算法、反馈、振荡器同步、移调。":"LFO 速度、延迟、音高与振幅深度；完整设置含波形与同步。",0,404,1048,&m1_font_20,M1_MUTED);
        button(mode==2?"算法图":"完整 LFO",0,480,336,64,mode==2?E_STOCK_OP+10:508,true);
    }else{label("FX · 当前原厂音色效果",0,262,1048,&m1_font_28,M1_FG);label("使用已接入的原厂效果参数；保持与当前音色绑定。",0,336,1048,&m1_font_24,M1_MUTED);button("效果参数",0,480,336,64,E_STOCK_OP+11,true);}
    m1_enabled(button("试听 C",356,480,336,64,E_NOTE+12,false),can_send());button("读取音色",712,480,336,64,E_REFRESH,false);
}
static void event(lv_event_t *e)
{
    unsigned a=(uintptr_t)lv_event_get_user_data(e);if(building||fm1_console_dragging())return;
    if(a>=E_NUDGE&&a<E_NUDGE+16){control_t *c=&controls[(a-E_NUDGE)/2];if(c->slider){int v=lv_slider_get_value(c->slider)+(a%2?1:-1);if(v>=c->minimum&&v<=c->maximum){lv_slider_set_value(c->slider,v,LV_ANIM_OFF);submit(c,v);}}return;}
    if(a>=E_MODE&&a<E_MODE+10){fm1_console_page(a-E_MODE);return;}
    if(instruments_event(a)||fl_event(a)||open_event(a))return;
    if(profile==FW_STOCK&&a>=500&&a<509){if(a<506){stock_op=a-500;render();}else fm1_ui_page(a==507?1:a==508?3:0,stock_op);return;}
    if(a>=E_NOTE&&a<E_NOTE+28){unsigned i=a-E_NOTE,n=note_base+i,ch=channel;static const unsigned pads[]={36,38,37,41,47,48,42,46};if(i>=20)n=pads[i-20];else if(i==12)n=60;if(fm1_adapter_at(profile)->open&&mode==1&&open_known&&open_engines[selected_track]==2){static const unsigned drums[]={36,38,41,45,50,37,39,56,49,46,42,36};n=drums[open_part%12];}midi(0x90|ch,n,velocity,3,180);return;}
    if(a>=E_TRACK&&a<E_TRACK+8){unsigned t=a-E_TRACK;if(fm1_adapter_at(profile)->open){selected_track=t;channel=t==7?9:t;open_reset();render();}return;}
    if(a>=E_STOCK_OP&&a<E_STOCK_OP+12){unsigned op=a-E_STOCK_OP;fm1_ui_page(op==10?2:op==11?4:0,op<6?op:0);return;}
    if(a>=E_MUTE&&a<E_MUTE+8){unsigned t=a-E_MUTE;if(mix_known[t]){uint8_t d[4]={t,0,0,!mix_mute[t]};fm1_put_v14(d+1,mix_level[t]);accepted(command(28,d,4,t));}return;}
    switch(a){
    case E_OCT_DOWN:if(note_base>=12)note_base-=12;render();break;
    case E_OCT_UP:if(note_base<=104)note_base+=12;render();break;
    case E_CH_DOWN:if(channel)channel--;render();break;
    case E_CH_UP:if(channel<15)channel++;render();break;
    case E_REFRESH:if(profile==FW_STOCK)accepted(can_send()&&fm1_host_read());else{need_read=true;memset(mix_known,0,sizeof(mix_known));live_known=false;live_throw=0;dirty=true;}break;
    case E_PREV:if(parameter_first>=4)parameter_first-=4;need_read=true;view_ready=false;dirty=true;break;
    case E_NEXT:if(parameter_first+4<remote.parameters)parameter_first+=4;need_read=true;view_ready=false;dirty=true;break;
    case E_ADVANCED:fm1_ui_extension_view(profile==FW_VA?1:profile==FW_FOMNI||profile==FW_X0X?2:4);break;
    case E_FILE:fm1_ui_extension_view(5);break;
    case E_START:midi(0xfa,0,0,1,0);break;
    case E_CONTINUE:midi(0xfb,0,0,1,0);break;
    case E_STOP:midi(0xfc,0,0,1,0);break;
    case E_CLOCK:if(can_send()){fm1_state_t h;fm1_host_snapshot(&h);bool ok=fm1_host_clock(connection,bpm,!h.clock_enabled);tell(ok?(h.clock_enabled?"Tab5 时钟已停止。":"Tab5 时钟已启动；请点击开始节奏。"):"时钟未改变；连接已变化。");dirty=true;}else accepted(false);break;
    case E_LIVE_PLAY:live(1,1);break;case E_LIVE_STOP:live(1,0);break;case E_LIVE_REC:live(4,0);break;
    case E_REVERB_ON:live(10,live_throw|1);break;case E_REVERB_OFF:live(10,live_throw&~1u);break;
    case E_ECHO_ON:live(10,live_throw|2);break;case E_ECHO_OFF:live(10,live_throw&~2u);break;
    case E_MMC_PLAY:accepted(can_send()&&fm1_adapter_mmc(2));break;case E_MMC_STOP:accepted(can_send()&&fm1_adapter_mmc(1));break;
    case E_MMC_CONTINUE:accepted(can_send()&&fm1_adapter_mmc(3));break;case E_MMC_REC:accepted(can_send()&&fm1_adapter_mmc(6));break;
    case E_MMC_REC_END:accepted(can_send()&&fm1_adapter_mmc(7));break;case E_MMC_PAUSE:accepted(can_send()&&fm1_adapter_mmc(9));break;
    case E_VA_PREV:midi(0xb0|channel,116,127,3,100);break;case E_VA_NEXT:midi(0xb0|channel,117,127,3,100);break;
    default:break;
    }
}
bool fm1_console_page(unsigned page)
{
    if(!panel||fm1_console_dragging())return false;
    unsigned count=fm1_adapter_at(profile)->open?10:profile==FW_STOCK?5:
        fm1_adapter_at(profile)->parameters?fl_page_count():instruments_page_count();
    if(page>=count)return false;
    mode=page;need_read=true;view_ready=false;view_waiting=false;
    memset(mix_known,0,sizeof(mix_known));live_known=false;live_throw=0;
    instruments_page_changed();fl_page_changed();open_sub=0;render();return true;
}
unsigned fm1_console_mode(void){return mode;}
bool fm1_console_dragging(void)
{if(!panel)return false;for(unsigned i=0;i<control_count;i++)if(controls[i].dragging||(controls[i].choice&&lv_dropdown_is_open(controls[i].choice)))return true;return false;}
static void reset_connection(void)
{
    connection=host.connection_id;snprintf(bound_identity,sizeof(bound_identity),"%s",host.identity);profile=fm1_adapter_profile(host.identity)->id;mode=0;channel=0;selected_track=0;parameter_first=0;note_base=48;stock_op=0;
    memset(cc_known,0,sizeof(cc_known));memset(cc_values,0,sizeof(cc_values));memset(mix_known,0,sizeof(mix_known));memset(mix_level,0,sizeof(mix_level));memset(mix_mute,0,sizeof(mix_mute));mix_count=0;live_known=false;live_throw=0;live_master=127;invalidate_open();memset(open_engines,0,sizeof(open_engines));pending=false;view_ready=false;view_waiting=false;waiting_feedback=false;need_read=true;notice_until=0;next_read=0;instruments_reset();fl_reset();open_reset();
    tell(!connected()?"等待 USB 固件身份确认。":remote.file_prepared?"文件待确认；先完成或取消文件操作。":fm1_adapter_at(profile)->parameters||profile==FW_STOCK?"已切换工作台；正在读取当前设备。":"工作台已就绪；可直接操作当前固件。");
}
void fm1_console_open(lv_obj_t *parent)
{panel=parent;fm1_host_snapshot(&host);fm1_adapter_snapshot(&remote);if(connection!=host.connection_id||strcmp(bound_identity,host.identity)||profile!=fm1_adapter_profile(host.identity)->id)reset_connection();else need_read=true;fm1_adapter_visible(true);render();}
void fm1_console_close(void)
{panel=state_label=detail_label=clock_label=NULL;control_count=0;pending=false;fm1_adapter_visible(false);}
static void consume_result(void)
{
    if(!pending)return;
    if(remote.connection_id!=pending_connection){pending=false;return;}
    if(remote.busy)return;
    if(remote.result_revision==pending_revision){pending=false;if(pending_command==27||pending_command==28)memset(mix_known,0,sizeof(mix_known));if(pending_command==77)live_known=false;if(pending_open)invalidate_open();tell("请求未完成；重新读取后再操作。");dirty=true;return;}
    pending=false;if(remote.result_command!=pending_command){memset(mix_known,0,sizeof(mix_known));live_known=false;live_throw=0;if(pending_open)invalidate_open();tell("回复不匹配；未更新控制状态。");dirty=true;return;}
    const uint8_t *d=remote.result;unsigned n=remote.result_size;
    if(pending_open&&pending_command==0&&n==14&&d[0]==4){
        bool valid=true;for(unsigned i=0;i<8;i++)valid=valid&&d[6+i]<6;
        bool changed=!open_known||memcmp(open_engines,d+6,8);
        if(!valid){invalidate_open();tell("轨道音源回复无效；请重新读取。");}
        else{if(open_known){
                if(open_engines[selected_track]!=d[6+selected_track])open_reset();
                for(unsigned i=0;i<8;i++)if(open_engines[i]!=d[6+i])memset(cc_known[i==7?9:i],0,sizeof(cc_known[0]));
            }
            memcpy(open_engines,d+6,8);open_known=true;dirty=true;if(changed){open_generation++;tell("已读取当前轨道引擎。");}}
    }
    else if(pending_command==27&&n>=2&&d[1]>0&&d[1]<=8&&(n==2+6u*d[1]||(profile==FW_SLOOP&&n==3+6u*d[1]))){
        bool valid=true;memset(mix_known,0,sizeof(mix_known));mix_count=d[1];
        for(unsigned i=0;i<mix_count;i++){int v=fm1_v14(d+2+i*6+2);mix_known[i]=v>=0&&v<=127&&d[2+i*6+4]<=1;valid=valid&&mix_known[i];mix_level[i]=v;mix_mute[i]=d[2+i*6+4]!=0;}
        if(!valid)memset(mix_known,0,sizeof(mix_known));
        tell(valid?"已读取当前混音状态。":"设备回复无效；请重新读取后操作。");dirty=true;
    }
    else if(pending_command==28&&n==4&&d[0]<8&&d[0]==pending_op){unsigned i=d[0];int v=fm1_v14(d+1);mix_known[i]=v>=0&&v<=127&&d[3]<=1;mix_level[i]=v;mix_mute[i]=d[3]!=0;tell(mix_known[i]?"已读回当前混音设置。":"设备回复无效；请重新读取后操作。");dirty=true;}
    else if(pending_command==77&&n==50&&d[0]==pending_op&&d[33]<=3){live_known=true;live_flags=d[1];live_master=d[32]?d[32]-1:127;live_throw=d[33];tell("已读取当前 LIVE 状态。");dirty=true;}
    else{if(pending_command==27||pending_command==28)memset(mix_known,0,sizeof(mix_known));if(pending_command==77)live_known=false;if(pending_open)invalidate_open();tell("设备回复无效；请重新读取后操作。");dirty=true;}
}
void fm1_console_tick(void)
{
    if(!panel)return;
    fm1_state_t h;fm1_host_snapshot(&h);bool change=h.connection_id!=connection||strcmp(h.identity,bound_identity);bool availability=h.connected!=host.connected||h.updating!=host.updating;bool clock_change=h.clock_enabled!=host.clock_enabled||h.clock_bpm!=host.clock_bpm;
    host=h;bool was_busy=remote.busy;fm1_adapter_snapshot(&remote);bool busy_change=was_busy!=remote.busy;if(change){pending=false;if(fm1_console_dragging()){tell("连接已变化；松开控件后重新读取。");return;}reset_connection();render();return;}
    if(waiting_feedback&&!remote.busy&&remote.revision!=notice_revision){waiting_feedback=false;notice_until=0;}
    consume_result();
    if(!fm1_console_dragging())fl_tick();
    if(connected()&&(need_read||fm1_adapter_at(profile)->open)&&!fm1_console_dragging()&&!remote.busy&&!remote.file_prepared&&!pending&&esp_timer_get_time()>=next_read&&!fm1_adapter_at(profile)->parameters){
        bool ok=fm1_adapter_at(profile)->open?open_info():profile==FW_STOCK?fm1_host_read():true;
        if(ok)need_read=false;
        next_read=esp_timer_get_time()+2000000;
    }
    if(!fm1_console_dragging()&&(dirty||availability||clock_change||busy_change||drawn_page!=remote.page_revision)){render();return;}
    for(unsigned i=0;i<control_count;i++){control_t *c=&controls[i];if(c->dragging)continue;int v=0;bool known=false;
        if(c->kind==C_STOCK){known=fresh_stock();v=host.voice[c->id];}
        else if(c->kind==C_REMOTE&&remote.page_revision==c->page&&c->id<remote.count){known=view_ready&&!remote.busy&&remote.ready&&remote.rows[c->id].known&&remote.rows[c->id].described;v=remote.rows[c->id].value;}
        else continue;
        c->known=known;bool enabled=known&&can_send();if(c->slider){m1_enabled(c->slider,enabled);if(known)lv_slider_set_value(c->slider,v,LV_ANIM_OFF);}if(c->minus)m1_enabled(c->minus,enabled);if(c->plus)m1_enabled(c->plus,enabled);if(c->choice){m1_enabled(c->choice,enabled);if(known&&!lv_dropdown_is_open(c->choice))lv_dropdown_set_selected(c->choice,v-c->minimum);}if(c->label){if(known)lv_label_set_text_fmt(c->label,"%s   %d",c->name,v);else lv_label_set_text_fmt(c->label,"%s   --",c->name);}
    }
    if(state_label)lv_label_set_text(state_label,notice_until>esp_timer_get_time()?notice:!connected()?"USB 未就绪；音乐控制暂不可用。":remote.file_prepared?"文件待确认；请先完成或取消文件操作。":remote.message);
}
