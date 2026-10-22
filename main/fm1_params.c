#include "fm1_params.h"
#include <stddef.h>

const fm1_param_t fm1_operator_params[21]={
 {"RATE 1",99,0,NULL},{"RATE 2",99,0,NULL},{"RATE 3",99,0,NULL},{"RATE 4",99,0,NULL},
 {"LEVEL 1",99,0,NULL},{"LEVEL 2",99,0,NULL},{"LEVEL 3",99,0,NULL},{"LEVEL 4",99,0,NULL},
 {"BREAKPOINT",99,0,NULL},{"LEFT DEPTH",99,0,NULL},{"RIGHT DEPTH",99,0,NULL},
 {"LEFT CURVE",3,0,"- Linear\n- Exponential\n+ Exponential\n+ Linear"},
 {"RIGHT CURVE",3,0,"- Linear\n- Exponential\n+ Exponential\n+ Linear"},
 {"RATE SCALING",7,0,NULL},{"AMP MOD SENS.",3,0,NULL},{"VELOCITY SENS.",7,0,NULL},
 {"OUTPUT",99,0,NULL},{"FREQUENCY MODE",1,0,"Ratio\nFixed Hz"},
 {"COARSE",31,0,NULL},{"FINE",99,0,NULL},{"DETUNE",14,-7,NULL}
};
const fm1_param_t fm1_global_params[19]={
 {"PITCH RATE 1",99,0,NULL},{"PITCH RATE 2",99,0,NULL},{"PITCH RATE 3",99,0,NULL},{"PITCH RATE 4",99,0,NULL},
 {"PITCH LEVEL 1",99,0,NULL},{"PITCH LEVEL 2",99,0,NULL},{"PITCH LEVEL 3",99,0,NULL},{"PITCH LEVEL 4",99,0,NULL},
 {"ALGORITHM",31,1,NULL},{"FEEDBACK",7,0,NULL},{"OSC SYNC",1,0,"Off\nOn"},
 {"LFO SPEED",99,0,NULL},{"LFO DELAY",99,0,NULL},{"PITCH MOD DEPTH",99,0,NULL},{"AMP MOD DEPTH",99,0,NULL},
 {"LFO SYNC",1,0,"Off\nOn"},{"LFO WAVE",5,0,"Triangle\nSaw down\nSaw up\nSquare\nSine\nSample & hold"},
 {"P.MOD SENS",7,0,NULL},{"TRANSPOSE",48,-24,NULL}
};
const char *fm1_effect_names[6]={"FILTER","REVERB","DELAY","DISTORTION","CHORUS","PHASER"};
const char *fm1_effect_labels[24]={
 "Enable","Type","Cutoff","Resonance","Enable","Type","Decay","Mix",
 "Enable","Decay","Rate","Mix","Enable","Gain","Tone","Level",
 "Enable","Frequency","Depth","Mix","Enable","Frequency","Depth","Mix"
};
const fm1_param_t *fm1_param_definition(unsigned p) {
    if(p<126)return &fm1_operator_params[p%21];
    if(p<145)return &fm1_global_params[p-126];
    return NULL;
}
bool fm1_param_valid(unsigned p,unsigned v) {
    const fm1_param_t *d=fm1_param_definition(p);return d && v<=d->maximum;
}
unsigned fm1_effect_max(unsigned cc,unsigned scale) {
    if(cc>=24 || (scale!=10 && scale!=100))return 0;
    if(cc%4==0)return 1;
    if(cc==1 || cc==5)return 2;
    if(cc==2)return 107;
    if(cc==3)return 10;
    return scale;
}
bool fm1_cc_packet(unsigned ch,unsigned cc,unsigned v,unsigned scale,uint8_t packet[4]) {
    if(ch>=16 || cc>=24 || (scale!=10 && scale!=100) || v>fm1_effect_max(cc,scale))return false;
    packet[0]=0x0b;packet[1]=0xb0|ch;packet[2]=cc;packet[3]=v;return true;
}
