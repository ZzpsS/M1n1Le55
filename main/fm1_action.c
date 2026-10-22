#include "fm1_action.h"
#include <string.h>
typedef struct {
  uint32_t profiles;
  const char *name;
  unsigned command, read_command, length, read_length;
  int status, read_status;
  bool writes;
  const char *kind;
  unsigned count;
  uint8_t initial[16], read[16];
  const fm1_action_field_t *fields;
} action_definition_t;
#include "fm1_action_catalog.inc"
unsigned fm1_action_count(fm1_firmware_t profile) {
  unsigned n = 0;
  if (profile >= FW_COUNT)
    return 0;
  for (unsigned i = 0; i < sizeof(definitions) / sizeof(*definitions); i++)
    if (definitions[i].profiles & (1u << profile))
      n++;
  return n;
}
bool fm1_action_build(fm1_firmware_t p, unsigned index, fm1_action_t *out) {
  if (p >= FW_COUNT)
    return false;
  for (unsigned i = 0; i < sizeof(definitions) / sizeof(*definitions); i++) {
    const action_definition_t *d = definitions + i;
    if (!(d->profiles & (1u << p)))
      continue;
    if (index-- != 0)
      continue;
    memset(out, 0, sizeof(*out));
    out->name = d->name;
    out->command = d->command;
    out->read_command = d->read_command;
    out->length = d->length;
    out->read_length = d->read_length;
    out->count = d->count;
    out->status = d->status;
    out->read_status = d->read_status;
    out->writes = d->writes;
    out->kind = d->kind;
    memcpy(out->initial, d->initial, sizeof(d->initial));
    memcpy(out->read, d->read, sizeof(d->read));
    memcpy(out->fields, d->fields, d->count * sizeof(*d->fields));
    return true;
  }
  return false;
}
static unsigned byte_for(const fm1_action_t *a, const fm1_action_field_t *f,
                         unsigned bit, bool reply, unsigned *shift) {
  unsigned off = reply ? (unsigned)f->reply : f->offset;
  unsigned b = f->shift + bit;
  *shift = b % 7;
  if (!strcmp(a->kind, "sloop_step") && f->offset == 12 && b == 7) {
    *shift = 1;
    return 11;
  }
  return off + b / 7;
}
bool fm1_action_encode(const fm1_action_t *a, const int *v, bool read,
                       uint8_t *out, unsigned *size) {
  if (!a || !out || !size || !v || a->count > FM1_ACTION_FIELDS ||
      a->length > 768)
    return false;
  if (read && !a->read_command)
    return false;
  if (!read && (!strcmp(a->kind, "fm6") || !strcmp(a->kind, "fm6_felucca")) &&
      v[0] == 2)
    return false;
  if (!strcmp(a->kind, "fm6_felucca") && v[0] == 1)
    return false;
  memcpy(out, a->initial, a->length);
  for (unsigned i = 0; i < a->count; i++) {
    const fm1_action_field_t *f = a->fields + i;
    if (v[i] < f->minimum || v[i] > f->maximum || f->bits > 32)
      return false;
    unsigned value = (unsigned)(v[i] + f->bias);
    for (unsigned bit = 0; bit < f->bits; bit++) {
      unsigned shift, at = byte_for(a, f, bit, false, &shift);
      if (at >= a->length)
        return false;
      out[at] = (out[at] & ~(1u << shift)) | (((value >> bit) & 1u) << shift);
    }
  }
  *size = a->length;
  if (!strcmp(a->kind, "song"))
    *size = 2 + out[1] * 5;
  if (!strcmp(a->kind, "chain"))
    *size = 2 + out[1];
  if (read) {
    *size = a->read_length;
    /* Same-command op selectors differ from editable payload selectors. */
    if (!strcmp(a->kind, "song"))
      out[0] = 4;
    else if (!a->read_length)
      memcpy(out, a->read, 0);
  }
  return true;
}
bool fm1_action_decode(const fm1_action_t *a, const uint8_t *data, unsigned n,
                       bool read, int *values) {
  if (!strcmp(a->kind, "open") || !strcmp(a->kind, "mmc"))
    return !read; /* Worker validates token/checksum/status, MMC has no reply.
                   */
  int status = read ? a->read_status : a->status;
  if (status >= 0 && ((unsigned)status >= n || data[status] != 0))
    return false;
  for (unsigned j = 0; j < n; j++)
    if (data[j] > 127)
      return false;
  /* Only matched read replies may populate fields; write replies often contain
   * status only. */
  if (!read)
    return true;
  for (unsigned i = 0; i < a->count; i++) {
    const fm1_action_field_t *f = a->fields + i;
    if (f->reply < 0)
      continue;
    if (!strcmp(a->kind, "song") && i > 0 && (i - 1) / 5 >= data[2])
      continue;
    unsigned value = 0;
    for (unsigned bit = 0; bit < f->bits; bit++) {
      unsigned shift, at = byte_for(a, f, bit, true, &shift);
      if (at >= n)
        return false;
      value |= ((data[at] >> shift) & 1u) << bit;
    }
    int decoded = (int)value - f->bias;
    if (decoded < f->minimum || decoded > f->maximum ||
        (f->offset < a->read_length && strcmp(a->kind, "song") &&
         decoded != values[i]))
      return false;
  }
  for (unsigned i = 0; i < a->count; i++) {
    const fm1_action_field_t *f = a->fields + i;
    if (f->reply < 0)
      continue;
    if (!strcmp(a->kind, "song") && i > 0 && (i - 1) / 5 >= data[2])
      continue;
    unsigned value = 0;
    for (unsigned bit = 0; bit < f->bits; bit++) {
      unsigned shift, at = byte_for(a, f, bit, true, &shift);
      value |= ((data[at] >> shift) & 1u) << bit;
    }
    values[i] = (int)value - f->bias;
  }
  return true;
}
