/* Open V12 / Probe / Diag share the native screen.c structure. Browsing
 * these pages is local; channel-addressed MIDI and object reads are explicit. */
enum { O_VOICE=3000,O_PROJECT,O_SEQ,O_LOCKS,O_ARP,O_TRACK_SETTINGS,
       O_FX_GLOBAL,O_SUB=3020,O_PART=3040,O_STEP=3060,O_STEP_PREV=3080,O_STEP_NEXT,O_SCL_TONE };
static unsigned open_sub,open_part,open_step,open_step_page;
static const char *const open_names[]={"DX7 FM","303","808","PSG","PLUCK","SAMPLER"};
static const char *open_engine_name(unsigned t)
{return open_known&&open_engines[t]<6?open_names[open_engines[t]]:"--";}
static void open_reset(void)
{open_sub=open_part=open_step=open_step_page=0;}
static void native_reference(unsigned col,const char *name,const char *where,int y)
{
    char s[80];snprintf(s,sizeof(s),"K%u  %s",col+1,name);
    label(s,col*264,y,248,&m1_font_20,M1_FG);
    label(where,col*264,y+42,248,&m1_font_20,M1_MUTED);
}
static void open_knobs(const char *const names[4],const int cc[4],int y)
{
    for(unsigned i=0;i<4;i++){
        if(!names[i]||!names[i][0]){native_reference(i,"—","此音源无对应参数",y);continue;}
        if(cc&&cc[i]>=0){char s[72];snprintf(s,sizeof(s),"K%u  %s",i+1,names[i]);
            control(C_CC,cc[i],s,0,127,cc_values[channel][cc[i]],cc_known[channel][cc[i]],NULL,i*264,y,248);
        }else native_reference(i,names[i],"进入对象编辑",y);
    }
}
static void open_context(void)
{
    static const char *const short_names[]={"DX7","303","808","PSG","PLUCK","SMP"};
    for(unsigned i=0;i<8;i++){char s[40];const char *short_name=open_known&&open_engines[i]<6?short_names[open_engines[i]]:"--";
        snprintf(s,sizeof(s),"T%u  %s",i+1,short_name);
        lv_obj_t *b=button(s,i*132,154,124,46,E_TRACK+i,false);m1_selected(b,selected_track==i);
        lv_obj_set_style_text_font(lv_obj_get_child(b,0),&m1_font_20,0);
    }
    char s[160];snprintf(s,sizeof(s),"T%u  /  %s  /  CH%u    ·    页面浏览在 Tab5，MIDI 按轨发送",selected_track+1,open_engine_name(selected_track),channel+1);
    label(s,0,211,1048,&m1_font_20,M1_MUTED);
}
static void open_voice_knobs(void)
{
    unsigned type=open_known?open_engines[selected_track]:255;
    if(type>5){label("正在读取当前轨道的真实音源…",0,275,1048,&m1_font_24,M1_MUTED);return;}
    static const char *const basic[6][4]={
        {"Algorithm","Feedback","Transpose","Volume"},
        {"Cutoff","Resonance","Env Mod","Decay"},
        {"Level","Tone","Decay","Tune"},
        {"Mode","Noise","Tune","Release"},
        {"Decay","Bright","Tune","Release"},
        {"Level","Tune","Length","Choke"}};
    static const int cc[6][4]={{-1,-1,-1,7},{74,71,1,75},{-1,-1,-1,-1},{-1,74,-1,72},{75,74,-1,72},{-1,-1,-1,-1}};
    static const char *const bass[3][4]={{"Cutoff","Resonance","Env Mod","Decay"},{"Accent","Wave","Tune","Volume"},{"Drive","Drive Type","Slide","Acc Decay"}};
    static const int bass_cc[3][4]={{74,71,1,75},{-1,-1,-1,-1},{-1,-1,5,-1}};
    if(type==1){const char *pages[]={"FILTER","OSC / LEVEL","DRIVE / SLIDE"};
        if(open_sub>2)open_sub=0;
        for(unsigned i=0;i<3;i++)m1_selected(button(pages[i],i*264,252,248,44,O_SUB+i,false),open_sub==i);
        open_knobs(bass[open_sub],bass_cc[open_sub],320);
    }else if(type==5){
        for(unsigned i=0;i<8;i++){char s[16];snprintf(s,sizeof(s),"PAD %u",i+1);m1_selected(button(s,i*132,252,124,44,O_PART+i,false),open_part==i);}
        open_knobs(basic[type],cc[type],320);
    }else if(type==2){
        static const char *const parts[]={"BD","SD","LT","MT","HT","RS","CP","CB","CY","OH","CH","KIT"};
        for(unsigned i=0;i<12;i++)m1_selected(button(parts[i],i*88,252,80,44,O_PART+i,false),open_part==i);
        /* The object editor carries each part's distinct order and both variants. */
        label("PRESETS 选鼓声部 / KIT；每个声部使用自己的参数。",0,324,1048,&m1_font_24,M1_FG);
        label("打开所选声部音色后读取实际字段，不套用统一旋钮。",0,375,1048,&m1_font_20,M1_MUTED);
    }else open_knobs(basic[type],cc[type],288);
    label("数值为 MIDI 发送目标；完整音色从设备读取后编辑。",0,440,1048,&m1_font_20,M1_MUTED);
}
static void open_panel(void)
{
    const char *names[]={"HOME","EDIT","ENV","LFO","FX","SEQ","GLO","SAVE","ARP","SCL"};mode_buttons(names,10);open_context();
    unsigned type=open_known?open_engines[selected_track]:255;
    if(mode==0){
        label("八轨总览 · 选择轨道后，EDIT / ENV / FX 保持同一轨",0,262,1048,&m1_font_24,M1_FG);
        static const char *const n[]={"Level","Source","Step","Tempo"};static const int cc[]={7,-1,-1,-1};open_knobs(n,cc,318);
        label("上方音源为设备读回；音源切换用本机，步进与速度进入工程编辑。",0,440,1048,&m1_font_20,M1_MUTED);
        m1_enabled(button("试听当前轨",0,480,248,64,E_NOTE,true),can_send());button("工程设置",264,480,248,64,O_PROJECT,false);button("重新读取",528,480,248,64,E_REFRESH,false);button("全部工具",792,480,256,64,E_ADVANCED,false);
    }else if(mode==1){open_voice_knobs();m1_enabled(button("试听当前声部",0,480,248,64,E_NOTE+(type==5?20+open_part:0),true),can_send());button("所选音色编辑",264,480,248,64,O_VOICE,false);button("轨道设置",528,480,248,64,O_TRACK_SETTINGS,false);button("音色文件",792,480,256,64,E_FILE,false);
    }else if(mode==2){
        static const char *const n[6][4]={{"Attack","Decay","Sustain","Release"},{"Env Mod","Decay","Acc Decay","Cutoff"},{"Decay","Attack","Tone","Tune"},{"Release","","",""},{"Decay","Release","Bright",""},{"Length","Choke","",""}};
        static const int cc[6][4]={{-1,-1,-1,-1},{1,75,-1,74},{-1,-1,-1,-1},{72,-1,-1,-1},{75,72,74,-1},{-1,-1,-1,-1}};
        if(type<6&&type!=2)open_knobs(n[type],cc[type],278);else if(type==2)label("808 ENV · 进入所选声部音色，按实际支持字段调节。",0,278,1048,&m1_font_24,M1_FG);
        label(type==0?"FM ENV 是总包络覆盖层；六算子 Rates / Levels 在音色内。":type==2?"808 仅显示当前鼓声部实际支持的项目；声部在 EDIT 选择。":"保持固件原有 ENV 顺序；空白项不补通用 ADSR。",0,408,1048,&m1_font_20,M1_MUTED);
        button("包络对象编辑",0,480,336,64,type==0?O_TRACK_SETTINGS:O_VOICE,true);button("轨道设置",356,480,336,64,O_TRACK_SETTINGS,false);button("全部工具",712,480,336,64,E_ADVANCED,false);
    }else if(mode==3){
        static const char *const fm[2][4]={{"Wave","Speed","PMD","AMD"},{"Delay","P Sens","LFO Sync","OSC Sync"}};
        static const char *const n[]={"Rate","Depth","Target","Wave"};
        if(type==0){m1_selected(button("LFO 1",0,252,248,44,O_SUB,false),open_sub==0);m1_selected(button("LFO 2",264,252,248,44,O_SUB+1,false),open_sub==1);}
        open_knobs(type==0?fm[open_sub?1:0]:n,NULL,type==0?320:278);
        label(type==0?"FM LFO · SELECT 翻两页，按本机四旋钮顺序对应。":"非 FM 的轨道 LFO：作用于整轨，目标为音量或声像。",0,440,1048,&m1_font_20,M1_MUTED);
        button("LFO 对象编辑",0,480,336,64,type==0?O_VOICE:O_TRACK_SETTINGS,true);button("试听当前轨",356,480,336,64,E_NOTE,false);button("全部工具",712,480,336,64,E_ADVANCED,false);
    }else if(mode==4){
        m1_selected(button("本轨发送",0,252,248,44,O_SUB,false),open_sub==0);m1_selected(button("共用效果",264,252,248,44,O_SUB+1,false),open_sub==1);
        static const char *const n[2][4]={{"Rev Send","Dly Send","Track Level","All Return"},{"Reverb Time","Delay Time","Feedback","Return"}};
        static const int cc[2][4]={{91,-1,7,-1},{-1,-1,-1,-1}};open_knobs(n[open_sub?1:0],cc[open_sub?1:0],320);
        button(open_sub?"共用效果编辑":"本轨发送编辑",0,480,336,64,open_sub?O_PROJECT:O_TRACK_SETTINGS,true);button("试听当前轨",356,480,336,64,E_NOTE,false);button("全部工具",712,480,336,64,E_ADVANCED,false);
    }else if(mode==5){
        char s[128];snprintf(s,sizeof(s),"SEQ · 选择编辑位置（非播放光标）  /  PAGE %u  /  STEP %u",open_step_page+1,open_step+1);label(s,0,250,1048,&m1_font_20,M1_MUTED);
        for(unsigned i=0;i<16;i++){snprintf(s,sizeof(s),"%02u",open_step_page*16+i+1);m1_selected(button(s,(i%8)*132,290+(i/8)*58,124,50,O_STEP+i,false),open_step==open_step_page*16+i);}
        button("< PAGE",0,416,124,44,O_STEP_PREV,false);button("PAGE >",132,416,124,44,O_STEP_NEXT,false);button("步进细节",264,416,248,44,O_SEQ,true);button("参数锁",528,416,248,44,O_LOCKS,false);button("循环 / 录入设置",792,416,256,44,O_TRACK_SETTINGS,false);
        button("播放",0,480,248,64,E_MMC_PLAY,true);button("停止",264,480,248,64,E_MMC_STOP,false);button("开始录音",528,480,248,64,E_MMC_REC,false);button("结束录音",792,480,256,64,E_MMC_REC_END,false);
    }else if(mode==6){
        unsigned first=selected_track<4?0:4;for(unsigned i=0;i<4;i++){char s[64];snprintf(s,sizeof(s),"K%u  T%u Level",i+1,first+i+1);unsigned old=channel;channel=first+i==7?9:first+i;control(C_CC,7,s,0,127,cc_values[channel][7],cc_known[channel][7],NULL,i*264,284,248);channel=old;}
        label("GLO · 当前四轨音量；静音与独奏是独立状态，在工程内读写。",0,422,1048,&m1_font_20,M1_MUTED);
        button("静音 / 独奏",0,480,336,64,O_PROJECT,true);button("播放",356,480,336,64,E_MMC_PLAY,false);button("停止",712,480,336,64,E_MMC_STOP,false);
    }else if(mode==7){
        label("SAVE / PRESETS",0,270,1048,&m1_pixel_32,M1_FG);label("工程预设包含八轨与步进；轨道音色只包含当前轨音源参数。\n采样音频使用独立采样库。",0,330,1048,&m1_font_24,M1_MUTED);
        button("工程预设 / 文件",0,480,336,64,E_FILE,true);button("当前轨音色",356,480,336,64,O_VOICE,false);button("工程设置",712,480,336,64,O_PROJECT,false);
    }else if(mode==8){
        static const char *const n[]={"Rate","Direction","Range","Gate"};open_knobs(n,NULL,284);label("ARP · 当前轨琶音；Swing、Hold 与启用状态一并保存在工程。",0,422,1048,&m1_font_20,M1_MUTED);
        button("当前轨琶音编辑",0,480,336,64,O_ARP,true);button("播放",356,480,336,64,E_MMC_PLAY,false);button("停止",712,480,336,64,E_MMC_STOP,false);
    }else{
        label(type==0?"SCL · 当前算子的键盘缩放":"SCL · 当前音源的音色快捷页",0,278,1048,&m1_font_28,M1_FG);
        label(type==0?"在音色内选择 OP1–OP6，调整断点、左右深度与曲线。":"本机 SCL 进入音色编辑；303 直接进入 OSC / LEVEL。",0,350,1048,&m1_font_24,M1_MUTED);
        m1_enabled(button(type==0?"FM 音色编辑":"进入音色快捷页",0,480,336,64,type==0?O_VOICE:O_SCL_TONE,true),type<6&&can_send());button("全部工具",712,480,336,64,E_ADVANCED,false);
    }
}
static bool open_event(unsigned a)
{
    if(!fm1_adapter_at(profile)->open)return false;
    if(a==O_SCL_TONE){mode=1;open_sub=open_known&&open_engines[selected_track]==1?1:0;render();return true;}
    if(a>=O_SUB&&a<O_SUB+3){open_sub=a-O_SUB;render();return true;}
    if(a>=O_PART&&a<O_PART+12){open_part=a-O_PART;render();return true;}
    if(a>=O_STEP&&a<O_STEP+16){open_step=open_step_page*16+a-O_STEP;render();return true;}
    if(a==O_STEP_PREV||a==O_STEP_NEXT){if(a==O_STEP_PREV&&open_step_page)open_step_page--;if(a==O_STEP_NEXT&&open_step_page<3)open_step_page++;open_step=open_step_page*16;render();return true;}
    if(a>=O_VOICE&&a<=O_FX_GLOBAL){
        unsigned object=a==O_VOICE?100+selected_track:0;
        unsigned section=a==O_SEQ?1:a==O_LOCKS?3:a==O_ARP?4:a==O_TRACK_SETTINGS?5:0;
        unsigned part=a==O_VOICE?open_part:open_step;
        /* Snapshot the chosen object before closing this console. */
        if(can_send())fm1_ui_object_view(object,section,selected_track,part);
        else accepted(false);
        return true;
    }
    return false;
}
