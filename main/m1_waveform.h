#pragma once
#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>
/* Absolute PCM peaks over 64 equal time bins, scaled 0..255. No audio playback. */
bool m1_sample_peaks(const uint8_t *bank,size_t bytes,uint8_t peaks[8][64],uint32_t frames[8]);
