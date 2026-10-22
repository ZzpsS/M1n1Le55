#pragma once
#include "fm1_adapter_wire.h"
#define FM1_OBJECT_CAPACITY 4096
typedef struct {
  bool loaded;
  uint32_t revision, connection;
  unsigned object, size;
  uint8_t data[FM1_OBJECT_CAPACITY];
} fm1_object_state_t;
typedef struct {
  char name[72];
  unsigned offset, bits, shift;
  int minimum, maximum;
  unsigned effect, base;
} fm1_object_field_t;
/* Returns semantic fields for one section/track/step. Unrepresented bytes
 * survive unchanged. */
unsigned fm1_object_fields(fm1_firmware_t profile, unsigned object,
                           const uint8_t *data, unsigned size, unsigned section,
                           unsigned track, unsigned step,
                           fm1_object_field_t *fields, unsigned capacity);
int fm1_object_value(const uint8_t *data, const fm1_object_field_t *field);
bool fm1_object_edit(uint8_t *data, unsigned size,
                     const fm1_object_field_t *field, int value);
void fm1_adapter_object_snapshot(fm1_object_state_t *out);
bool fm1_adapter_object_read(unsigned object);
bool fm1_adapter_object_stage(const fm1_object_state_t *edited);
