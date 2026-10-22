#pragma once
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#define FM1_VOICE_SIZE 155
#define FM1_FRAME_MAX 1024
typedef struct {
    uint8_t data[FM1_FRAME_MAX];
    size_t used;
    bool active;
} fm1_stream_t;

size_t fm1_identity_request(uint8_t *out, size_t capacity);
size_t fm1_encode(uint8_t command,const uint8_t *payload,size_t size,uint8_t *out,size_t capacity);
size_t fm1_voice_request(uint8_t *out, size_t capacity);
size_t fm1_parameter_request(uint16_t parameter, uint8_t value, uint8_t channel,
                             uint8_t *out, size_t capacity);
bool fm1_decode(const uint8_t *frame, size_t size, uint8_t *command,
                uint8_t *payload, size_t *payload_size);
// One USB-MIDI event packet at a time, cable 0 only. Returned frame lives in stream.data.
size_t fm1_stream_packet(fm1_stream_t *stream, const uint8_t packet[4]);
size_t fm1_usb_packets(const uint8_t *sysex, size_t size, uint8_t *out, size_t capacity);
