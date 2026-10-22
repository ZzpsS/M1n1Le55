#include "fm1_adapter_wire.h"
#include <stdlib.h>
#include <string.h>

static const fm1_adapter_profile_t profiles[FW_COUNT] = {
    {FW_UNKNOWN, "", "未识别", "等待 USB 身份；未知固件不开放控制。", false,
     false, false},
    {FW_STOCK, "FM-1_015", "M-VAVE V15",
     "原厂效果没有读回；当前音色编辑不等于整个预设库。", false, false, false},
    {FW_FELUCCA, "FM-1_916", "Felucca 1.6.1",
     "USB 编辑协议；没有与 Open 相同的蓝牙采样协议。", true, true, false},
    {FW_SLOOP, "FM-1_900", "SLOOP 2.7",
     "USB 编辑与 LIVE；使用 FSMP 分槽采样，不能使用 Open 八槽包。", true, true,
     false},
    {FW_VA, "FM-1_101", "FM-1+VA 101",
     "面板 SAVE 没有 CC；CC 设置没有设备读回。", false, false, false},
    {FW_FOMNI, "FM-1_8000400", "FoMni 0.4",
     "USB MIDI 提供演奏与时钟；没有音色参数编辑 SysEx。", false, false, false},
    {FW_X0X, "FM-1_9010100", "X0X 1.1.0",
     "FL 仅开放采样；参数、轨道和预设命令不回复。", true, false, false},
    {FW_JANGADA, "FM-1_910", "Jangada 1.0",
     "USB 编辑协议；没有 SLOOP 的 LIVE 与鼓合成命令。", true, true, false},
    {FW_OPEN, "FM-1_8000012", "FM1 Open V12",
     "MIDI 设置仅显示发送值；工程与音色支持导入导出。", false, false, true},
    {FW_PROBE, "FM-1_8000912", "BLE Probe V912",
     "实验固件：继承 Open 应用协议；不用于 OTA。", false, false, true},
    {FW_DIAG, "FM-1_8000913", "BLE Diag V913",
     "实验固件：继承 Open 应用协议并增加诊断。", false, false, true}};
