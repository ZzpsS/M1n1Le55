#pragma once
#include "fm1_adapter_wire.h"
#define FM1_ACTION_FIELDS 192
typedef struct {
  char name[96];
  int minimum, maximum, initial;
  unsigned offset, bits, shift;
  int bias, reply;
} fm1_action_field_t;
typedef struct {
  const char *name;
  unsigned command, read_command, length, read_length, count;
  int status, read_status;
  bool writes;
  const char *kind;
  uint8_t initial[768], read[16];
  fm1_action_field_t fields[FM1_ACTION_FIELDS];
} fm1_action_t;
unsigned fm1_action_count(fm1_firmware_t profile);
bool fm1_action_build(fm1_firmware_t profile, unsigned index,
                      fm1_action_t *out);
bool fm1_action_encode(const fm1_action_t *a, const int *values, bool read,
                       uint8_t *out, unsigned *size);
bool fm1_action_decode(const fm1_action_t *a, const uint8_t *data,
                       unsigned size, bool read, int *values);
void fm1_action_summary(fm1_firmware_t profile, unsigned command,
                        const uint8_t *data, unsigned size, char *out,
                        unsigned capacity);
