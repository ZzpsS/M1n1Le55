#include "fm1_exchange_file.h"
#include "fm1_adapter_wire.h"
#include <string.h>
bool fm1_felucca_bank_validate(const uint8_t *d, unsigned n) {
  if (!d || n < 15 || memcmp(d, "FB32", 4) || d[14] > 32 ||
      n != 15u + d[14] * 128u)
    return false;
  for (unsigned i = 4; i < 14; i++)
    if (d[i] < 32 || d[i] > 126)
      return false;
  for (unsigned i = 15; i < n; i++)
    if (d[i] > 127)
      return false;
  return true;
}
static uint32_t u32(const uint8_t *p) {
  return p[0] | (uint32_t)p[1] << 8 | (uint32_t)p[2] << 16 |
         (uint32_t)p[3] << 24;
}
static void put32(uint8_t *p, uint32_t v) {
  for (unsigned i = 0; i < 4; i++)
    p[i] = v >> (i * 8);
}
bool fm1_exchange_name(const char *name) {
  if (!name)
    return false;
  size_t n = strlen(name);
  if (n < 5 || n > 63 || strcmp(name + n - 4, ".m1x"))
    return false;
  for (size_t i = 0; i < n; i++)
    if (!((name[i] >= 'a' && name[i] <= 'z') ||
          (name[i] >= 'A' && name[i] <= 'Z') ||
          (name[i] >= '0' && name[i] <= '9') || name[i] == '-' ||
          name[i] == '_' || name[i] == '.'))
      return false;
  return !strstr(name, "..");
}
bool fm1_exchange_header(uint8_t out[64], const char *identity, unsigned object,
                         const uint8_t *data, unsigned size) {
  if (!out || !identity || strlen(identity) > 31 ||
      fm1_adapter_profile(identity)->id == FW_UNKNOWN || object > 255 ||
      size > FM1_EXCHANGE_MAX || (!data && size))
    return false;
  memset(out, 0, 64);
  memcpy(out, "M1XP", 4);
  out[4] = 1;
  out[5] = object;
  memcpy(out + 8, identity, strlen(identity));
  put32(out + 40, size);
  put32(out + 44, fm1_adapter_crc(data, size));
  put32(out + 60, fm1_adapter_crc(out, 60));
  return true;
}
bool fm1_exchange_validate(const uint8_t *f, size_t n, const char *identity,
                           unsigned *object) {
  if (!f || n < 64 || !identity || !object || memcmp(f, "M1XP", 4) ||
      f[4] != 1 || !memchr(f + 8, 0, 32) ||
      strcmp((const char *)f + 8, identity))
    return false;
  if (f[6] || f[7] || u32(f + 40) > FM1_EXCHANGE_MAX ||
      n != 64u + u32(f + 40) || u32(f + 60) != fm1_adapter_crc(f, 60) ||
      u32(f + 44) != fm1_adapter_crc(f + 64, n - 64))
    return false;
  for (unsigned i = 48; i < 60; i++)
    if (f[i])
      return false;
  *object = f[5];
  return true;
}
bool fm1_fsmp_validate(const uint8_t *d, unsigned n, unsigned loop_max) {
  if (!d || n < 512 || n > 81920 || memcmp(d, "FSMP", 4) || d[4] != 1 || d[5] ||
      !d[6] || d[6] > 16 || d[7] || u32(d + 16) != n - 512 ||
      u32(d + 20) != fm1_adapter_crc(d + 512, n - 512))
    return false;
  for (unsigned i = 0; i < d[6]; i++) {
    const uint8_t *z = d + 32 + i * 28;
    uint32_t off = u32(z), samples = u32(z + 4), start = u32(z + 8),
             end = u32(z + 12);
    if (!samples || off > n - 512 ||
        ((uint64_t)samples + 1) / 2 > n - 512 - off || !u32(z + 16) ||
        z[24] > 88 || z[25] > z[26] || z[26] > 127 || z[27] > loop_max)
      return false;
    if (z[27] && (start >= end || end > samples))
      return false;
  }
  return true;
}
