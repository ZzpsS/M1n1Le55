#pragma once
#include "fm1_wire.h"
#include "esp_err.h"
#include "fm1_ota.h"
/* Diagnostic app replies; never interpreted as a BLE delivery receipt. */
unsigned fm1_host_app_last(uint8_t *data,unsigned capacity,uint32_t *sequence);

typedef struct {
    bool connected;
    bool valid_voice;
    bool can_edit;
    bool updating;
    uint32_t revision;
    int64_t last_read_us;
    char identity[32];
    char status[96];
    uint8_t voice[FM1_VOICE_SIZE];
    uint32_t rx_packets, rx_frames, bad_frames, tx_frames;
    uint16_t last_frame_size;
    uint8_t last_command;
    uint8_t fx_values[24],fx_channel,fx_scale;
    uint32_t fx_known,fx_config_revision;
    uint32_t writes_verified,writes_failed;
    uint32_t connection_id;
    bool clock_enabled;
    uint16_t clock_bpm;
    uint32_t clock_pulses,clock_missed;
} fm1_state_t;

esp_err_t fm1_host_start(void);
void fm1_host_snapshot(fm1_state_t *out);
bool fm1_host_read(void);
bool fm1_host_clock(uint32_t connection_id,unsigned bpm,bool enabled);
bool fm1_host_parameter(uint16_t parameter, uint8_t value);
bool fm1_host_effect(uint8_t cc,uint8_t value);
bool fm1_host_fx_config(uint8_t channel,uint8_t scale);
void fm1_host_update_lock(bool lock);
bool fm1_host_ota_begin(const uint8_t *image,size_t size,const char *source,const char *target,uint32_t expected_connection);
/* Connection captured by the identity reply that completed this OTA step. */
void fm1_host_ota_snapshot(fm1_ota_t *out,uint32_t *verified_connection);
/* Worker-only, serialized transaction. No retry on timeout. Identity + USB
 * generation are checked again immediately before sending. A zero prefix means
 * MIDI/SysEx send-only; success then means USB completion, not device readback. */
int fm1_host_exchange(const char *identity,uint32_t connection_id,const uint8_t *data,unsigned size,
    const uint8_t *prefix,unsigned prefix_size,uint8_t *reply,unsigned capacity,unsigned timeout_ms);
