/* Native pages from pinned releases; see docs/CONSOLE_STRUCTURE_SOURCES_FL.md. */
typedef struct { const char *name; uint8_t family,scope,kind,condition; uint8_t ids[4]; } fl_page_t;
static const fl_page_t fl_felucca_pages[]={
    {"ENV",1,0,0,16,{1,2,3,4}},
    {"ENV DEST",1,0,0,16,{5,6,7,102}},
    {"LFO",2,0,0,16,{9,10,11,12}},
    {"LFO 2",2,0,0,16,{91,92,93,255}},
    {"LFO DEST",2,0,0,16,{13,14,15,16}},
    {"MOD",2,0,13,16,{255,49,50,51}},
    {"FX",3,0,0,16,{33,34,35,36}},
    {"SLICER",3,0,0,0,{45,46,47,48}},
    {"INSERT",3,0,0,16,{96,97,98,99}},
    {"INSERT 2",3,0,0,16,{100,255,255,255}},
    {"DLY",3,1,0,0,{4,5,6,7}},
    {"REVERB",3,1,0,0,{24,8,9,255}},
    {"CHORUS",3,1,0,0,{10,11,255,255}},
    {"SCL",4,0,0,16,{25,26,27,28}},
    {"CHORD",4,0,0,16,{81,82,255,255}},
    {"EDIT 1",5,0,0,16,{103,104,105,106}},
    {"EDIT 2",5,0,0,16,{107,108,109,110}},
    {"FILTER",5,0,0,17,{101,107,108,110}},
    {"LANES",5,0,0,18,{83,84,85,86}},
    {"LANES 2",5,0,0,18,{87,88,89,90}},
    {"SLICES",5,0,10,19,{255,255,255,255}},
    {"OPERATOR",5,0,9,20,{255,255,255,255}},
    {"OP ENV",5,0,9,20,{255,255,255,255}},
    {"OPERATOR 2",5,0,9,20,{255,255,255,255}},
    {"OP1 ENV",5,0,0,21,{61,62,63,64}},
    {"OP2 ENV",5,0,0,21,{66,67,68,69}},
    {"OP3 ENV",5,0,0,21,{71,72,73,74}},
    {"OP4 ENV",5,0,0,21,{76,77,78,79}},
    {"OP LEVEL",5,0,0,21,{65,70,75,80}},
    {"VOICE",5,0,0,16,{37,38,41,42}},
    {"VOICE 2",5,0,0,16,{43,44,39,40}},
    {"VOICE 3",5,0,0,16,{95,255,255,255}},
    {"SONG",6,1,2,0,{255,255,255,255}},
    {"USER",7,1,5,16,{255,255,255,255}},
    {"PRESETS",7,1,4,16,{255,255,255,255}},
    {"PHRASES",7,1,8,0,{255,255,255,255}},
    {"PROJECT",7,1,6,0,{16,255,18,19}},
    {"TOOLS",7,1,7,0,{22,23,255,255}},
    {"ARP",8,0,0,16,{17,18,19,20}},
    {"ARP 2",8,0,0,16,{21,22,23,24}},
    {"STEP",9,0,1,0,{0,1,2,3}},
    {"DETAIL",9,0,1,0,{0,1,2,3}},
    {"AUTOMATION",9,0,3,0,{255,255,255,255}},
    {"MIXER",0,0,0,0,{0,39,36,40}},
    {"CLOCK",0,1,0,0,{0,1,2,255}},
};



