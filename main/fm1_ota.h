#pragma once
#include "fm1_wire.h"
typedef enum { OTA_IDLE,OTA_HANDSHAKE,OTA_START_CHECK,OTA_CHECK,OTA_WAIT_LOADER,
    OTA_START_WRITE,OTA_WRITE,OTA_WAIT_REBOOT,OTA_DONE,OTA_ERROR } fm1_ota_phase_t;
typedef struct { uint8_t flags;uint32_t address,length; } fm1_ota_read_t;
typedef struct {
    fm1_ota_phase_t phase;
    const uint8_t *image;size_t size;
    char source[32],target[21],error[96];
    int64_t deadline,not_before,next_query,started,sampled;
    uint32_t elapsed_ms,check_ms,write_ms;
    bool attached,inflight,fast_transfer;
    unsigned tx_kind,head,count;
    fm1_ota_read_t queue[4],sending;
    uint32_t requests,bytes,stage_requests;
    uint32_t stage_bytes,write_bytes;
    unsigned progress;
} fm1_ota_t;
bool fm1_ota_loader(const char *identity);
bool fm1_ota_open_loader(const char *identity);
bool fm1_ota_identity_valid(const char *identity);
bool fm1_ota_active(const fm1_ota_t *o);
void fm1_ota_begin(fm1_ota_t *o,const uint8_t *image,size_t size,const char *source,const char *target,int64_t now);
void fm1_ota_fail(fm1_ota_t *o,const char *reason);
void fm1_ota_identity(fm1_ota_t *o,const char *identity,int64_t now);
void fm1_ota_connection(fm1_ota_t *o,bool attached,int64_t now);
void fm1_ota_request(fm1_ota_t *o,const uint8_t *payload,size_t size,int64_t now);
size_t fm1_ota_next(fm1_ota_t *o,uint8_t *frame,size_t capacity,int64_t now);
void fm1_ota_sent(fm1_ota_t *o,bool ok,int64_t now);
// Absolute next useful wakeup (milliseconds), including timeout while unplugged.
int64_t fm1_ota_wakeup(const fm1_ota_t *o);
void fm1_ota_sample(fm1_ota_t *o,int64_t now);
unsigned fm1_ota_progress(const fm1_ota_t *o);
const char *fm1_ota_phase_name(fm1_ota_phase_t phase);
