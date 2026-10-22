#include "fm1_object.h"
#include "fm1_params.h"
#include <stdio.h>
#include <string.h>
static void add(fm1_object_field_t *f, unsigned cap, unsigned *n,
                const char *name, unsigned off, unsigned bits, unsigned shift,
                int lo, int hi) {
  if (*n >= cap)
    return;
  fm1_object_field_t *p = f + (*n)++;
  snprintf(p->name, sizeof(p->name), "%s", name);
  p->offset = off;
  p->bits = bits;
  p->shift = shift;
  p->minimum = lo;
  p->maximum = hi;
  p->effect = p->base = 0;
}
static bool va_extended(const uint8_t *s) { return (s[29] & 0xe0) == 0x60; }
static bool va_release_has(const uint8_t *s, unsigned note) {
  unsigned off = va_extended(s) ? 14 : 11, cap = va_extended(s) ? 6
                                                 : s[31] < 9    ? s[31]
                                                                : 9;
  for (unsigned i = 0; i < cap; i++)
    if (s[off + i] == note)
      return true;
  return false;
}
static bool va_release_add(uint8_t *s, unsigned note) {
  if (va_release_has(s, note))
    return true;
  if (va_extended(s)) {
    for (unsigned i = 14; i < 20; i++)
      if (s[i] >= 128) {
        s[i] = note;
        s[31] = 9;
        return true;
      }
    return false;
  }
  if (s[31] >= 9)
    return false;
  s[11 + s[31]++] = note;
  return true;
}
static bool va_extend(uint8_t *data, unsigned size, unsigned base) {
  if (size != 2565 || base < 5 || (base - 5) % 32 || base > 5 + 63 * 32)
    return false;
  uint8_t *s = data + base;
  if (va_extended(s))
    return true;
  uint8_t copy[2048], ends[64][9];
  memcpy(copy, data + 5, sizeof(copy));
  for (unsigned i = 0; i < 64; i++)
    for (unsigned j = 0; j < 9; j++) {
      uint8_t *t = copy + i * 32;
      unsigned end = 64;
      if (j < t[30] && t[j] < 128)
        for (unsigned k = i; k < 64; k++)
          if (va_release_has(copy + k * 32, t[j])) {
            end = k;
            break;
          }
      if (end == 64)
        end = data[3] > i ? data[3] - 1 : i;
      ends[i][j] = end < 64 ? end : 63;
    }
  uint8_t *target = copy + base - 5;
  unsigned flags = (target[29] & 0xc0) == 0x40 ? target[29] & 0x3f : 0;
  target[29] = 0x70 | flags;
  memset(target + 11, 255, 9);
  target[31] = 0;
  for (unsigned i = 0; i < 64; i++) {
    uint8_t *t = copy + i * 32;
    memset(t + (va_extended(t) ? 14 : 11), 255, va_extended(t) ? 6 : 9);
    t[31] = 0;
  }
  for (unsigned pass = 0; pass < 2; pass++)
    for (unsigned i = 0; i < 64; i++) {
      uint8_t *t = copy + i * 32;
      for (unsigned j = 0; j < 9 && j < t[30]; j++) {
        if (t[j] >= 128 || ((ends[i][j] != i) ? 1u : 0u) != pass)
          continue;
        bool added = false;
        for (int k = ends[i][j]; k >= (int)i; k--)
          if (va_release_add(copy + k * 32, t[j])) {
            added = true;
            break;
          }
        for (unsigned k = ends[i][j] + 1; !added && k < 64; k++)
          added = va_release_add(copy + k * 32, t[j]);
        if (!added)
          return false;
      }
    }
  memcpy(data + 5, copy, sizeof(copy));
  return true;
}
int fm1_object_value(const uint8_t *data, const fm1_object_field_t *f) {
  if (f->effect == 5) {
    unsigned which = f->offset - f->base - 11;
    if (!va_extended(data + f->base) || data[f->offset] == 255)
      return which == 1 ? 100 : 0;
    return (data[f->offset] & 127) - (which == 2 ? 64 : 0);
  }
  unsigned v = 0;
  for (unsigned b = 0; b < f->bits; b++)
    v |= ((data[f->offset + (f->shift + b) / 8] >> ((f->shift + b) % 8)) & 1u)
         << b;
  return f->minimum < 0 && f->bits == 8 && v >= 128 ? (int)v - 256 : (int)v;
}
bool fm1_object_edit(uint8_t *data, unsigned size, const fm1_object_field_t *f,
                     int v) {
  if (!data || !f || f->bits > 16 || v < f->minimum || v > f->maximum ||
      f->offset + (f->shift + f->bits + 7) / 8 > size)
    return false;
  if (f->effect == 5) {
    if (!va_extend(data, size, f->base))
      return false;
    unsigned which = f->offset - f->base - 11;
    data[f->offset] = (which == 0 && v < 5) || (which == 1 && v >= 100) ||
                              (which == 2 && v == 0)
                          ? 255
                          : 128 | (v + (which == 2 ? 64 : 0));
    return true;
  }
  for (unsigned b = 0; b < f->bits; b++) {
    unsigned at = f->offset + (f->shift + b) / 8, shift = (f->shift + b) % 8;
    data[at] = (data[at] & ~(1u << shift)) | (((unsigned)v >> b & 1u) << shift);
  }
  if (f->effect == 1 && f->base + 223 <= size) {
    uint8_t *voice = data + f->base;
    voice[3] = 1;
    if (voice[0] == 1 && f->offset == f->base + 16)
      voice[2] = v;
    if (voice[0] == 3 && f->offset == f->base + 11)
      voice[2] = v;
  }
  if (f->effect == 2 && f->base + 32 <= size)
    data[f->base + 29] = (data[f->base + 29] & 0x3f) | 0x40;
  if (f->effect == 3 && (size == 36 || size == 40)) {
    uint32_t crc = fm1_adapter_crc(data, size - 4);
    for (unsigned i = 0; i < 4; i++)
      data[size - 4 + i] = crc >> (8 * i);
  }
  return true;
}
/* DX7 VMEM packed fields retain all unedited/reserved bits. */
static void packed_fm(fm1_object_field_t *f, unsigned cap, unsigned *n) {
  static const unsigned off[21] = {0,  1,  2,  3,  4,  5,  6,  7,  8,  9, 10,
                                   11, 11, 12, 13, 13, 14, 15, 15, 16, 12};
  static const unsigned bits[21] = {7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7,
                                    2, 2, 3, 2, 3, 7, 1, 5, 7, 4};
  static const unsigned shift[21] = {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
                                     0, 2, 0, 0, 2, 0, 0, 1, 0, 3};
  char name[72];
  for (unsigned i = 0; i < 145; i++) {
    const fm1_param_t *p = fm1_param_definition(i);
    unsigned at, b = 7, s = 0;
    if (i < 126) {
      unsigned k = i % 21;
      at = i / 21 * 17 + off[k];
      b = bits[k];
      s = shift[k];
      snprintf(name, sizeof(name), "OP%u · %s", 6 - i / 21, p->name);
    } else {
      snprintf(name, sizeof(name), "%s", p->name);
      at = i <= 134   ? 102 + i - 126
           : i <= 136 ? 111
           : i <= 140 ? 112 + i - 137
           : i <= 143 ? 116
                      : 117;
      if (i == 135) {
        b = 3;
      }
      if (i == 136) {
        b = 1;
        s = 3;
      }
      if (i == 141)
        b = 1;
      if (i == 142) {
        b = 3;
        s = 1;
      }
      if (i == 143) {
        b = 3;
        s = 4;
      }
    }
    add(f, cap, n, name, at, b, s, 0, p->maximum);
  }
  for (unsigned i = 0; i < 10; i++) {
    snprintf(name, sizeof(name), "名称字符 %u (ASCII)", i + 1);
    add(f, cap, n, name, 118 + i, 8, 0, 0, 126);
  }
}
unsigned fm1_object_fields(fm1_firmware_t profile, unsigned object,
                           const uint8_t *data, unsigned size, unsigned section,
                           unsigned track, unsigned step, fm1_object_field_t *f,
                           unsigned cap) {
  unsigned n = 0;
  char label[72];
  if (!data || !f || !cap)
    return 0;
#define FIELD(name, off, lo, hi) add(f, cap, &n, name, off, 8, 0, lo, hi)
#define BIT(name, off, bits, shift, lo, hi)                                    \
  add(f, cap, &n, name, off, bits, shift, lo, hi)
  bool open = fm1_adapter_at(profile)->open;
  if (profile == FW_SLOOP && object == 9 && size == 1464 &&
      !memcmp(data, "DSU1", 4)) {
    unsigned base = 8 + (track % 4) * 364;
    if (section == 0) {
      for (unsigned i = 0; i < 8; i++) {
        snprintf(label, sizeof(label), "鼓组 %u 名称字符 %u", track % 4 + 1,
                 i + 1);
        FIELD(label, base + i, 0, 126);
      }
      FIELD("Crush", base + 8, 0, 255);
      FIELD("工厂来源", base + 9, 0, 31);
    } else {
      static const char *names[] = {
          "Wave",        "Noise / Clap", "Pitch",
          "Fine",        "Bend",         "Bend Time",
          "Hold",        "Decay",        "Tone Level",
          "Tone 2",      "Tone 2 Level", "Click",
          "Noise Level", "Noise Hold",   "Noise Decay",
          "Filter",      "Cutoff",       "Filter Envelope",
          "High Pass",   "Chip",         "Drive",
          "Level"};
      static const unsigned max[] = {5,   20,  127, 15,  96,  127, 255, 127,
                                     127, 255, 127, 127, 127, 255, 127, 255,
                                     127, 127, 127, 127, 127, 255};
      unsigned off = base + 12 + (step % 16) * 22;
      for (unsigned i = 0; i < 22; i++) {
        if (i == 1) {
          BIT("Noise", off + i, 4, 0, 0, 4);
          BIT("Clap", off + i, 1, 4, 0, 1);
        } else if (i == 15) {
          BIT("Filter Mode", off + i, 2, 0, 0, 3);
          BIT("Filter All", off + i, 1, 2, 0, 1);
          BIT("Resonance", off + i, 5, 3, 0, 31);
        } else
          FIELD(names[i], off + i, i == 17 ? -128 : 0, max[i]);
      }
    }
  }
  if (profile == FW_SLOOP && object == 10 && size == 40) {
    unsigned off = (step % 16) * 2;
    BIT("学习映射 CC · 0为空", off, 7, 0, 0, 127);
    BIT("映射轨道", off, 2, 7, 0, 3);
    BIT("参数代码 · 公共ID+1 / 引擎120+k", off, 7, 9, 0, 127);
    for (unsigned i = 0; i < 3; i++) {
      snprintf(label, sizeof(label), "轨道 %u 通道 (0关闭)", i + 1);
      FIELD(label, 32 + i, 0, 16);
    }
    BIT("鼓轨通道", 35, 7, 0, 0, 16);
    BIT("其他通道 · 0选中轨 1忽略", 35, 1, 7, 0, 1);
    FIELD("弯音范围", 36, 1, 12);
  }
  if (profile == FW_SLOOP && object == 11 &&
      ((size == 36 && !memcmp(data, "THM1", 4)) ||
       (size == 40 && !memcmp(data, "THM2", 4)))) {
    for (unsigned i = 0; i < 8; i++) {
      snprintf(label, sizeof(label), "主题名称字符 %u", i + 1);
      FIELD(label, 4 + i, 0, 126);
    }
    const char *names[] = {"线条", "暗色",   "标签",    "次级",
                           "数值", "轨道 1", "轨道 2",  "轨道 3",
                           "鼓轨", "背景",   "触控强调"};
    for (unsigned i = 0; i < (size == 40 ? 11u : 10u); i++) {
      snprintf(label, sizeof(label), "%s 红", names[i]);
      BIT(label, 12 + i * 2, 5, 11, 0, 31);
      snprintf(label, sizeof(label), "%s 绿", names[i]);
      BIT(label, 12 + i * 2, 6, 5, 0, 63);
      snprintf(label, sizeof(label), "%s 蓝", names[i]);
      BIT(label, 12 + i * 2, 5, 0, 0, 31);
    }
    for (unsigned i = 0; i < n; i++)
      f[i].effect = 3;
  }
  if (open && object == 0 && size == 4064 && track < 8 && step < 64) {
    if (section == 0) {
      BIT("速度 BPM", 3832, 16, 0, 30, 300);
      BIT("所选轨道静音", 3834, 1, track, 0, 1);
      BIT("所选轨道独奏", 3835, 1, track, 0, 1);
      const char *fx[] = {"混响衰减", "延迟时间", "延迟反馈", "效果返回"};
      for (unsigned i = 0; i < 4; i++)
        FIELD(fx[i], 3848 + i, 0, 127);
      FIELD("Swing", 3853, 0, 50);
      FIELD("节拍器", 3854, 0, 2);
      FIELD("预备拍", 3855, 0, 1);
      for (unsigned i = 0; i < 12; i++) {
        snprintf(label, sizeof(label), "工程名称字符 %u (ASCII)", i + 1);
        FIELD(label, 3836 + i, 32, 126);
      }
    } else if (section == 1) {
      unsigned off = (track * 64 + step) * 4, type = data[2048 + track * 223];
      if (type == 2 || type == 5)
        BIT("概率 · 0总是 1–7分之八", off, 3, 4, 0, 7);
      else
        FIELD("音符", off, 0, 127);
      FIELD("力度 (0关闭)", off + 1, 0, 127);
      if (type == 2 || type == 5) {
        for (unsigned i = 0; i < (type == 2 ? 11u : 8u); i++) {
          snprintf(label, sizeof(label), "鼓 / Pad %u", i + 1);
          BIT(label, off + 2, 1, i, 0, 1);
        }
      } else {
        BIT("和弦 · 0单音", off + 2, 5, 0, 0, 16);
        BIT("Gate · 0默认", off + 2, 4, 5, 0, 15);
        BIT("概率 · 0总是", off + 2, 3, 9, 0, 7);
        BIT("延音", off + 2, 1, 12, 0, 1);
      }
    } else if (section == 2) {
      unsigned off = 3856 + (step % 16) * 8;
      FIELD("和弦数量 · 0空或2–8", off, 0, 8);
      for (unsigned i = 1; i < 8; i++) {
        snprintf(label, sizeof(label), "音程 %u · 升序，未用填0", i);
        FIELD(label, off + i, 0, 127);
      }
    } else if (section == 3) {
      unsigned off = 3984 + (step % 16) * 4;
      FIELD("轨道 (255空)", off, 0, 255);
      FIELD("步进", off + 1, 0, 63);
      FIELD("参数 ID", off + 2, 0, 144);
      FIELD("参数值", off + 3, 0, 127);
    } else if (section == 4) {
      unsigned off = 4048 + track * 2;
      const char *names[] = {"速度", "方向", "音域", "Gate"};
      for (unsigned i = 0; i < 4; i++)
        BIT(names[i], off, 2, i * 2, 0, 3);
      BIT("Swing", off + 1, 6, 0, 0, 50);
      BIT("保持", off + 1, 1, 6, 0, 1);
      BIT("启用", off + 1, 1, 7, 0, 1);
    } else if (section == 5) {
      unsigned off = 2048 + track * 223;
      FIELD("轨道音量", off + 1, 0, 127);
      FIELD("图案页数 · 0=64步", off + 221, 0, 3);
      BIT("混响发送", off + 220, 4, 0, 0, 15);
      BIT("延迟发送", off + 220, 4, 4, 0, 15);
      FIELD("包络 / LFO 启用", off + 8, 0, 1);
      for (unsigned i = 0; i < 4; i++) {
        snprintf(label, sizeof(label), "%s %u",
                 data[off] == 0 ? "ADSR" : "LFO 设置", i + 1);
        FIELD(label, off + 4 + i, 0, 100);
      }
      if (data[off] == 0) {
        FIELD("算子开关位图", off + 9, 0, 63);
        FIELD("单音模式", off + 10, 0, 1);
      }
    }
  }
  if (open && object >= 200 && object < 208 && size == 156) {
    for (unsigned i = 0; i < 145; i++) {
      const fm1_param_t *p = fm1_param_definition(i);
      if (i < 126)
        snprintf(label, sizeof(label), "OP%u · %s", 6 - i / 21, p->name);
      else
        snprintf(label, sizeof(label), "%s", p->name);
      FIELD(label, i, 0, p->maximum);
    }
    for (unsigned i = 0; i < 10; i++) {
      snprintf(label, sizeof(label), "音色名称字符 %u (ASCII)", i + 1);
      FIELD(label, 145 + i, 32, 126);
    }
  }
  if (open && object >= 100 && object < 108 && size == 223) {
    FIELD("轨道音量", 1, 0, 127);
    BIT("混响发送", 220, 4, 0, 0, 15);
    BIT("延迟发送", 220, 4, 4, 0, 15);
    if (data[0] == 0) {
      for (unsigned i = 0; i < 145; i++) {
        const fm1_param_t *p = fm1_param_definition(i);
        if (i < 126)
          snprintf(label, sizeof(label), "OP%u · %s", 6 - i / 21, p->name);
        else
          snprintf(label, sizeof(label), "%s", p->name);
        FIELD(label, 11 + i, 0, p->maximum);
      }
    } else if (data[0] == 2) {
      static const unsigned counts[] = {8, 10, 9, 9, 9, 9, 10, 8, 8, 8, 8, 3};
      static const char *parts[] = {"BD", "SD", "LT", "MT", "HT", "RS",
                                    "CP", "CB", "CY", "OH", "CH", "KIT"};
      static const char *names[12][10] = {
          {"Level", "Tone", "Decay", "Tune", "Attack", "Drive", "Dist", "Pan"},
          {"Level", "Tone", "Snappy", "Tune", "Decay", "Drive", "Dist", "Rev",
           "Dly", "Pan"},
          {"Level", "Tune", "Decay", "Sound", "Drive", "Dist", "Rev", "Dly",
           "Pan"},
          {"Level", "Tune", "Decay", "Sound", "Drive", "Dist", "Rev", "Dly",
           "Pan"},
          {"Level", "Tune", "Decay", "Sound", "Drive", "Dist", "Rev", "Dly",
           "Pan"},
          {"Level", "Tune", "Decay", "Sound", "Drive", "Dist", "Rev", "Dly",
           "Pan"},
          {"Level", "Tune", "Decay", "Attack", "Sound", "Drive", "Dist", "Rev",
           "Dly", "Pan"},
          {"Level", "Tune", "Decay", "Drive", "Dist", "Rev", "Dly", "Pan"},
          {"Level", "Decay", "Tune", "Drive", "Dist", "Rev", "Dly", "Pan"},
          {"Level", "Decay", "Tune", "Drive", "Dist", "Rev", "Dly", "Pan"},
          {"Level", "Tune", "Decay", "Drive", "Dist", "Rev", "Dly", "Pan"},
          {"Level", "Accent", "Choke"}};
      unsigned part = step % 12, off = 22 + (track % 2) * 99;
      for (unsigned i = 0; i < part; i++)
        off += counts[i];
      if (part < 11) {
        snprintf(label, sizeof(label), "%s 变体开关", parts[part]);
        FIELD(label, 11 + part, 0, 1);
      }
      for (unsigned i = 0; i < counts[part]; i++) {
        const char *name = names[part][i];
        snprintf(label, sizeof(label), "%s %s · 变体 %u", parts[part], name,
                 track % 2 + 1);
        FIELD(label, off + i, 0,
              !strcmp(name, "Dist")    ? 6
              : !strcmp(name, "Sound") ? 1
              : !strcmp(name, "Choke") ? 2
                                       : 127);
      }
    } else {
      static const char *names[6][12] = {
          {0},
          {"Cutoff", "Resonance", "Env Mod", "Decay", "Accent", "Wave", "Tune",
           "Volume", "Drive", "Drive type", "Slide", "Accent decay"},
          {0},
          {"Mode", "Noise", "Tune", "Release"},
          {"Decay", "Brightness", "Tune", "Release"},
          {"Level", "Tune", "Length", "Choke", "Variation", "Velocity LPF"}};
      static const uint8_t maxima[6][12] = {
          {0},
          {127, 127, 127, 127, 127, 1, 127, 127, 127, 2, 127, 127},
          {0},
          {3, 31, 48, 127},
          {127, 127, 48, 127},
          {127, 48, 127, 4, 3, 127}};
      unsigned type = data[0], count = type == 1                ? 12
                                       : type == 5              ? 6
                                       : type == 2 || type >= 6 ? 0
                                                                : 4;
      for (unsigned i = 0; i < count; i++) {
        unsigned off = 11 + (type == 5 ? (step % 8) * 6 : 0) + i;
        FIELD(names[type][i], off, 0, maxima[type][i]);
      }
    }
  }
  if (profile == FW_VA && object < 128 && size == 187) {
    if (section == 0)
      packed_fm(f, cap, &n);
    else {
      static const char *record[59] = {"Filter Cutoff",
                                       "Filter Resonance",
                                       "Mixer / 标记",
                                       "Reverb Decay",
                                       "Reverb Mix",
                                       "Bitcrush / 标记",
                                       "Delay Feedback",
                                       "Delay Rate",
                                       "Delay Mix",
                                       "Dist Gain",
                                       "Dist Tone",
                                       "Dist Level",
                                       "Chorus Rate",
                                       "Chorus Depth",
                                       "Chorus Mix",
                                       "Phaser Rate",
                                       "Phaser Depth",
                                       "Phaser Mix",
                                       "引擎 · 165 FM / 90 VA / 195 8-Bit",
                                       "OSC Wave",
                                       "Super",
                                       "Detune",
                                       "Drift",
                                       "VA Cutoff / 8-Bit ARP",
                                       "VA Resonance / 8-Bit ARP",
                                       "VA Envelope / 8-Bit ARP",
                                       "VA Tracking / Key",
                                       "Reverb Order",
                                       "Reverb Type",
                                       "Reverb On",
                                       "Delay Order",
                                       "Delay Type",
                                       "Delay On",
                                       "Dist Order",
                                       "Dist Type",
                                       "Dist On",
                                       "Chorus Order",
                                       "Chorus Type",
                                       "Chorus On",
                                       "Phaser Order",
                                       "Phaser Type",
                                       "Phaser On",
                                       "Filter Order",
                                       "Filter Type",
                                       "Filter On",
                                       "VA Filter / 8-Bit ARP 45",
                                       "VA Filter / 8-Bit ARP 46",
                                       "VA Filter / 8-Bit ARP 47",
                                       "VA Filter / 8-Bit ARP 48",
                                       "VA Filter / 8-Bit ARP 49",
                                       "VA Filter / 8-Bit ARP 50",
                                       "VA Filter / 8-Bit ARP 51",
                                       "Knob 1",
                                       "Knob 2 / Envelope",
                                       "Attack",
                                       "Decay",
                                       "Sustain",
                                       "Release",
                                       "Setting 58"};
      for (unsigned i = 0; i < 59; i++)
        FIELD(record[i], 128 + i, 0, 255);
    }
  }
  if (profile == FW_VA && object >= 128 && object < 144 && size == 2565 &&
      step < 64) {
    if (section == 0) {
      FIELD("Gate %", 0, 5, 100);
      FIELD("Swing %", 1, 50, 75);
      FIELD("音符时值", 2, 0, 9);
      FIELD("图案长度", 3, 1, 64);
      BIT("图案链启用", 4, 1, 7, 0, 1);
      BIT("下一个图案 · 0–15", 4, 4, 0, 0, 15);
      BIT("重复 · 0–7 = 1/2/3/4/6/8/12/16", 4, 3, 4, 0, 7);
    } else if (section == 1) {
      unsigned off = 5 + step * 32;
      FIELD("音符数量", off + 30, 0, 9);
      for (unsigned i = 0; i < 9; i++) {
        snprintf(label, sizeof(label), "音符 %u (255空)", i + 1);
        FIELD(label, off + i, 0, 255);
        snprintf(label, sizeof(label), "力度 %u", i + 1);
        FIELD(label, off + 20 + i, 0, 127);
      }
      FIELD("时值 (255默认)", off + 10, 0, 255);
      BIT("重音", off + 29, 1, 0, 0, 1);
      f[n - 1].effect = 2;
      f[n - 1].base = off;
      BIT("连击 · 0–3 = 1–4次", off + 29, 2, 2, 0, 3);
      f[n - 1].effect = 2;
      f[n - 1].base = off;
      bool ext = (data[off + 29] & 0xe0) == 0x60;
      FIELD("Gate % · 0默认", off + 11, 0, 100);
      f[n - 1].effect = 5;
      f[n - 1].base = off;
      FIELD("Chance %", off + 12, 5, 100);
      f[n - 1].effect = 5;
      f[n - 1].base = off;
      FIELD("Transpose", off + 13, -24, 24);
      f[n - 1].effect = 5;
      f[n - 1].base = off;
      FIELD("音符释放数量", off + 31, 0, 9);
      for (unsigned i = 0; i < (ext ? 6u : 9u); i++) {
        snprintf(label, sizeof(label), "释放音符 %u (255空)", i + 1);
        FIELD(label, off + (ext ? 14 : 11) + i, 0, 255);
      }
    } else if (section == 3) {
      unsigned off = 2053 + step * 8;
      for (unsigned i = 0; i < 4; i++) {
        snprintf(label, sizeof(label), "参数锁 %u · 目标 (255空)", i + 1);
        FIELD(label, off + i * 2, 0, 255);
        snprintf(label, sizeof(label), "参数锁 %u · 值", i + 1);
        FIELD(label, off + i * 2 + 1, 0, 127);
      }
    }
  }
  if (open)
    for (unsigned i = 0; i < n; i++) {
      if (size == 223 ||
          (size == 4064 && f[i].offset >= 2048 && f[i].offset < 3832)) {
        f[i].effect = 1;
        f[i].base = size == 223 ? 0 : 2048 + ((f[i].offset - 2048) / 223) * 223;
      }
    }
#undef FIELD
#undef BIT
  return n;
}