static const fl_page_t fl_sloop_pages[]={
    {"ENV",1,0,0,16,{1,2,3,4}},
    {"ENV DEST",1,0,0,16,{5,6,7,59}},
    {"LFO",2,0,0,16,{9,10,11,12}},
    {"LFO 2",2,0,0,16,{60,61,62,255}},
    {"LFO DEST",2,0,0,16,{13,14,15,16}},
    {"FX",3,0,0,16,{33,34,35,36}},
    {"FILTER",3,0,0,16,{50,255,255,255}},
    {"SLICER",3,0,0,0,{45,46,47,48}},
    {"INSERT",3,0,0,0,{53,54,55,56}},
    {"INSERT 2",3,0,0,0,{57,255,255,255}},
    {"DLY",3,1,0,0,{4,5,6,7}},
    {"REV/CHO",3,1,0,0,{8,9,10,11}},
    {"SEL",4,0,0,16,{25,26,27,49}},
    {"SEL 2",4,0,0,16,{28,51,52,255}},
    {"EDIT 1",5,0,0,16,{63,64,65,66}},
    {"EDIT 2",5,0,0,16,{67,68,69,70}},
    {"FILTER",5,0,0,17,{58,67,68,70}},
    {"VOICE",5,0,0,16,{37,38,41,42}},
    {"VOICE 2",5,0,0,16,{43,44,39,40}},
    {"GLOBAL",6,1,0,0,{0,1,2,3}},
    {"MASTER",6,1,0,0,{27,28,29,30}},
    {"SYSTEM",6,1,0,0,{12,13,14,15}},
    {"DRUMS",6,1,0,0,{24,25,26,32}},
    {"MIDI",6,1,0,0,{33,34,35,24}},
    {"MIDI 2",6,1,0,0,{14,36,12,44}},
    {"MIDI 3",6,1,0,0,{45,46,47,48}},
    {"TEXTURE",6,1,0,0,{37,38,39,40}},
    {"TEXTURE 2",6,1,0,0,{41,42,43,255}},
    {"PRESETS",7,1,4,16,{255,255,255,255}},
    {"USER",7,1,5,16,{255,255,255,255}},
    {"SECTION",7,1,6,0,{16,255,18,19}},
    {"TOOLS",7,1,7,0,{22,23,255,31}},
    {"ARP",8,0,0,16,{17,18,19,20}},
    {"ARP 2",8,0,0,16,{21,22,23,24}},
    {"STEP",9,0,1,16,{0,1,2,3}},
    {"PATTERN",9,0,0,0,{29,30,31,32}},
    {"SONG",9,0,2,0,{255,255,255,255}},
    {"TRACKS",10,0,0,0,{255,0,29,39}},
    {"DRUMS",10,0,11,0,{255,255,255,255}},
};
static const fl_page_t fl_jangada_pages[]={
    {"ENV",1,0,0,16,{1,2,3,4}},
    {"ENV DEST",1,0,0,16,{5,6,7,101}},
    {"LFO",2,0,0,16,{9,10,11,12}},
    {"LFO 2",2,0,0,16,{102,103,104,255}},
    {"LFO DEST",2,0,0,16,{13,14,15,16}},
    {"MOD 1",2,0,0,16,{49,50,51,255}},
    {"MOD 2",2,0,0,16,{52,53,54,255}},
    {"MOD 3",2,0,0,16,{55,56,57,255}},
    {"MOD 4",2,0,0,16,{58,59,60,255}},
    {"MACRO",2,1,0,0,{35,36,37,38}},
    {"FX",3,0,0,16,{33,34,35,36}},
    {"DIST",3,0,0,16,{62,33,63,83}},
    {"SLICER",3,0,0,0,{45,46,47,48}},
    {"INSERT",3,0,0,16,{86,87,88,89}},
    {"INSERT 2",3,0,0,16,{90,255,255,255}},
    {"DLY",3,1,0,0,{4,5,6,7}},
    {"REV/CHO",3,1,0,0,{8,9,10,11}},
    {"REVERB",3,1,0,0,{32,8,9,255}},
    {"SCL",4,0,0,16,{25,26,27,28}},
    {"CHORD",4,0,0,16,{61,84,85,255}},
    {"EDIT 1",5,0,0,16,{64,65,66,67}},
    {"EDIT 2",5,0,0,16,{68,69,70,71}},
    {"EDIT 3",5,0,0,16,{72,73,74,75}},
    {"EDIT 4",5,0,0,16,{76,77,78,79}},
    {"VOICE",5,0,0,16,{37,38,41,42}},
    {"VOICE 2",5,0,0,16,{43,44,39,40}},
    {"GLOBAL",6,1,0,0,{0,1,2,3}},
    {"CLICK",6,1,0,0,{39,40,41,42}},
    {"SYSTEM",6,1,0,0,{12,13,14,15}},
    {"DRUMS",6,1,0,0,{24,25,26,27}},
    {"MASTER",6,1,0,0,{28,29,30,33}},
    {"MASTER 2",6,1,0,0,{34,33,28,255}},
    {"KIT",6,1,0,0,{31,25,26,255}},
    {"PRESETS",7,1,4,16,{255,255,255,255}},
    {"USER",7,1,5,16,{255,255,255,255}},
    {"PROJECT",7,1,6,0,{16,255,18,19}},
    {"TOOLS",7,1,7,0,{22,23,255,255}},
    {"ARP",8,0,0,16,{17,18,19,20}},
    {"ARP 2",8,0,0,16,{21,22,23,24}},
    {"DRONE",8,0,0,16,{23,80,81,82}},
    {"STEP",9,0,1,0,{0,1,2,3}},
    {"STEP 2",9,0,1,0,{0,4,5,6}},
    {"PATTERN",9,0,0,0,{29,30,31,32}},
    {"VIVO",9,0,0,0,{91,92,93,94}},
    {"VIVO 2",9,0,0,0,{95,96,97,98}},
    {"VIVO 3",9,0,0,0,{99,100,255,255}},
    {"TRACKS",10,0,0,0,{255,0,29,39}},
};
typedef struct { const char *name,*pages[2]; uint8_t home[4],edit_count; } fl_engine_t;
static const fl_engine_t fl_felucca_engines[]={
    {"ANALOG",{"OSC","FLT"},{107,108,1,4},8},
    {"DIGITAL",{"OPS","MOD"},{107,108,109,4},8},
    {"PHASE",{"PHS","LINE"},{105,106,107,4},8},
    {"LOFI",{"CHIP","MOTN"},{104,105,106,4},8},
    {"SAMPLE",{"SET","TONE"},{103,107,1,4},8},
    {"VOICE",{"VOWL","TONE"},{103,107,108,105},8},
    {"TRIO",{"OSC","TONE"},{108,109,110,104},8},
    {"WHEEL",{"BARS","TONE"},{103,107,109,110},8},
    {"GRAIN",{"GRAN","SPRY"},{104,105,106,108},8},
    {"PHYS",{"BODY","EXCT"},{104,105,106,107},8},
    {"DRUM",{"KIT","HIT"},{104,105,106,107},8},
    {"NOISE",{"SRC","MOVE"},{105,106,104,4},8},
    {"FM6",{"OPS","PATCH"},{105,106,107,110},8},
    {"SLICE",{"SLCE","PLAY"},{103,104,106,109},8},
};
static const fl_engine_t fl_sloop_engines[]={
    {"ANALOG",{"OSC","FLT"},{67,68,1,4},8},
    {"DIGITAL",{"OPS","MOD"},{67,68,69,4},8},
    {"PHASE",{"PHS","LINE"},{65,66,67,4},8},
    {"LOFI",{"CHIP","MOTN"},{64,65,66,4},8},
    {"SAMPLE",{"SET","TONE"},{63,67,1,4},8},
    {"VOICE",{"VOWL","TONE"},{63,67,68,65},8},
    {"TRIO",{"OSC","TONE"},{68,69,70,64},8},
    {"WHEEL",{"BARS","TONE"},{63,67,69,70},8},
    {"GRAIN",{"GRAN","SPRY"},{64,65,66,68},8},
    {"FM6",{"OPS","PATCH"},{65,66,67,70},8},
    {"PHYS",{"BODY","EXCT"},{64,65,66,67},8},
    {"NOISE",{"SRC","MOVE"},{65,66,64,4},8},
    {"SLICE",{"SLCE","PLAY"},{63,64,66,69},8},
};
static const fl_engine_t fl_jangada_engines[]={
    {"ANALOG",{"OSC","FLT"},{68,69,1,4},16},
    {"DIGITAL",{"OPS","MOD"},{68,69,70,4},8},
    {"PHASE",{"PHS","LINE"},{66,67,68,4},8},
    {"LOFI",{"CHIP","MOTN"},{65,66,67,4},8},
    {"SAMPLE",{"SET","TONE"},{64,68,1,4},8},
    {"VOICE",{"VOWL","TONE"},{64,68,69,66},8},
    {"TRIO",{"OSC","TONE"},{69,70,71,65},8},
    {"WHEEL",{"BARS","TONE"},{64,68,70,71},8},
    {"GRAIN",{"GRAN","SPRY"},{65,66,67,69},8},
    {"FM6",{"OPS","PATCH"},{66,67,68,71},8},
    {"NOISE",{"SRC","MOVE"},{66,67,65,4},8},
    {"PHYS",{"BODY","EXCT"},{65,66,67,68},8},
    {"ROBO",{"VOICE","MOVE"},{65,67,69,71},8},
    {"SLICE",{"SLCE","PLAY"},{64,65,67,70},8},
};