const fm1_adapter_profile_t *fm1_adapter_at(unsigned id) {
  return &profiles[id < FW_COUNT ? id : 0];
}
const fm1_adapter_profile_t *fm1_adapter_profile(const char *identity) {
  if (!identity)
    return profiles;
  for (unsigned i = 1; i < FW_COUNT; i++)
    if (!strcmp(identity, profiles[i].identity))
      return profiles + i;
  if (strlen(identity) == 8 && !strncmp(identity, "FM-1_", 5) &&
      identity[5] >= '0' && identity[5] <= '9' && identity[6] >= '0' &&
      identity[6] <= '9' && identity[7] >= '0' && identity[7] <= '9' &&
      atoi(identity + 5) <= 15)
    return profiles + FW_STOCK;
  return profiles;
}
bool fm1_adapter_sysex_allowed(fm1_firmware_t profile, const uint8_t *data,
                               size_t size) {
  if (profile <= FW_UNKNOWN || profile >= FW_COUNT || !data || size < 6 ||
      data[0] != 0xf0 || data[size - 1] != 0xf7)
    return false;
  for (size_t i = 1; i + 1 < size; i++)
    if (data[i] & 128)
      return false;
  if (profiles[profile].open && size == 6 && data[1] == 0x7f &&
      (data[2] == 0 || data[2] == 0x7f) && data[3] == 6)
    return data[4] == 1 || data[4] == 2 || data[4] == 3 || data[4] == 6 ||
           data[4] == 7 || data[4] == 9;
  if (profiles[profile].fl && !memcmp(data + 1,
                                      "\x7d"
                                      "FL",
                                      3)) {
    unsigned cmd = data[4];
    if (profile == FW_X0X)
      return cmd == 1 || cmd == 25 || (cmd >= 11 && cmd <= 16);
    if (cmd >= 1 && cmd <= 32)
      return cmd != 23 && cmd != 24 && cmd != 26 && cmd != 32;
    if (profile == FW_FELUCCA)
      return (cmd >= 33 && cmd <= 38) || (cmd >= 64 && cmd <= 81);
    if (profile == FW_SLOOP)
      return (cmd >= 33 && cmd <= 42) || (cmd >= 68 && cmd <= 77);
    return (cmd >= 34 && cmd <= 36) || (cmd >= 68 && cmd <= 72);
  }
  if (profiles[profile].open && size >= 10 &&
      !memcmp(data + 1,
              "\x7d"
              "FM\x02",
              4)) {
    unsigned cmd = data[5];
    return cmd <= 20 || (profile >= FW_PROBE && cmd >= 32 && cmd <= 35);
  }
  if (profile == FW_VA && size >= 8 && !memcmp(data + 1, "\x43\x00\x7d", 3)) {
    unsigned cmd = data[4];
    return (cmd == 0x10 && size == 8) || (cmd == 0x11 && size == 14) ||
           (cmd == 4 && size == 231) || (cmd == 0x22 && size == 162) ||
           (cmd == 0x21 && size == 74);
  }
  /* Yamaha patch changes are separate from the vendor's firmware-writing
   * protocol. */
  if ((profile == FW_STOCK || profile == FW_VA || profiles[profile].open ||
       profiles[profile].parameters) &&
      data[1] == 0x43)
    return (size == 7 && (data[2] & 0xf0) == 0x10 && data[3] <= 1) ||
           (size == 163 && data[2] < 16 && data[3] == 0 && data[4] == 1 &&
            data[5] == 27);
  return false;
}
size_t fm1_fl_frame(unsigned command, const uint8_t *args, size_t count,
                    uint8_t *out, size_t capacity) {
  if (command > 127 || count + 6 > capacity || (!args && count))
    return 0;
  for (size_t i = 0; i < count; i++)
    if (args[i] > 127)
      return 0;
  memcpy(out,
         "\xf0\x7d"
         "FL",
         4);
  out[4] = command;
  if (count)
    memcpy(out + 5, args, count);
  out[count + 5] = 0xf7;
  return count + 6;
}
int fm1_v14(const uint8_t *p) { return (p[0] | p[1] << 7) - 8192; }
void fm1_put_v14(uint8_t *p, int v) {
  unsigned u = (unsigned)(v + 8192);
  p[0] = u & 127;
  p[1] = (u >> 7) & 127;
}
uint32_t fm1_u35(const uint8_t *p) {
  uint32_t v = 0;
  for (unsigned i = 0; i < 5; i++)
    v |= (uint32_t)(p[i] & (i == 4 ? 15 : 127)) << (7 * i);
  return v;
}
void fm1_put_u35(uint8_t *p, uint32_t v) {
  for (unsigned i = 0; i < 5; i++)
    p[i] = (v >> (7 * i)) & 127;
}
size_t fm1_group_pack(const uint8_t *data, size_t n, uint8_t *out, size_t cap) {
  if (!data || !out || n + (n + 6) / 7 > cap)
    return 0;
  size_t at = 0;
  for (size_t i = 0; i < n; i += 7) {
    size_t mark = at++;
    out[mark] = 0;
    for (size_t j = 0; j < 7 && i + j < n; j++) {
      out[mark] |= (data[i + j] >> 7) << j;
      out[at++] = data[i + j] & 127;
    }
  }
  return at;
}
size_t fm1_group_unpack(const uint8_t *data, size_t n, uint8_t *out,
                        size_t cap) {
  if (!data || !out || !n || n % 8 == 1)
    return 0;
  size_t at = 0;
  for (size_t i = 0; i < n;) {
    unsigned mark = data[i++];
    if (mark > 127)
      return 0;
    unsigned remain = n - i;
    if (remain < 7 && (mark >> remain))
      return 0;
    for (unsigned j = 0; j < 7 && i < n; j++) {
      if (at == cap || data[i] > 127)
        return 0;
      out[at++] = data[i++] | ((mark >> j) & 1) << 7;
    }
  }
  return at;
}
uint32_t fm1_adapter_crc(const void *data, size_t n) {
  const uint8_t *p = data;
  uint32_t c = ~0u;
  while (n--) {
    c ^= *p++;
    for (unsigned i = 0; i < 8; i++)
      c = (c >> 1) ^ ((c & 1) ? 0xedb88320u : 0);
  }
  return ~c;
}
size_t fm1_midi_packet(const uint8_t *m, size_t n, uint8_t out[4]) {
  if (!m || !out || !n)
    return 0;
  unsigned cin = m[0] >> 4, want = (cin == 12 || cin == 13) ? 2 : 3;
  if (m[0] >= 0xf8) {
    if (m[0] != 0xf8 && m[0] != 0xfa && m[0] != 0xfb && m[0] != 0xfc)
      return 0;
    cin = 15;
    want = 1;
  } else if (m[0] == 0xf2) {
    cin = 3;
    want = 3;
  } else if (cin < 8 || cin > 14)
    return 0;
  if (n != want)
    return 0;
  for (size_t i = 1; i < n; i++)
    if (m[i] > 127)
      return 0;
  out[0] = cin;
  out[1] = m[0];
  out[2] = n > 1 ? m[1] : 0;
  out[3] = n > 2 ? m[2] : 0;
  return 4;
}
void fm1_dx7_unpack(const uint8_t p[128], uint8_t e[155]) {
  for (unsigned op = 0; op < 6; op++) {
    const uint8_t *s = p + 17 * op;
    uint8_t *d = e + 21 * op;
    memcpy(d, s, 11);
    d[11] = s[11] & 3;
    d[12] = (s[11] >> 2) & 3;
    d[13] = s[12] & 7;
    d[20] = s[12] >> 3;
    d[14] = s[13] & 3;
    d[15] = s[13] >> 2;
    d[16] = s[14];
    d[17] = s[15] & 1;
    d[18] = s[15] >> 1;
    d[19] = s[16];
  }
  memcpy(e + 126, p + 102, 9);
  e[135] = p[111] & 7;
  e[136] = p[111] >> 3;
  memcpy(e + 137, p + 112, 4);
  e[141] = p[116] & 1;
  e[142] = (p[116] >> 1) & 7;
  e[143] = p[116] >> 4;
  e[144] = p[117];
  memcpy(e + 145, p + 118, 10);
}
void fm1_dx7_pack(const uint8_t e[155], uint8_t p[128]) {
  for (unsigned op = 0; op < 6; op++) {
    const uint8_t *s = e + 21 * op;
    uint8_t *d = p + 17 * op;
    memcpy(d, s, 11);
    d[11] = (s[11] & 3) | ((s[12] & 3) << 2);
    d[12] = (s[13] & 7) | ((s[20] & 15) << 3);
    d[13] = (s[14] & 3) | ((s[15] & 7) << 2);
    d[14] = s[16];
    d[15] = (s[17] & 1) | ((s[18] & 31) << 1);
    d[16] = s[19];
  }
  memcpy(p + 102, e + 126, 9);
  p[111] = (e[135] & 7) | ((e[136] & 1) << 3);
  memcpy(p + 112, e + 137, 4);
  p[116] = (e[141] & 1) | ((e[142] & 7) << 1) | ((e[143] & 7) << 4);
  p[117] = e[144];
  memcpy(p + 118, e + 145, 10);
}
size_t fm1_va_request(unsigned command, const uint8_t *args, size_t n,
                      uint8_t *out, size_t capacity) {
  if (command > 127 || n + 7 > capacity || (!args && n))
    return 0;
  memcpy(out, "\xf0\x43\x00\x7d", 4);
  out[4] = command;
  unsigned sum = command ^ 255u;
  for (size_t i = 0; i < n; i++) {
    if (args[i] > 127)
      return 0;
    out[5 + i] = args[i];
    sum += args[i] ^ 255u;
  }
  out[5 + n] = sum & 127;
  out[6 + n] = 0xf7;
  return n + 7;
}
int fm1_va_reply(const uint8_t *f, size_t n, unsigned kind, uint32_t argument,
                 uint8_t *out, size_t cap) {
  if (!f || n < 13 || f[0] != 0xf0 || f[n - 1] != 0xf7)
    return -1;
  uint8_t raw[300];
  unsigned bits = 0, used = 0;
  uint32_t acc = 0;
  for (size_t i = 1; i < n - 1; i++) {
    if (f[i] > 127)
      return -1;
    acc |= (uint32_t)f[i] << bits;
    bits += 7;
    if (bits >= 8) {
      if (used == sizeof(raw))
        return -1;
      raw[used++] = acc & 255;
      acc >>= 8;
      bits -= 8;
    }
  }
  if (acc || bits >= 7 || used < 10 || raw[0] != 0x7d || raw[1] != kind ||
      raw[2])
    return -1;
  uint32_t arg = raw[3] | (uint32_t)raw[4] << 8 | (uint32_t)raw[5] << 16 |
                 (uint32_t)raw[6] << 24;
  unsigned bytes = raw[7] | raw[8] << 8;
  if (arg != argument || used != bytes + 10 || bytes > cap)
    return -1;
  unsigned sum = 0;
  for (unsigned i = 0; i < used; i++)
    sum += raw[i];
  if ((sum & 255) != 255)
    return -1;
  if (bytes)
    memcpy(out, raw + 9, bytes);
  return (int)bytes;
}
