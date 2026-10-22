#include "fm1_ui.h"
#include "m1_theme.h"
#include "m1_version.h"
#include "fm1_host.h"
#include "fm1_params.h"
#include "fm1_algorithms.h"
#include "fm1_ble.h"
#include "fm1_storage.h"
#include "fm1_samples.h"
#include "fm1_update.h"
#include "fm1_adapter.h"
#include "fm1_adapter_ui.h"
#include "fm1_console.h"
#include "esp_timer.h"
#include <math.h>
#include <stdio.h>
#include <string.h>

static lv_obj_t *work_label(lv_obj_t *p,const char *s,int x,int y,int w,const lv_font_t *font,uint32_t color)
{return m1_label(p,s,m1_wide(x),y,m1_wide(w),font,color);}
static lv_obj_t *work_button(lv_obj_t *p,const char *s,int x,int y,int w,int h,lv_event_cb_t cb,uintptr_t data,bool primary)
{return m1_button(p,s,m1_wide(x),y,m1_wide(w),h,cb,data,primary);}
static lv_obj_t *work_panel(lv_obj_t *p,int x,int y,int w,int h,bool scroll)
{return m1_panel(p,m1_wide(x),y,m1_wide(w),h,scroll);}
static void work_rule(lv_obj_t *p,int x,int y,int w){m1_rule(p,m1_wide(x),y,m1_wide(w));}

