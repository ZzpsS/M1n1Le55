#pragma once
#include <stdbool.h>
#include <stdint.h>

typedef struct { const char *name; uint8_t maximum; int8_t offset; const char *options; } fm1_param_t;
extern const fm1_param_t fm1_operator_params[21];
extern const fm1_param_t fm1_global_params[19];
extern const char *fm1_effect_names[6];
extern const char *fm1_effect_labels[24];
const fm1_param_t *fm1_param_definition(unsigned parameter);
bool fm1_param_valid(unsigned parameter,unsigned value);
unsigned fm1_effect_max(unsigned cc,unsigned scale);
bool fm1_cc_packet(unsigned channel,unsigned cc,unsigned value,unsigned scale,uint8_t packet[4]);
