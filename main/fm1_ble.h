#pragma once
#include <stdbool.h>
#include <stdint.h>

// Keep room for MIDI devices when nearby switches/sensors advertise first.
#define FM1_BLE_DEVICES 24
typedef enum { FM1_BLE_OFF,FM1_BLE_STARTING,FM1_BLE_IDLE,FM1_BLE_SCANNING,
    FM1_BLE_CONNECTING,FM1_BLE_DISCOVERING,FM1_BLE_READY,FM1_BLE_ERROR } fm1_ble_phase_t;
typedef struct {
    char name[32],address[18];
    int8_t rssi;
    bool midi;
} fm1_ble_device_t;
typedef struct {
    fm1_ble_phase_t phase;
    uint32_t revision,scan_id,connection_id,rx_packets,rx_bytes,tx_packets,rx_invalid;
    unsigned count;
    bool pending,testing,reserved;
    char message[128],peer[32],coprocessor[32],last_rx[73];
    fm1_ble_device_t devices[FM1_BLE_DEVICES];
} fm1_ble_state_t;

void fm1_ble_start(void);
void fm1_ble_snapshot(fm1_ble_state_t *out);
bool fm1_ble_scan(void);
bool fm1_ble_connect(unsigned index,uint32_t scan_id);
bool fm1_ble_disconnect(void);
bool fm1_ble_test_note(void);
bool fm1_ble_play_note(unsigned channel,unsigned note);
/* Read-only link diagnostics / ATT MTU negotiation on an established link. */
bool fm1_ble_link_check(bool exchange_mtu);
bool fm1_ble_fast_interval(void);
/* Single diagnostic consumer; wait for a complete BLE SysEx or timeout. */
void fm1_ble_sysex_wait(unsigned milliseconds);
/* App info (0), sample bank operations (11..16), RAM-only probe (32..35).
 * Samples 12..14 write the user sample bank; never firmware/boot partitions. */
bool fm1_ble_sysex_send(const uint8_t *data,unsigned size);
unsigned fm1_ble_sysex_last(uint8_t *data,unsigned capacity,uint32_t *sequence);
/* Prevent firmware installation while a BLE procedure/link is active. */
bool fm1_ble_reserve_update(void);
void fm1_ble_release_update(void);

bool fm1_ble_session_begin(void);
void fm1_ble_session_end(void);
