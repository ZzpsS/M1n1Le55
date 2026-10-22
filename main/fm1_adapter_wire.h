#pragma once
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

typedef enum { FW_UNKNOWN,FW_STOCK,FW_FELUCCA,FW_SLOOP,FW_VA,FW_FOMNI,FW_X0X,
    FW_JANGADA,FW_OPEN,FW_PROBE,FW_DIAG,FW_COUNT } fm1_firmware_t;
typedef struct {
    fm1_firmware_t id;
    const char *identity,*name,*gap;
    bool fl,parameters,open;
} fm1_adapter_profile_t;
const fm1_adapter_profile_t *fm1_adapter_profile(const char *identity);
const fm1_adapter_profile_t *fm1_adapter_at(unsigned id);
bool fm1_adapter_sysex_allowed(fm1_firmware_t profile,const uint8_t *data,size_t size);
size_t fm1_fl_frame(unsigned command,const uint8_t *args,size_t count,uint8_t *out,size_t capacity);
size_t fm1_group_pack(const uint8_t *data,size_t n,uint8_t *out,size_t capacity);
size_t fm1_group_unpack(const uint8_t *data,size_t n,uint8_t *out,size_t capacity);
int fm1_v14(const uint8_t *p);
void fm1_put_v14(uint8_t *p,int value);
uint32_t fm1_u35(const uint8_t *p);
void fm1_put_u35(uint8_t *p,uint32_t value);
uint32_t fm1_adapter_crc(const void *data,size_t count);
/* A complete short MIDI message, encoded for USB cable zero. */
size_t fm1_midi_packet(const uint8_t *data,size_t n,uint8_t out[4]);
void fm1_dx7_unpack(const uint8_t packed[128],uint8_t edit[155]);
void fm1_dx7_pack(const uint8_t edit[155],uint8_t packed[128]);
size_t fm1_va_request(unsigned command,const uint8_t *args,size_t n,uint8_t *out,size_t capacity);
int fm1_va_reply(const uint8_t *frame,size_t n,unsigned kind,uint32_t argument,uint8_t *out,size_t capacity);