static lv_obj_t *body,*status_label,*clock_stop,*nav[4],*links[3],*unavailable_label;
static lv_obj_t *envelope,*diagram,*frequency_label,*fx_channel,*fx_scale_buttons[2];
static lv_point_precise_t envelope_points[5],path_points[64][6];
static unsigned route,sound_mode,voice_page,selected,group,env_group,global_group,fx_group,path_count;
static bool envelope_mode,updating,preview;
static bool extension;
static bool built_can_edit;
static uint32_t built_connection;
static char built_identity[32];
static fm1_adapter_state_t adapter_status;
static int drawn_algorithm=-1;
static int64_t notice_until;
static fm1_state_t current;
static fm1_ble_state_t link;
static fm1_storage_state_t storage;
static fm1_samples_state_t sample;
static fm1_update_state_t install;
typedef struct {
    lv_obj_t *widget,*label,*minus,*plus,*choices[6];
    unsigned choice_count,parameter,maximum;
    const fm1_param_t *definition;
    bool fx,dragging;
} control_t;
static control_t controls[8];
static unsigned control_count;
static void build_page(void);
static void update_controls(void);
static bool fresh(void)
{return !current.updating&&current.connected&&current.valid_voice&&current.can_edit&&esp_timer_get_time()-current.last_read_us<3000000;}
static const char *voice_unavailable_reason(void)
{return !current.connected?"音色编辑需要 USB 连接。":!current.identity[0]?"正在识别 USB 设备，请稍候。":"当前固件的音色编辑器尚未适配。";}
static bool dragging(void){if(fm1_console_dragging()||fm1_adapter_ui_dragging())return true;for(unsigned i=0;i<control_count;i++)if(controls[i].dragging)return true;return false;}
void fm1_ui_notice(const char *message)
{if(status_label)lv_label_set_text(status_label,message);notice_until=esp_timer_get_time()+5000000;}
void fm1_ui_route(unsigned target)
{if(target>3||dragging())return;route=target;sound_mode=0;preview=false;extension=false;build_page();}
bool fm1_ui_extension_view(unsigned page)
{if(page>5||dragging())return false;route=1;preview=false;extension=true;build_page();return fm1_adapter_ui_page(page);}
bool fm1_ui_object_view(unsigned object,unsigned section,unsigned track,unsigned step)
{if(dragging())return false;route=1;preview=false;extension=true;build_page();return fm1_adapter_ui_object(object,section,track,step);}
bool fm1_ui_action_view(unsigned index)
{if(dragging())return false;route=1;preview=false;extension=true;build_page();return fm1_adapter_ui_action(index);}
bool fm1_ui_page(unsigned view,unsigned op)
{
    if(view>9||op>5||dragging())return false;
    selected=op;
    extension=false;
    preview=false;sound_mode=0;
    if(view<5){route=2;sound_mode=1;voice_page=view<2?0:view-1;envelope_mode=view==1;preview=true;}
    else if(view==5)route=3;
    else if(view==6)route=1;
    else if(view==7){route=2;sound_mode=2;}
    else route=view==8?0:2;
    build_page();return true;
}
unsigned fm1_ui_view(void){return route==0?8:route==1?6:route==3?5:sound_mode==0?9:sound_mode==2?7:voice_page==0?(envelope_mode?1:0):voice_page+1;}
unsigned fm1_ui_operator(void){return selected;}
static void route_clicked(lv_event_t *e){fm1_ui_route((uintptr_t)lv_event_get_user_data(e));}
static void device_clicked(lv_event_t *e){(void)e;fm1_ui_route(3);fm1_manager_section(3);}
static void voice_tab(lv_event_t *e){if(dragging())return;voice_page=(uintptr_t)lv_event_get_user_data(e);build_page();}
static void operator_clicked(lv_event_t *e){if(dragging())return;selected=(uintptr_t)lv_event_get_user_data(e);voice_page=0;build_page();}
static void group_clicked(lv_event_t *e)
{if(dragging())return;unsigned n=(uintptr_t)lv_event_get_user_data(e);if(voice_page==0){envelope_mode=n==3;if(!envelope_mode)group=n;}else if(voice_page==2)global_group=n;else fx_group=n;build_page();}
static void env_clicked(lv_event_t *e){if(dragging())return;env_group=(uintptr_t)lv_event_get_user_data(e);build_page();}
static void preview_clicked(lv_event_t *e){(void)e;preview=true;build_page();}
static void extension_clicked(lv_event_t *e){(void)e;fm1_ui_extension_view(0);}
static void sound_clicked(lv_event_t *e){if(dragging())return;sound_mode=(uintptr_t)lv_event_get_user_data(e);preview=sound_mode==1&&!current.can_edit;build_page();}
static void files_clicked(lv_event_t *e){(void)e;fm1_ui_extension_view(5);}
static void read_clicked(lv_event_t *e){(void)e;fm1_ui_notice(fm1_host_read()?"已请求读取音色，等待设备回复。":"读取未受理：请检查 USB 与设备状态。");}
static void stop_clock_clicked(lv_event_t *e)
{(void)e;fm1_host_snapshot(&current);fm1_ui_notice(fm1_host_clock(current.connection_id,current.clock_bpm,false)?"Tab5 时钟已停止。":"时钟停止未受理：请检查连接。");}
static void task_clicked(lv_event_t *e)
{(void)e;if(sample.busy||sample.prepared)fm1_ui_page(7,0);else if(install.stage!=UPDATE_IDLE)fm1_ui_route(0);}
static void control_label(control_t *c,unsigned value,bool known)
{
    const char *name=c->fx?fm1_effect_labels[c->parameter]:c->definition->name;
    if(!known){lv_label_set_text_fmt(c->label,"%s   --",name);return;}
    if(c->choice_count){lv_label_set_text(c->label,name);return;}
    if(!c->fx&&c->parameter<126&&c->parameter%21==8){
        static const char *notes[]={"C","C#","D","D#","E","F","F#","G","G#","A","A#","B"};
        unsigned midi=value+9;lv_label_set_text_fmt(c->label,"BREAKPOINT   %s%d",notes[midi%12],(int)midi/12-1);return;
    }
    lv_label_set_text_fmt(c->label,"%s   %d",name,(int)value+(c->fx?0:c->definition->offset));
}
static void submit(control_t *c,unsigned value)
{
    fm1_host_snapshot(&current);
    if(value>c->maximum||!fresh()||current.connection_id!=built_connection||
       strcmp(current.identity,built_identity))return;
    bool ok=c->fx?fm1_host_effect(c->parameter,value):fm1_host_parameter(c->parameter,value);
    fm1_ui_notice(ok?(c->fx?"效果已入队；显示发送值，不代表设备读回。":"参数已入队，等待设备读回确认。"):"未发送：设备忙碌或连接已失效。");
}
static void control_event(lv_event_t *e)
{
    if(updating)return;
    control_t *c=lv_event_get_user_data(e);lv_event_code_t code=lv_event_get_code(e);
    unsigned value=lv_slider_get_value(c->widget);
    if(code==LV_EVENT_PRESSED)c->dragging=true;
    if(code==LV_EVENT_VALUE_CHANGED)control_label(c,value,true);
    if(code==LV_EVENT_RELEASED){c->dragging=false;submit(c,value);}
    if(code==LV_EVENT_PRESS_LOST){c->dragging=false;update_controls();}
}
static void choice_clicked(lv_event_t *e)
{if(updating||!fresh())return;uintptr_t p=(uintptr_t)lv_event_get_user_data(e);control_t *c=&controls[p/8];if(p%8<c->choice_count)submit(c,p%8);}
static void nudge(lv_event_t *e)
{
    if(updating||!fresh())return;
    uintptr_t n=(uintptr_t)lv_event_get_user_data(e);control_t *c=&controls[n/2];
    int v=lv_slider_get_value(c->widget)+(n%2?1:-1);if(v<0||v>(int)c->maximum)return;
    lv_slider_set_value(c->widget,v,LV_ANIM_OFF);control_label(c,v,true);submit(c,v);
}
static void make_control(lv_obj_t *parent,unsigned p,bool fx,int x,int y,int width)
{
    if(control_count>=8)return;
    x=m1_wide(x);width=m1_wide(width);
    unsigned index=control_count++;control_t *c=&controls[index];
    *c=(control_t){.parameter=p,.fx=fx,.definition=fx?NULL:fm1_param_definition(p)};
    c->maximum=fx?fm1_effect_max(p,current.fx_scale):c->definition->maximum;
    const char *options=fx?(p%4==0?"Off\nOn":p==1?"LP\nBP\nHP":p==5?"Room\nHall\nPlate":NULL):c->definition->options;
    if(!fx&&p<126&&(p%21==11||p%21==12))options="-LIN\n-EXP\n+EXP\n+LIN";
    if(!fx&&p==142)options="Tri\nDown\nUp\nSqr\nSin\nS&H";
    c->choice_count=options?c->maximum+1:0;
    c->label=m1_label(parent,"--",x,y,width,&m1_font_24,M1_MUTED);lv_label_set_long_mode(c->label,LV_LABEL_LONG_DOT);
    if(options){
        int cols=c->choice_count>4?3:c->choice_count,bw=(width-(cols-1)*8)/cols;
        for(unsigned o=0;o<c->choice_count;o++){
            const char *end=strchr(options,'\n');size_t len=end?(size_t)(end-options):strlen(options);char text[24];if(len>=sizeof(text))len=sizeof(text)-1;
            memcpy(text,options,len);text[len]=0;options=end?end+1:options+len;
            c->choices[o]=m1_button(parent,text,x+(o%cols)*(bw+8),y+42+(o/cols)*64,bw,c->choice_count>4?58:88,choice_clicked,index*8+o,false);
        }
        c->widget=c->choices[0];
    }else{
        c->minus=m1_button(parent,"-",x,y+42,88,88,nudge,index*2,false);c->plus=m1_button(parent,"+",x+width-88,y+42,88,88,nudge,index*2+1,false);
        c->widget=lv_slider_create(parent);lv_obj_set_pos(c->widget,x+116,y+82);lv_obj_set_size(c->widget,width-232,8);
        lv_slider_set_range(c->widget,0,c->maximum);lv_obj_set_ext_click_area(c->widget,32);
        lv_obj_set_style_bg_color(c->widget,lv_color_hex(M1_LINE),LV_PART_MAIN);lv_obj_set_style_bg_color(c->widget,lv_color_hex(M1_GREEN),LV_PART_INDICATOR);
        lv_obj_set_style_bg_color(c->widget,lv_color_hex(M1_FG),LV_PART_KNOB);lv_obj_set_style_radius(c->widget,0,LV_PART_KNOB);
        lv_obj_set_style_pad_all(c->widget,10,LV_PART_KNOB);lv_obj_add_event_cb(c->widget,control_event,LV_EVENT_ALL,c);
    }
}
static void grid_controls(const unsigned *params,unsigned count,int top,bool fx)
{
    lv_obj_t *grid=work_panel(body,0,top,1048,552-top,true);
    int y=0;
    for(unsigned i=0;i<count;i+=2){
        make_control(grid,params[i],fx,0,y,496);if(i+1<count)make_control(grid,params[i+1],fx,540,y,496);
        y+=(!fx&&(params[i]==142||(i+1<count&&params[i+1]==142)))?194:154;
    }
}
bool fm1_ui_edit(unsigned parameter,unsigned value)
{
    if(!fresh()||parameter>=145||!fm1_param_valid(parameter,value)||route!=2||sound_mode!=1||dragging())return false;
    /* Legacy diagnostics still exercise the real controls in their new groups. */
    if(parameter<126){selected=5-parameter/21;unsigned p=parameter%21;voice_page=0;envelope_mode=p<8;
        if(p<8)env_group=p/4;else group=(p>=16&&p<=19)?0:(p>=8&&p<=12)?1:2;
    }else if(parameter==134||parameter==135)voice_page=1;
    else {voice_page=2;global_group=parameter<130?0:parameter<134?1:parameter>=137&&parameter<=140?2:3;}
    preview=true;build_page();
    for(unsigned i=0;i<control_count;i++){
        control_t *c=&controls[i];if(c->fx||c->parameter!=parameter)continue;
        if(c->choice_count)lv_obj_send_event(c->choices[value],LV_EVENT_CLICKED,NULL);
        else {lv_obj_send_event(c->widget,LV_EVENT_PRESSED,NULL);lv_slider_set_value(c->widget,value,LV_ANIM_OFF);lv_obj_send_event(c->widget,LV_EVENT_VALUE_CHANGED,NULL);lv_obj_send_event(c->widget,LV_EVENT_RELEASED,NULL);}
        return true;
    }
    return false;
}
static void path(const int *xy,unsigned count,uint32_t color)
{
    if(path_count>=64||count>6)return;
    lv_point_precise_t *p=path_points[path_count++];for(unsigned i=0;i<count;i++)p[i]=(lv_point_precise_t){xy[i*2],xy[i*2+1]};
    lv_obj_t *line=lv_line_create(diagram);lv_line_set_points(line,p,count);lv_obj_set_style_line_color(line,lv_color_hex(color),0);lv_obj_set_style_line_width(line,2,0);
}
static void draw_algorithm(unsigned algorithm)
{
    if(!diagram||algorithm>31)return;
    lv_obj_clean(diagram);path_count=0;drawn_algorithm=algorithm;
    const fm1_alg_node_t *nodes=fm1_algorithms[algorithm];int minx=5,maxx=0,miny=3;
    for(unsigned i=0;i<6;i++){if(nodes[i].x<minx)minx=nodes[i].x;if(nodes[i].x>maxx)maxx=nodes[i].x;if(nodes[i].y<miny)miny=nodes[i].y;}
    int ox=(lv_obj_get_width(diagram)-8-(maxx-minx)*108)/2-minx*108,oy=(390-(3-miny)*96)/2-miny*96;
    for(unsigned i=0;i<6;i++){
        fm1_alg_node_t n=nodes[i];int x=ox+n.x*108,y=oy+n.y*96+30,next=y+66,bus=y+18;
        if(n.link==0){int p[]={x,y,x,next};path(p,2,M1_MUTED);}
        if(n.link==1||n.link==6||n.link==7){int p[]={x,y,x,bus,x+(n.link==7?-108:n.link==6?216:108),bus};path(p,3,M1_MUTED);}
        if(n.link==2){int p[]={x,y,x,bus+6};path(p,2,M1_MUTED);}
        if(n.link==3||n.link==4){int p[]={x,y,x,next};path(p,2,M1_MUTED);int q[]={x-(n.link==4?108:0),next,x-(n.link==4?108:0),bus,x+108,bus,x+108,next};path(q,4,M1_MUTED);}
        if(n.feedback){int cy=oy+n.y*96,rows=n.feedback==2?2:n.feedback==3?1:0,side=x+(n.feedback==4?-49:49);int p[]={x,cy-30,x,cy-44,side,cy-44,side,cy+rows*96+42,x,cy+rows*96+42};path(p,5,M1_YELLOW);}
    }
    for(unsigned i=0;i<6;i++){
        fm1_alg_node_t n=nodes[i];char text[16];snprintf(text,sizeof(text),"OP %u",n.id);
        lv_obj_t *b=m1_button(diagram,text,ox+n.x*108-44,oy+n.y*96-30,88,60,operator_clicked,n.id-1,false);
        if(n.y==3)lv_obj_set_style_border_color(b,lv_color_hex(M1_GREEN),0);
    }
}
static void fx_config_changed(lv_event_t *e)
{(void)e;if(updating)return;fm1_host_fx_config(lv_dropdown_get_selected(fx_channel),current.fx_scale);fm1_host_snapshot(&current);update_controls();}
static void fx_scale_clicked(lv_event_t *e)
{if(updating)return;fm1_host_fx_config(current.fx_channel,(uintptr_t)lv_event_get_user_data(e));fm1_host_snapshot(&current);update_controls();}
static void update_controls(void)
{
    if(route!=2||sound_mode!=1)return;
    updating=true;bool enabled=fresh();
    for(unsigned i=0;i<control_count;i++){
        control_t *c=&controls[i];m1_enabled(c->widget,enabled);if(!enabled)c->dragging=false;
        if(c->minus){m1_enabled(c->minus,enabled);m1_enabled(c->plus,enabled);}if(c->dragging)continue;
        bool known=c->fx?(current.fx_known&(1u<<c->parameter))!=0:current.valid_voice;
        unsigned v=c->fx?current.fx_values[c->parameter]:current.voice[c->parameter];
        if(c->fx){c->maximum=fm1_effect_max(c->parameter,current.fx_scale);if(!c->choice_count)lv_slider_set_range(c->widget,0,c->maximum);}if(v>c->maximum)v=c->maximum;
        if(c->choice_count){for(unsigned o=0;o<c->choice_count;o++){m1_enabled(c->choices[o],enabled);m1_selected(c->choices[o],known&&v==o);}}
        else if(known)lv_slider_set_value(c->widget,v,LV_ANIM_OFF);
        control_label(c,v,known);
    }
    if(envelope){unsigned base=(5-selected)*21;for(unsigned i=0;i<5;i++){unsigned v=current.valid_voice?current.voice[i?base+3+i:base+7]:0;envelope_points[i]=(lv_point_precise_t){i*84,36-(v>99?99:v)*34/99};}lv_line_set_points(envelope,envelope_points,5);}
    if(frequency_label){
        if(!current.valid_voice)lv_label_set_text(frequency_label,"FREQUENCY\n--");
        else {const uint8_t *op=current.voice+(5-selected)*21;float f=op[17]?powf(10,(op[18]&3)+op[19]/100.0f):(op[18]?op[18]:0.5f)*(1+op[19]/100.0f);lv_label_set_text_fmt(frequency_label,op[17]?"NOMINAL\n%.2f Hz":"RATIO\n%.3f",(double)f);}
    }
    if(diagram&&current.valid_voice&&drawn_algorithm!=current.voice[134])draw_algorithm(current.voice[134]);
    if(fx_channel){if(!lv_dropdown_is_open(fx_channel))lv_dropdown_set_selected(fx_channel,current.fx_channel);for(unsigned i=0;i<2;i++)m1_selected(fx_scale_buttons[i],current.fx_scale==(i?100:10));}
    updating=false;
}
static void voice_page_build(void)
{
    if(!current.can_edit&&!preview){
        work_label(body,"音色",0,0,500,&m1_font_28,M1_FG);work_label(body,"SOUND EDITOR",0,76,800,&m1_pixel_32,M1_GREEN);
        unavailable_label=work_label(body,voice_unavailable_reason(),0,162,1000,&m1_font_28,M1_FG);
        work_label(body,"六算子编辑器支持原厂固件。你仍可管理采样、连接和固件。",0,230,1000,NULL,M1_MUTED);
        work_button(body,"打开固件控制台",0,352,336,88,route_clicked,1,true);
        work_button(body,"预览原厂编辑器",368,352,336,88,preview_clicked,0,false);return;
    }
    const char *tabs[]={"算子","算法","全局","效果"};
    for(unsigned i=0;i<4;i++){lv_obj_t *b=work_button(body,tabs[i],i*184,0,168,64,voice_tab,i,false);m1_selected(b,i==voice_page);}
    work_button(body,"扩展",728,0,72,64,extension_clicked,0,false);
    work_button(body,"读取音色",816,0,220,64,read_clicked,0,false);
    if(voice_page==0){
        for(unsigned i=0;i<6;i++){char text[12];snprintf(text,sizeof(text),"OP %u",i+1);lv_obj_t *b=work_button(body,text,i*176,84,156,72,operator_clicked,i,false);m1_selected(b,i==selected);}
        unsigned params[8],count=0,base=(5-selected)*21;
        if(envelope_mode){
            work_button(body,"返回音高",0,178,196,64,group_clicked,0,false);
            for(unsigned i=0;i<2;i++){lv_obj_t *b=work_button(body,i?"包络电平":"包络速率",212+i*212,178,196,64,env_clicked,i,false);m1_selected(b,env_group==i);}
            work_label(body,"电平示意",664,170,370,&m1_font_24,M1_MUTED);
            envelope=lv_line_create(body);lv_obj_set_pos(envelope,m1_wide(664),212);lv_obj_set_style_line_width(envelope,2,0);lv_obj_set_style_line_color(envelope,lv_color_hex(M1_GREEN),0);
            for(unsigned i=0;i<4;i++)params[count++]=base+i+env_group*4;
            grid_controls(params,count,268,false);
        }else{
            const char *groups[]={"音高与输出","键盘缩放","力度与响应","包络"};
            for(unsigned i=0;i<4;i++){lv_obj_t *b=work_button(body,groups[i],i*212,178,196,64,group_clicked,i,false);m1_selected(b,i==group);}
            frequency_label=work_label(body,"",856,184,180,&lv_font_montserrat_16,M1_MUTED);
            static const unsigned sets[3][5]={{16,17,18,19,0},{8,9,10,11,12},{20,13,14,15,0}};
            count=group==1?5:4;for(unsigned i=0;i<count;i++)params[i]=base+sets[group][i];grid_controls(params,count,268,false);
        }
    }else if(voice_page==1){
        work_label(body,"绿色边框：输出算子   黄色连线：反馈",0,88,1030,NULL,M1_MUTED);
        diagram=work_panel(body,0,136,696,404,false);draw_algorithm(current.valid_voice?current.voice[134]:0);
        make_control(body,134,false,720,160,316);make_control(body,135,false,720,340,316);
    }else if(voice_page==2){
        const char *groups[]={"音高包络速率","音高包络电平","调制","同步与音高"};
        for(unsigned i=0;i<4;i++){lv_obj_t *b=work_button(body,groups[i],i*264,88,248,72,group_clicked,i,false);m1_selected(b,global_group==i);}
        unsigned params[5],count=4;
        if(global_group<2)for(unsigned i=0;i<4;i++)params[i]=126+global_group*4+i;
        else if(global_group==2)for(unsigned i=0;i<4;i++)params[i]=137+i;
        else {unsigned p[]={136,141,142,143,144};memcpy(params,p,sizeof(p));count=5;}
        grid_controls(params,count,192,false);
    }else{
        for(unsigned i=0;i<6;i++){lv_obj_t *b=work_button(body,fm1_effect_names[i],i*176,86,160,72,group_clicked,i,false);m1_selected(b,fx_group==i);lv_obj_set_style_text_font(lv_obj_get_child(b,0),&lv_font_montserrat_16,0);}
        work_label(body,"MIDI 通道",0,198,146,NULL,M1_MUTED);
        fx_channel=lv_dropdown_create(body);lv_dropdown_set_options(fx_channel,"1\n2\n3\n4\n5\n6\n7\n8\n9\n10\n11\n12\n13\n14\n15\n16");lv_obj_set_pos(fx_channel,m1_wide(160),178);lv_obj_set_size(fx_channel,m1_wide(104),72);
        lv_dropdown_set_symbol(fx_channel,"v");lv_obj_set_style_radius(fx_channel,2,0);lv_obj_set_style_border_color(fx_channel,lv_color_hex(M1_LINE),0);
        lv_obj_set_style_bg_color(fx_channel,lv_color_hex(M1_SURFACE),0);lv_obj_set_style_text_color(fx_channel,lv_color_hex(M1_FG),0);
        lv_obj_t *list=lv_dropdown_get_list(fx_channel);lv_obj_set_style_bg_color(list,lv_color_hex(M1_SURFACE),0);lv_obj_set_style_text_color(list,lv_color_hex(M1_FG),0);
        lv_obj_add_event_cb(fx_channel,fx_config_changed,LV_EVENT_VALUE_CHANGED,NULL);
        for(unsigned i=0;i<2;i++)fx_scale_buttons[i]=work_button(body,i?"0..100":"0..10",296+i*152,178,136,72,fx_scale_clicked,i?100:10,false);
        work_label(body,"最近发送值\n-- 表示未知",648,182,380,NULL,M1_MUTED);
        unsigned params[4];for(unsigned i=0;i<4;i++)params[i]=fx_group*4+i;grid_controls(params,4,268,true);
    }
}
static void sound_hub(void)
{
    const fm1_adapter_profile_t *profile=fm1_adapter_profile(current.identity);
    work_label(body,"音色采样管理",0,0,700,&m1_font_28,M1_FG);
    work_label(body,current.connected?profile->name:"连接 FM-1 后可管理设备音色与采样",0,60,1036,NULL,M1_MUTED);
    work_rule(body,0,116,1036);
    work_label(body,"音色编辑",0,156,500,&m1_font_28,M1_GREEN);
    work_label(body,current.can_edit?"六算子、算法、全局参数与效果":"原厂编辑器可预览；当前固件控制请进入工作台",0,208,1036,NULL,M1_MUTED);
    work_button(body,current.can_edit?"编辑当前音色":"预览原厂音色",0,260,490,84,sound_clicked,1,false);
    work_button(body,"音色与工程文件",536,260,500,84,files_clicked,0,false);
    work_rule(body,0,374,1036);
    work_label(body,"Open FM1S 采样库",0,408,610,&m1_font_28,M1_YELLOW);
    work_label(body,"离线预览；设备操作需兼容的蓝牙采样协议",0,458,610,NULL,M1_MUTED);
    work_button(body,"打开采样管理",660,416,376,104,sound_clicked,2,true);
}
static void build_page(void)
{
    fm1_host_snapshot(&current);
    built_can_edit=current.can_edit;
    built_connection=current.connection_id;
    snprintf(built_identity,sizeof(built_identity),"%s",current.identity);
    updating=true;fm1_manager_close();fm1_console_close();fm1_adapter_ui_close();lv_obj_clean(body);memset(controls,0,sizeof(controls));control_count=0;
    envelope=diagram=frequency_label=fx_channel=unavailable_label=NULL;drawn_algorithm=-1;
    for(unsigned i=0;i<4;i++)m1_selected(nav[i],i==route);
    if(route==1&&!extension)fm1_console_open(body);
    else if(route==1)fm1_adapter_ui_open(body);
    else if(route==2&&sound_mode==0)sound_hub();
    else if(route==2&&sound_mode==1)voice_page_build();
    else fm1_manager_open(body,route==0?FM1_MANAGER_FIRMWARE:route==2?FM1_MANAGER_SAMPLES:FM1_MANAGER_DEVICE);
    updating=false;update_controls();
}
static void tick(lv_timer_t *timer)
{
    (void)timer;fm1_host_snapshot(&current);fm1_ble_snapshot(&link);fm1_storage_snapshot(&storage);fm1_samples_snapshot(&sample);fm1_update_snapshot(&install);
    if(route==2&&sound_mode<2&&!dragging()&&
       (built_connection!=current.connection_id||strcmp(built_identity,current.identity)||(!preview&&built_can_edit!=current.can_edit)))build_page();
    if(unavailable_label)lv_label_set_text(unavailable_label,voice_unavailable_reason());
    lv_label_set_text(links[0],current.connected?"USB 已连接":"USB 未连接");
    lv_label_set_text(links[1],link.phase==FM1_BLE_READY?"BLE 已连接":link.phase==FM1_BLE_SCANNING?"BLE 扫描中":link.phase==FM1_BLE_CONNECTING||link.phase==FM1_BLE_DISCOVERING?"BLE 连接中":"BLE 未连接");
    lv_label_set_text(links[2],storage.mounted?(storage.busy?"SD 忙碌":"SD 已挂载"):"SD 未挂载");
    lv_obj_set_hidden(clock_stop,!current.clock_enabled);
    lv_obj_set_width(status_label,current.clock_enabled?M1_CONTENT_W-244:M1_CONTENT_W);
    if(route==0||route==3||(route==2&&sound_mode==2))fm1_manager_tick();
    if(route==1&&extension)fm1_adapter_ui_tick();
    if(route==1&&!extension)fm1_console_tick();
    if(sample.busy||sample.prepared)lv_label_set_text(status_label,"采样任务进行中 / 待确认    ·    点此查看");
    else if(install.stage!=UPDATE_IDLE)lv_label_set_text(status_label,"固件任务    ·    点此查看进度或结果");
    else if(esp_timer_get_time()>notice_until){
        if(current.clock_enabled)lv_label_set_text_fmt(status_label,"Tab5 时钟  %u BPM    /    USB MIDI",current.clock_bpm);
        else if(route==1&&extension){fm1_adapter_snapshot(&adapter_status);lv_label_set_text(status_label,adapter_status.message);}
        else if(route==2&&sound_mode==1)lv_label_set_text(status_label,fresh()?current.writes_failed?"上次参数写入失败；请核对设备读回。":"参数以设备读回为准；效果仅显示最近发送值。":"音色编辑不可用：检查原厂固件、USB 与数据新鲜度。");
        else lv_label_set_text(status_label,"M1n1Le55  " M1_VERSION "    /    FIRMWARE WORKSTATION");
    }
    update_controls();
}
void fm1_ui_start(void)
{
    lv_obj_t *root=lv_screen_active();lv_obj_set_style_bg_color(root,lv_color_hex(M1_BG),0);lv_obj_set_style_text_font(root,&m1_font_24,0);lv_obj_set_style_text_color(root,lv_color_hex(M1_FG),0);lv_obj_set_scrollable(root,false);
    m1_label(root,"M1n1Le55",24,24,400,&m1_pixel_32,M1_FG);m1_label(root,M1_VERSION,440,30,120,&m1_font_24,M1_MUTED);m1_rule(root,24,79,1232);
    for(unsigned i=0;i<3;i++){lv_obj_t *b=m1_button(root,"",704+i*184,12,176,56,device_clicked,0,false);lv_obj_set_style_border_width(b,0,0);links[i]=lv_obj_get_child(b,0);}
    const char *titles[]={"固件\n管理","工作台","音色","设备"};
    const uint32_t colors[]={M1_ORANGE,M1_BLUE,M1_YELLOW,M1_GREEN};
    for(unsigned i=0;i<4;i++){
        nav[i]=m1_button(root,titles[i],16,104+i*136,80,120,route_clicked,i,false);
        lv_obj_t *l=lv_obj_get_child(nav[i],0);lv_obj_set_style_text_font(l,&m1_font_20,0);lv_obj_align(l,LV_ALIGN_CENTER,0,5);
        lv_obj_t *mark=m1_panel(nav[i],30,10,20,3,false);
        lv_obj_set_style_bg_opa(mark,LV_OPA_COVER,0);lv_obj_set_style_bg_color(mark,lv_color_hex(colors[i]),0);
    }
    body=m1_panel(root,M1_CONTENT_X,104,M1_CONTENT_W,552,false);
    m1_rule(root,M1_CONTENT_X,674,M1_CONTENT_W);status_label=m1_label(root,"M1n1Le55",M1_CONTENT_X,684,M1_CONTENT_W,&m1_font_24,M1_MUTED);
    lv_obj_set_height(status_label,36);lv_label_set_long_mode(status_label,LV_LABEL_LONG_DOT);
    lv_obj_set_clickable(status_label,true);lv_obj_set_ext_click_area(status_label,8);lv_obj_add_event_cb(status_label,task_clicked,LV_EVENT_CLICKED,NULL);
    clock_stop=m1_button(root,"停止时钟",1036,678,220,42,stop_clock_clicked,0,false);lv_obj_set_style_text_font(lv_obj_get_child(clock_stop,0),&m1_font_20,0);lv_obj_set_hidden(clock_stop,true);
    current.fx_scale=10;route=0;build_page();lv_timer_create(tick,100,NULL);
}
