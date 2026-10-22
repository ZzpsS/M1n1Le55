#include "fm1_action.h"
#include <stdarg.h>
#include <stdio.h>
#include <string.h>
static void append(char *out, unsigned cap, const char *fmt, ...) {
  unsigned used = strlen(out);
  if (used >= cap)
    return;
  va_list ap;
  va_start(ap, fmt);
  vsnprintf(out + used, cap - used, fmt, ap);
  va_end(ap);
}
static bool string(const uint8_t *d, unsigned n, unsigned *at, char out[80]) {
  unsigned k = 0;
  while (*at < n && d[*at]) {
    unsigned c = d[(*at)++];
    if (k < 79)
      out[k++] = c >= 32 && c <= 126 ? c : '?';
  }
  out[k] = 0;
  if (*at == n)
    return false;
  (*at)++;
  return true;
}
void fm1_action_summary(fm1_firmware_t p, unsigned cmd, const uint8_t *d,
                        unsigned n, char *out, unsigned cap) {
  if (!out || !cap)
    return;
  out[0] = 0;
  unsigned at = 0;
  char name[80];
  if (p == FW_SLOOP && cmd == 73 && n > 3 && !d[1]) {
    at = 2;
    if (!string(d, n, &at, name))
      return;
    uint8_t raw[354];
    if (fm1_group_unpack(d + at, n - at, raw, sizeof(raw)) != sizeof(raw))
      return;
    append(out, cap, "%s · Crush %u · 工厂来源 %u\n", name, raw[0], raw[1]);
    for (unsigned lane = 0; lane < 16; lane++) {
      const uint8_t *v = raw + 2 + lane * 22;
      append(out, cap,
             "Lane %u · Wave %u · Pitch %u · Decay %u · Noise %u · Cutoff %u · "
             "Level %u\n",
             lane + 1, v[0], v[2], v[7], v[12], v[16], v[21]);
    }
    append(out, cap, "完整 22 参数可在工程文件页选择鼓合成套件后编辑。");
    return;
  }
  if (fm1_adapter_at(p)->open) {
    if (cmd == 0 && n == 14) {
      append(out, cap, "协议 %u · 工程槽位 %u · 当前槽 %u · 时钟模式 %u\n",
             d[0], d[1], d[2], d[5]);
      for (unsigned i = 0; i < 8; i++)
        append(out, cap, "轨道 %u 引擎 %u\n", i + 1, d[6 + i]);
    } else if (cmd == 11 && n == 9)
      append(out, cap, "采样状态 %u · %u bytes · 容量 %u", d[0],
             (unsigned)d[1] | (unsigned)d[2] << 8 | (unsigned)d[3] << 16 |
                 (unsigned)d[4] << 24,
             (unsigned)d[5] | (unsigned)d[6] << 8 | (unsigned)d[7] << 16 |
                 (unsigned)d[8] << 24);
    else if (cmd >= 17 && cmd <= 20) {
      const char *labels[4][8] = {{"提交", "完成", "欠载", "丢弃 MIDI"},
                                  {"渲染 ticks", "峰值 ticks", "MIDI ticks",
                                   "降载次数", "超载次数", "声部容量", "总增益",
                                   "期限 ticks"},
                                  {"音频块", "短间隔", "超时", "峰值 ticks"},
                                  {"总线状态"}};
      for (unsigned i = 0; i < n / 4 && i < 8; i++)
        append(out, cap, "%s  %u\n",
               labels[cmd - 17][i] ? labels[cmd - 17][i] : "值",
               (unsigned)d[4 * i] | (unsigned)d[4 * i + 1] << 8 |
                   (unsigned)d[4 * i + 2] << 16 | (unsigned)d[4 * i + 3] << 24);
      if (cmd == 17 && n == 18)
        append(out, cap, "采样状态 %u · 录音 %u\n", d[16], d[17]);
    } else if ((cmd == 32 || cmd == 35) && n == 17 && !memcmp(d, "BLP1", 4)) {
      append(out, cap, "传输校验状态 %u · 0空闲 1接收 2通过 3失败\n", d[4]);
      const char *label[] = {"目标字节", "已接收字节", "CRC32"};
      for (unsigned i = 0; i < 3; i++) {
        unsigned k = 5 + i * 4;
        append(out, cap, "%s  %u\n", label[i],
               (unsigned)d[k] | (unsigned)d[k + 1] << 8 |
                   (unsigned)d[k + 2] << 16 | (unsigned)d[k + 3] << 24);
      }
    } else if (cmd == 32) {
      append(out, cap, "诊断快照 · %u bytes\n", n);
      for (unsigned i = 0; i < n; i++)
        append(out, cap, "%02X%s", d[i], (i + 1) % 16 ? " " : "\n");
    } else
      append(out, cap, "设备已确认 · %u bytes", n);
    return;
  }
  if (p == FW_FELUCCA && cmd == 77 && n >= 3) {
    at = 2;
    if (!string(d, n, &at, name))
      return;
    append(out, cap, "FM6 音色库 · %s · %s\n", d[1] ? "已存储" : "空", name);
    for (unsigned i = 0; i < d[0] && at < n; i++) {
      unsigned used = d[at++];
      if (!string(d, n, &at, name))
        break;
      append(out, cap, "%02u  %s\n", i + 1, used ? name : "空");
    }
    return;
  }
  if (cmd == 1 && string(d, n, &at, name) && n - at >= 5) {
    append(out, cap, "固件 %s\n引擎 %u · 轨道参数 %u · 全局参数 %u · 步数 %u",
           name, d[at], d[at + 1], d[at + 2], d[at + 3]);
    return;
  }
  if (cmd == 10 && n >= 2) {
    at = 2;
    append(out, cap, "引擎 %u · %u 个工厂预设\n", d[0], d[1]);
    for (unsigned i = 0; i < d[1] && string(d, n, &at, name); i++)
      append(out, cap, "%03u  %s\n", i, name);
    return;
  }
  if (cmd == 16 && p != FW_X0X && n >= 3) {
    at = 3;
    append(out, cap, "用户预设 · 总数 %u\n", d[2]);
    for (unsigned i = 0; i < d[1] && n - at >= 3; i++) {
      unsigned used = d[at++], eng = d[at++];
      if (!string(d, n, &at, name))
        break;
      append(out, cap, "%02u  %s  · 引擎 %u\n", d[0] + i, used ? name : "空",
             eng);
    }
    return;
  }
  if (cmd == 70 && n >= 2) {
    at = 2;
    for (unsigned i = 0; i < (unsigned)d[0] + d[1] && at < n; i++) {
      unsigned used = d[at++];
      if (!string(d, n, &at, name))
        break;
      append(out, cap, "%s %02u  %s\n", i < d[0] ? "工厂" : "库",
             i < d[0] ? i : i - d[0], used ? name : "空");
    }
    return;
  }
  if (cmd == 36 && p == FW_FELUCCA && n) {
    at = 1;
    for (unsigned i = 0; i < d[0] && string(d, n, &at, name); i++)
      append(out, cap, "%02u  %s\n", i, name);
    return;
  }
  if (cmd == 15 && n >= 2) {
    at = 2;
    append(out, cap, "%u 个采样槽 · 每槽 %u KiB\n", d[0], d[1]);
    for (unsigned i = 0; i < d[0] && at < n; i++) {
      unsigned zones = d[at++];
      if (!string(d, n, &at, name) || at >= n)
        break;
      append(out, cap, "%02u  %s · %u 区域 · %u KiB\n", i, zones ? name : "空",
             zones, d[at++]);
    }
    return;
  }
  if (cmd == 64 && p == FW_FELUCCA && n >= 7) {
    unsigned count = d[3] | d[4] << 7, max = d[5] | d[6] << 7;
    at = 7;
    if (count <= 128 && n >= at + count * 5) {
      append(out, cap, "T%u · %s · %u / %u\n", d[0] + 1, d[2] ? "启用" : "关闭",
             count, max);
      for (unsigned i = 0; i < count; i++) {
        const uint8_t *r = d + at + i * 4;
        append(out, cap, "步 %02u · 参数 %03u = %d · %s\n", r[0] + 1, r[1],
               fm1_v14(r + 2), d[at + count * 4 + i] ? "锁" : "自动化");
      }
      return;
    }
  }
  if (cmd == 78 && p == FW_FELUCCA && n == 65) {
    for (unsigned i = 0; i < 16; i++) {
      const uint8_t *r = d + 1 + i * 4;
      append(out, cap, "%02u  ", i + 1);
      if (r[0])
        append(out, cap, "CC%u → T%u 参数 %u\n", r[1], r[2] + 1, r[3]);
      else
        append(out, cap, "空\n");
    }
    return;
  }
  if (cmd == 37 && p == FW_SLOOP && n >= 2 && n == 2u + 4u * d[1]) {
    append(out, cap, "T%u · %u 个参数锁\n", d[0] + 1, d[1]);
    for (unsigned i = 0; i < d[1]; i++) {
      const uint8_t *r = d + 2 + i * 4;
      append(out, cap, "步 %02u · 参数 %03u = %d\n", r[0] + 1, r[1],
             fm1_v14(r + 2));
    }
    return;
  }
  if (cmd == 39 && p == FW_SLOOP && n == 65) {
    for (unsigned i = 0; i < 64; i++)
      append(out, cap, "步 %02u  %+d / 64\n", i + 1, (int)d[i + 1] - 64);
    return;
  }
  if (cmd == 41 && p == FW_SLOOP && n > 1) {
    uint8_t values[16];
    if (fm1_group_unpack(d + 1, n - 1, values, 16) == 16) {
      const char *condition[] = {"正常", "仅填充", "非填充", "正常"};
      for (unsigned i = 0; i < 64; i++)
        append(out, cap, "步 %02u  %s\n", i + 1,
               condition[values[i / 4] >> (2 * (i % 4)) & 3]);
      return;
    }
  }
  if (cmd == 77 && p == FW_SLOOP && n >= 33) {
    append(out, cap,
           "%s · BPM %d\n录音 %s · 歌曲 %s\n当前段落 %u · 下一段落 "
           "%u\n静音位图 %u · 独奏位图 %u\n选中 T%u · 效果 %u · CPU %u%%",
           d[1] & 1 ? "播放中" : "已停止", fm1_v14(d + 3),
           d[2] & 1 ? "已武装" : "关闭", d[1] & 4 ? "开启" : "关闭", d[5], d[6],
           d[9], d[10], d[12] + 1, d[13], d[14]);
    return;
  }
  if (cmd == 34 && p == FW_FELUCCA && n >= 5) {
    append(out, cap, "配色 %u · 收藏筛选 %s", d[1],
           d[4] == 1 ? "开启" : "关闭");
    return;
  }
  if (cmd == 37 && p == FW_FELUCCA && n >= 5 && !d[0]) {
    unsigned start = fm1_v14(d + 2), count = d[4];
    if (n >= 5 + count) {
      for (unsigned i = 0; i < count; i++)
        append(out, cap, "引擎 %u · 预设 %u · %s\n", d[1], start + i,
               d[5 + i] ? "已收藏" : "未收藏");
      return;
    }
  }
  if (cmd == 72 && p == FW_SLOOP && n >= 3) {
    at = 3;
    append(out, cap, "套件 %s\n", d[2] ? "已保存" : "有未保存修改");
    for (unsigned i = 0; i < (unsigned)d[0] + d[1] && string(d, n, &at, name);
         i++) {
      append(out, cap, "%s %02u  %s\n", i < d[0] ? "工厂" : "用户",
             i < d[0] ? i : i - d[0], name);
      if (i >= d[0] && at < n)
        at++;
    }
    return;
  }
  append(out, cap, "设备已回复。\n可编辑数据已放入参数表单；操作状态见页脚。");
}
