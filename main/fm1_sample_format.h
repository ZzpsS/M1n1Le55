#pragma once
#include <stdbool.h>
#include <stdint.h>
#include <stddef.h>
#define FM1_SAMPLE_CAPACITY 466944u
bool fm1_sample_name(const char *name);
bool fm1_sample_validate(const uint8_t *data,size_t size);
uint32_t fm1_sample_u32(const uint8_t *p);
void fm1_sample_put32(uint8_t *p,uint32_t n);
unsigned fm1_sample_encode(uint8_t *out,unsigned cmd,unsigned token,const uint8_t *data,unsigned size);
/* -1 malformed, 0 unrelated, positive decoded length including status. */
int fm1_sample_decode(const uint8_t *frame,unsigned size,unsigned cmd,unsigned token,uint8_t out[65]);
