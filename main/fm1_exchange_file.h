#pragma once
#include <stdbool.h>
#include <stdint.h>
bool fm1_felucca_bank_validate(const uint8_t *data, unsigned size);
#include <stddef.h>
#define FM1_EXCHANGE_HEADER 64u
#define FM1_EXCHANGE_MAX 524288u
bool fm1_exchange_name(const char *name);
bool fm1_exchange_header(uint8_t out[64], const char *identity, unsigned object,
                         const uint8_t *data, unsigned size);
bool fm1_exchange_validate(const uint8_t *file, size_t size,
                           const char *identity, unsigned *object);
bool fm1_fsmp_validate(const uint8_t *data, unsigned size, unsigned loop_max);