#define FL_LEN(a) (sizeof(a)/sizeof((a)[0]))
enum { FL_PREV=2000,FL_NEXT,FL_REC,FL_MENU,FL_LIVE,FL_MOD,FL_ACTION,FL_MENU_GROUP=2020 };
static unsigned fl_sub[11],fl_aux,fl_group,fl_menu_page,fl_mod;
static uint32_t fl_generation,fl_request_generation;
static unsigned fl_requested_track=255;
static bool fl_bootstrap;
static fm1_parameter_ref_t fl_requested[4];
static unsigned fl_requested_count;
static bool fl_profile(void){return profile==FW_FELUCCA||profile==FW_SLOOP||profile==FW_JANGADA;}
static unsigned fl_page_count(void){return 10;}
static const fl_engine_t *fl_engine(void){
    const fl_engine_t *e=profile==FW_FELUCCA?fl_felucca_engines:profile==FW_SLOOP?fl_sloop_engines:fl_jangada_engines;
    unsigned n=profile==FW_FELUCCA?FL_LEN(fl_felucca_engines):profile==FW_SLOOP?FL_LEN(fl_sloop_engines):FL_LEN(fl_jangada_engines);
    return remote.engine<n?e+remote.engine:NULL;
}
static bool fl_drum(void){return profile!=FW_FELUCCA&&remote.engines&&remote.engine>=remote.engines;}
static bool fl_layout(void){
    return profile==FW_FELUCCA?(remote.parameters==111&&remote.globals==27&&remote.engine_base==103):
        profile==FW_SLOOP?(remote.parameters==71&&remote.globals==49&&remote.engine_base==63&&remote.protocol>=12):
        (remote.parameters==105&&remote.globals==43&&remote.engine_base==64);
}
static bool fl_shown(const fl_page_t *p){
    unsigned c=p->condition&15;
    if(c==1&&remote.engine!=0)return false;
    if(c==2&&remote.engine!=10)return false;
    if(c==3&&remote.engine!=13)return false;
    if(c==4&&remote.engine!=12)return false;
    if(c==5&&remote.engine!=1)return false;
    const fl_engine_t *e=fl_engine();
    if(profile==FW_JANGADA&&p->family==5&&p->ids[0]>=64&&p->ids[0]<=79&&e&&p->ids[0]-64>=e->edit_count)return false;
    return true;
}
static const fl_page_t *fl_current(unsigned *total){
    static const fl_page_t home={"HOME",0,0,0,0,{255,255,255,255}};
    const fl_page_t *p=profile==FW_FELUCCA?fl_felucca_pages:profile==FW_SLOOP?fl_sloop_pages:fl_jangada_pages;
    unsigned n=profile==FW_FELUCCA?FL_LEN(fl_felucca_pages):profile==FW_SLOOP?FL_LEN(fl_sloop_pages):FL_LEN(fl_jangada_pages);
    unsigned family=fl_aux==1?10:mode,count=family==0?1:0;
    for(unsigned i=0;i<n;i++)if(p[i].family==family&&fl_shown(p+i))count++;
    *total=count;if(!count)return &home;
    fl_sub[family]%=count;unsigned at=fl_sub[family];
    if(family==0){if(!at)return &home;at--;}
    for(unsigned i=0;i<n;i++)if(p[i].family==family&&fl_shown(p+i)){if(!at--)return p+i;}
    return &home;
}
static const uint8_t fl_menu_indices[5][8]={
    {0,1,2,3,4,17,18,23},{5,6,7,8,16,19,20,25},{9,10,12,13,14,22,255,255},
    {21,26,27,28,29,30,31,255},{11,15,24,32,255,255,255,255}
};
static const char *const fl_groups[]={"DISPLAY","CONTROL","AUDIO","MIDI","SYSTEM"};
static unsigned fl_refs(fm1_parameter_ref_t refs[4],unsigned slots[4]){
    unsigned n=0,total;const fl_page_t *p=fl_current(&total);
    if(fl_aux==3)return 0;
    if(fl_aux==2){
        if(profile!=FW_FELUCCA)return 0;
        for(unsigned i=0;i<4;i++){unsigned id=fl_menu_indices[fl_group][fl_menu_page*4+i];if(id<remote.menu_count){refs[n]=(fm1_parameter_ref_t){2,id};slots[n++]=i;}}
        return n;
    }
    if(p->kind&&p->kind!=13)return 0;
    if(fl_drum()&&(p->condition&16))return 0;
    for(unsigned i=0;i<4;i++){
        unsigned scope=p->scope,id=p->ids[i];
        if(!strcmp(p->name,"HOME")){
            if(fl_drum()){static const unsigned ids[]={25,26,39,29};id=ids[i];scope=i<2?1:0;}
            else {const fl_engine_t *e=fl_engine();if(!e)return 0;id=e->home[i];}
        }
        if(id==255)continue;
        if(p->kind==13)id+=3*fl_mod;
        refs[n]=(fm1_parameter_ref_t){scope,id};slots[n++]=i;
    }
    return n;
}
static bool fl_matches(const fm1_parameter_ref_t *refs,unsigned n){
    if(!n||!remote.ready||!remote.native_view||remote.count!=n||remote.connection_id!=connection||strcmp(remote.identity,host.identity))return false;
    for(unsigned i=0;i<n;i++)if(remote.rows[i].scope!=refs[i].scope||remote.rows[i].index!=refs[i].id||!remote.rows[i].described||!remote.rows[i].known)return false;
    return true;
}
static void fl_changed(void){fl_generation++;need_read=true;view_ready=false;dirty=true;next_read=0;}
static void fl_page_changed(void){if(!fl_profile())return;fl_aux=0;fl_changed();}
static void fl_reset(void){memset(fl_sub,0,sizeof(fl_sub));fl_aux=fl_group=fl_menu_page=fl_mod=0;fl_requested_track=255;fl_requested_count=0;fl_bootstrap=false;fl_changed();}
static void fl_action(const fl_page_t *p){
    switch(p->kind){
    case 1:fm1_ui_action_view(1);break;
    case 2:fm1_ui_action_view(profile==FW_FELUCCA?14:35);break;
    case 3:fm1_ui_action_view(17);break;
    case 4:fm1_ui_action_view(3);break;
    case 5:fm1_ui_action_view(4);break;
    case 9:fm1_ui_action_view(profile==FW_FELUCCA?12:13);break;
    case 11:fm1_ui_action_view(14);break;
    case 6:fm1_ui_extension_view(5);break;
    default:fm1_ui_extension_view(4);break;
    }
}
static const char *fl_action_text(const fl_page_t *p){
    switch(p->kind){case 1:return "编辑单步音符与条件";case 2:return profile==FW_FELUCCA?"编辑 SONG 段落":"LIVE 快速段落链";case 3:return "读取 AUTOMATION 记录";
    case 4:return "浏览工厂预设";case 5:return "浏览用户预设";case 6:return "工程文件";case 9:return "FM6 六算子编辑";case 11:return "十六鼓轨步进";
    default:return "相关工具";}
}
static void fl_panel(void){
    const char *names[]={"HOME","ENV","LFO","FX",profile==FW_SLOOP?"SEL":"SCL","EDIT","GLO","SAVE","ARP","SEQ"};mode_buttons(names,10);
    for(unsigned i=0;i<4;i++){char t[12];snprintf(t,sizeof(t),"T%u",i+1);lv_obj_t *b=button(t,i*84,156,76,44,E_TRACK+i,false);m1_selected(b,remote.ready&&remote.selected==i);m1_enabled(b,can_send()&&!remote.busy&&!pending);}
    button(profile==FW_FELUCCA?"MIXER":"REC / TRACKS",352,156,224,44,FL_REC,false);
    m1_selected(button("MENU",592,156,144,44,FL_MENU,false),fl_aux==2);
    if(profile==FW_SLOOP)m1_selected(button("LIVE",752,156,144,44,FL_LIVE,false),fl_aux==3);
    button("读取",912,156,136,44,E_REFRESH,false);
    if(fl_aux==3){
        label(live_known?(live_flags&1?"LIVE · 播放中":"LIVE · 已停止"):"LIVE · 正在读取演奏状态",0,221,1048,&m1_font_24,M1_FG);
        m1_enabled(button("播放",0,270,248,54,E_LIVE_PLAY,true),can_send()&&!remote.busy&&!pending);
        m1_enabled(button("停止",264,270,248,54,E_LIVE_STOP,false),can_send()&&!remote.busy&&!pending);
        m1_enabled(button("录音切换",528,270,248,54,E_LIVE_REC,false),can_send()&&!remote.busy&&!pending);
        control(C_MASTER,0,"MASTER · 总音量",0,127,live_master,live_known,NULL,0,350,512);
        m1_enabled(button(live_throw&1?"关闭混响抛送":"混响抛送",536,350,248,54,live_throw&1?E_REVERB_OFF:E_REVERB_ON,false),live_known&&can_send()&&!remote.busy&&!pending);
        m1_enabled(button(live_throw&2?"关闭回声抛送":"回声抛送",800,350,248,54,live_throw&2?E_ECHO_OFF:E_ECHO_ON,false),live_known&&can_send()&&!remote.busy&&!pending);
        label("LIVE 操作会影响本机演奏；页面导航仅在 Tab5 浏览。",0,478,1048,&m1_font_20,M1_MUTED);return;
    }
    if(fl_aux==2){
        if(profile==FW_JANGADA){
            label("MENU · 按本机顺序",0,226,1048,&m1_font_24,M1_FG);
            label("COLOR / SPEAKER / LIGHTS / KEYS / NOTES / USB AUDIO\nMIDI OUT / MIDI IN / MIDI LEARN / 2ND CORE\nNEW PROJECT / ABOUT",0,290,1048,&m1_font_20,M1_FG);
            label("这些设置在 FM-1 本机操作；MIDI 与音频参数另见 GLO。",0,448,1048,&m1_font_20,M1_MUTED);return;
        }
        if(profile==FW_SLOOP){
            static const char *const groups[]={"SCREEN","LIGHTS","AUDIO","SYSTEM"};
            static const char *const rows[]={"COLOR / ZOOM / KNOB ACCEL","LIGHTS / KEYS / NOTES","SPEAKER LOWCUT / USB AUDIO / USB RATE","2ND CORE / HARDWARE CALIBRATION\nMIDI LEARN CLEAR / ABOUT"};
            for(unsigned i=0;i<4;i++){lv_obj_t *b=button(groups[i],i*264,214,248,44,FL_MENU_GROUP+i,false);m1_selected(b,fl_group%4==i);}
            label(rows[fl_group%4],0,300,1048,&m1_font_24,M1_FG);
            label("MENU 设置在 FM-1 本机操作；音频与同步参数另见 GLO。",0,448,1048,&m1_font_20,M1_MUTED);return;
        }
        for(unsigned i=0;i<5;i++){lv_obj_t *b=button(fl_groups[i],i*212,214,200,44,FL_MENU_GROUP+i,false);m1_selected(b,fl_group==i);lv_obj_set_style_text_font(lv_obj_get_child(b,0),&m1_font_20,0);}
    }
    unsigned total;const fl_page_t *p=fl_current(&total);char title[128];
    static const char *const aux[]={"引擎旋钮","包络","低频调制","效果","音阶与和弦","声音编辑","全局设置","保存与载入","琶音","步进编排","轨道"};
    const fl_engine_t *e=fl_engine();
    if(fl_aux==2)snprintf(title,sizeof(title),"MENU / %s · %u / 2",fl_groups[fl_group],fl_menu_page+1);
    else if(!strcmp(p->name,"EDIT 1")||!strcmp(p->name,"EDIT 2"))snprintf(title,sizeof(title),"%s / %s · %s",p->name,e?e->pages[p->name[5]=='2']: "EDIT",e?e->name:"--");
    else snprintf(title,sizeof(title),"%s · %s   %u / %u",p->name,aux[p->family],fl_sub[p->family]+1,total);
    if(fl_aux!=2){button("<",0,214,64,44,FL_PREV,false);label(title,80,223,888,&m1_font_24,M1_FG);button(">",984,214,64,44,FL_NEXT,false);}
    if(remote.parameters&&!fl_layout()){label("当前固件布局未匹配固定版本。",0,298,1048,&m1_font_28,M1_ORANGE);label("请使用完整参数页读取设备描述后编辑。",0,358,1048,&m1_font_24,M1_MUTED);button("完整参数",0,458,336,54,E_ADVANCED,false);return;}
    if(fl_aux==2&&fl_group==4&&fl_menu_page==1){label("CALIBRATION / INFO / ABOUT",0,308,1048,&m1_font_24,M1_FG);label("这些面板页面请在 FM-1 本机打开。",0,374,1048,&m1_font_20,M1_MUTED);button("上一组参数",0,478,336,54,FL_PREV,false);return;}
    if(fl_drum()&&(p->condition&16)&&fl_aux!=2){label("DRUM TRACK · 本页不适用",0,310,1048,&m1_font_28,M1_MUTED);return;}
    if(p->kind&&p->kind!=13&&fl_aux!=2){
        const char *msg=p->kind==1?"STEP · 按轨道与步进编辑音符和条件":p->kind==2?"SONG · 段落与播放次序":p->kind==3?"AUTOMATION · 参数锁与自动化记录":
            p->kind==4?"PRESETS · 原厂声音目录":p->kind==5?"USER · 用户声音目录与保存":p->kind==6?"PROJECT / SECTION · 工程存取":
            p->kind==9?"OPERATOR · 六算子与包络":p->kind==11?"DRUMS · 鼓轨步进与套件":"此原生页面包含专用操作";
        label(msg,0,292,1048,&m1_font_28,M1_FG);
        label(p->kind==7||p->kind==8||p->kind==10?"该面板操作请在 FM-1 本机完成；下方提供相关工具。":"进入对应工具后读取当前对象，再编辑或执行操作。",0,352,1048,&m1_font_20,M1_MUTED);
        button(fl_action_text(p),0,434,512,64,FL_ACTION,true);
        if(p->kind==5)button("保存当前声音",536,434,512,64,2051,false);
        if(p->kind==4)button("载入工厂预设",536,434,512,64,2052,false);
        return;
    }
    fm1_parameter_ref_t refs[4];unsigned slots[4],n=fl_refs(refs,slots);bool ready=view_ready&&fl_layout()&&fl_matches(refs,n);
    if(p->kind==13&&fl_aux!=2){char s[32];snprintf(s,sizeof(s),"K1 · SLOT %u",fl_mod+1);button(s,0,304,248,64,FL_MOD,false);}
    if(!strcmp(p->name,"TRACKS")&&fl_aux!=2)label(profile==FW_SLOOP?"K1 · TRACK\n使用上方 T1–T4":"K1 · TYPE\nT4 类型见 GLO / DRUMS",0,288,248,&m1_font_20,M1_MUTED);
    if(profile==FW_JANGADA&&!strcmp(p->name,"KIT")&&fl_aux!=2)label("K4 · BEAT\n在 FM-1 载入节奏",792,298,248,&m1_font_20,M1_MUTED);
    for(unsigned i=0;i<n;i++){
        unsigned k=slots[i];fm1_remote_parameter_t *r=remote.rows+i;char text[80];
        snprintf(text,sizeof(text),"K%u · %s",k+1,ready?r->name:"--");
        control(C_REMOTE,i,text,ready?r->minimum:0,ready?r->maximum:127,ready?r->value:0,ready,ready?r->options:NULL,k*264,fl_aux==2?308:298,248);
    }
    label(fl_aux==2?title:!ready?"正在读取当前页面…":fl_aux==1?"已选轨道与 FM-1 同步；其余按原页旋钮顺序。":"Tab5 按原生页浏览 · 参数与 FM-1 同步",0,426,1048,&m1_font_20,M1_MUTED);
    if(fl_aux==2){button("上一组参数",0,478,336,54,FL_PREV,false);button("下一组参数",356,478,336,54,FL_NEXT,false);}
    else {button("完整参数",0,478,336,54,E_ADVANCED,false);button("预设 / 工程",356,478,336,54,E_FILE,false);}
}
static bool fl_event(unsigned a){
    if(!fl_profile())return false;
    if(a==E_ADVANCED){fm1_ui_extension_view(0);return true;}
    if(a>=E_TRACK&&a<E_TRACK+4){if(can_send()&&!checked_remote.busy&&!pending){fl_requested_track=a-E_TRACK;fl_changed();}else accepted(false);return true;}
    if(a==E_REFRESH){fl_changed();live_known=false;return true;}
    if(a==FL_REC){if(profile==FW_FELUCCA){mode=0;fl_sub[0]=1;fl_aux=0;}else fl_aux=1;fl_changed();render();return true;}
    if(a==FL_MENU){fl_aux=2;fl_menu_page=0;fl_changed();render();return true;}
    if(a==FL_LIVE){fl_aux=3;live_known=false;fl_changed();render();return true;}
    if(a>=FL_MENU_GROUP&&a<FL_MENU_GROUP+5){fl_group=a-FL_MENU_GROUP;fl_menu_page=0;fl_changed();render();return true;}
    if(a==FL_PREV||a==FL_NEXT){
        if(fl_aux==2)fl_menu_page^=1;else{unsigned n;fl_current(&n);unsigned f=fl_aux==1?10:mode;if(n)fl_sub[f]=(fl_sub[f]+(a==FL_NEXT?1:n-1))%n;}
        fl_changed();render();return true;
    }
    if(a==FL_MOD){fl_mod=(fl_mod+1)%4;fl_changed();render();return true;}
    if(a==FL_ACTION){unsigned n;fl_action(fl_current(&n));return true;}
    if(a==2051){fm1_ui_action_view(7);return true;}if(a==2052){fm1_ui_action_view(2);return true;}
    return false;
}
static void fl_tick(void){
    if(!fl_profile()||!connected())return;
    if(remote.busy||pending)return;
    fm1_parameter_ref_t refs[4];unsigned slots[4],n=fl_refs(refs,slots);
    if(view_waiting){
        view_waiting=false;
        view_ready=fl_request_generation==fl_generation&&remote.page_revision!=view_before&&fl_matches(fl_requested,fl_requested_count)&&!fl_bootstrap&&fl_matches(refs,n);
        if(!view_ready)need_read=true;
        dirty=true;
    }
    if(fl_aux==3){
        view_ready=false;
        if(need_read||(!live_known&&esp_timer_get_time()>next_read)){uint8_t op=0;if(command(77,&op,1,0)){need_read=false;next_read=esp_timer_get_time()+1000000;}}
        return;
    }
    if(remote.parameters&&!fl_layout()){view_ready=false;need_read=false;return;}
    if(!remote.parameters){refs[0]=(fm1_parameter_ref_t){0,0};n=1;fl_bootstrap=true;}
    else fl_bootstrap=false;
    if(n&&!fl_matches(refs,n)){view_ready=false;need_read=true;}
    if(!n&&fl_requested_track==255){view_ready=false;need_read=false;return;}
    if((need_read||fl_requested_track!=255)&&esp_timer_get_time()>=next_read&&can_send()&&!checked_remote.busy){
        if(!n){refs[0]=(fm1_parameter_ref_t){0,0};n=1;fl_bootstrap=true;}
        uint32_t before=checked_remote.page_revision;
        if(fm1_adapter_native_view(refs,n,fl_requested_track)){
            memcpy(fl_requested,refs,n*sizeof(*refs));fl_requested_count=n;fl_request_generation=fl_generation;
            fl_requested_track=255;view_before=before;view_waiting=true;view_ready=false;need_read=false;next_read=esp_timer_get_time()+400000;
        }
    }
}
#undef FL_LEN
