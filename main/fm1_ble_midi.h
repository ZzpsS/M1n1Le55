#pragma once
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

/* One complete channel message per BLE packet. No SysEx or running status TX. */
size_t fm1_ble_midi_packet(uint32_t milliseconds, const uint8_t *midi, size_t size,
                           uint8_t *out, size_t capacity);
/* Parses advertising TLVs; MIDI service bytes use Bluetooth little-endian order. */
bool fm1_ble_advert(const uint8_t *data, size_t size, bool *midi, char name[32]);

/* Bounded application SysEx, separate from the USB firmware update protocol. */
#define FM1_BLE_SYSEX_MAX 192
size_t fm1_ble_sysex_packet(uint32_t ms,const uint8_t *m,size_t n,size_t *offset,
                            uint8_t *out,size_t capacity);
typedef struct { uint8_t bytes[FM1_BLE_SYSEX_MAX]; size_t size; bool active; } fm1_ble_sysex_rx_t;
typedef void (*fm1_ble_sysex_cb)(const uint8_t *,size_t,void *);
bool fm1_ble_sysex_receive(fm1_ble_sysex_rx_t *rx,const uint8_t *packet,size_t n,
                           fm1_ble_sysex_cb callback,void *context);
