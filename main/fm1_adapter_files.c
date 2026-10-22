/* Included by the single adapter worker. File I/O and transactions stay off
 * LVGL. */
#define EXCHANGE_DIR BSP_SD_MOUNT_POINT "/fm1/exchange"
static uint8_t *prepared_file;
static size_t prepared_size;
static unsigned file_sequence;
static uint32_t file_generation;
static unsigned file_page;
bool fm1_adapter_files_page(unsigned page) {
  if (page > 65535)
    return false;
  job_t j = {.kind = JOB_FILES, .a = page};
  return enqueue(&j);
}
bool fm1_adapter_files_scan(void) {
  job_t j = {.kind = JOB_FILES};
  return enqueue(&j);
}
bool fm1_adapter_file_read(unsigned object) {
  if (object > 255)
    return false;
  job_t j = {.kind = JOB_EXPORT, .a = object};
  return enqueue(&j);
}
bool fm1_adapter_file_prepare(const char *name) {
  if (!fm1_exchange_name(name))
    return false;
  job_t j = {.kind = JOB_PREPARE};
  snprintf((char *)j.data, 64, "%s", name);
  return enqueue(&j);
}
bool fm1_adapter_file_confirm(uint32_t token) {
  job_t j = {.kind = JOB_CONFIRM, .page = token};
  return enqueue(&j);
}
bool fm1_adapter_file_cancel(uint32_t token) {
  job_t j = {.kind = JOB_CANCEL, .page = token};
  return enqueue(&j);
}
static void file_progress(unsigned done, unsigned total) {
  lock();
  state.file_progress = done;
  state.file_total = total;
  unlock();
}
static void files_clear(void) {
  free(prepared_file);
  prepared_file = NULL;
  prepared_size = 0;
  lock();
  state.file_prepared = false;
  state.file_token = ++file_generation;
  unlock();
}
static bool files_scan(void) {
  mkdir(EXCHANGE_DIR, 0775);
  DIR *d = opendir(EXCHANGE_DIR);
  if (!d)
    return false;
  char names[16][64] = {{0}};
  unsigned count = 0, total = 0;
  struct dirent *e;
  while ((e = readdir(d)))
    if (fm1_exchange_name(e->d_name)) {
      if (total / 16 == file_page)
        snprintf(names[count++], 64, "%.63s", e->d_name);
      total++;
    }
  closedir(d);
  lock();
  memcpy(state.files, names, sizeof(names));
  state.file_count = count;
  state.file_page = file_page;
  state.file_pages = (total + 15) / 16;
  state.revision++;
  unlock();
  return true;
}
static bool object_allowed(fm1_firmware_t p, unsigned object) {
  if (p == FW_X0X)
    return object >= 32 && object <= 34;
  if (p == FW_FELUCCA)
    return object <= 9 || object == 90 || (object >= 32 && object <= 34);
  if (p == FW_SLOOP)
    return object <= 11 || (object >= 32 && object <= 35);
  if (p == FW_JANGADA)
    return object <= 11 || (object >= 32 && object <= 34);
  if (p == FW_VA)
    return object < 144;
  if (fm1_adapter_at(p)->open)
    return object == 0 || object == 32 || (object >= 100 && object < 108) ||
           (object >= 200 && object < 208);
  return false;
}
static int va_rpc(const job_t *j, const uint8_t *frame, unsigned size,
                  unsigned kind, uint32_t argument, uint8_t *out,
                  unsigned cap) {
  uint8_t reply[1024], prefix[] = {0xf0, 0x7d, (kind << 1) & 127};
  int n = fm1_host_exchange(j->identity, j->connection, frame, size, prefix, 3,
                            reply, sizeof(reply), 5000);
  return n > 0 ? fm1_va_reply(reply, n, kind, argument, out, cap) : -1;
}
static bool va_mem(const job_t *j, uint32_t addr, unsigned size, uint8_t *out) {
  uint8_t args[7], frame[32];
  fm1_put_u35(args, addr);
  args[5] = size & 127;
  args[6] = size >> 7;
  unsigned n = fm1_va_request(0x11, args, 7, frame, sizeof(frame));
  return va_rpc(j, frame, n, 0x51, addr, out, size) == (int)size;
}
static bool read_object(const job_t *j, unsigned object, uint8_t **data,
                        unsigned *size) {
  fm1_firmware_t p = fm1_adapter_profile(j->identity)->id;
  if (!object_allowed(p, object))
    return false;
  if (p == FW_FELUCCA && object == 90)
    return false; /* No bank-content read command. */
  uint8_t reply[1024], args[16];
  uint8_t *bytes = NULL;
  unsigned length = 0;
  uint32_t crc = 0;
  bool check_crc = false, ok = true;
  if (p == FW_VA) {
    length = object < 128 ? 187 : 2565;
    bytes = malloc(length);
    if (!bytes)
      return false;
    if (object < 128) {
      uint8_t slot = object, frame[16];
      unsigned n = fm1_va_request(0x10, &slot, 1, frame, sizeof(frame));
      ok = va_rpc(j, frame, n, 0x50, object, bytes, length) == 187;
    } else {
      unsigned pat = object - 128;
      uint8_t settings[137], mark[4];
      ok = va_mem(j, 0x01c0fef8, 137, settings) &&
           va_mem(j, 0x01c76ca0, 4, mark);
      const unsigned at[] = {18, 34, 50, 98, 118};
      for (unsigned i = 0; i < 5; i++)
        bytes[i] = settings[at[i] + pat];
      for (unsigned i = 0; ok && i < 8; i++) {
        uint32_t addr = i < 2 ? 0x01c14cd0 + pat * 512 + i * 256
                              : 0x01c79d30 + pat * 1536 + (i - 2) * 256;
        ok = va_mem(j, addr, 256, bytes + 5 + i * 256);
        file_progress(i + 1, 10);
      }
      if (ok && memcmp(mark, "FMLK", 4))
        memset(bytes + 2053, 255, 512);
      else
        for (unsigned i = 0; ok && i < 2; i++)
          ok = va_mem(j, 0x01c76d00 + pat * 512 + i * 256, 256,
                      bytes + 2053 + i * 256);
    }
  } else if (fm1_adapter_at(p)->open) {
    if (object == 32) {
      int n = open_rpc(j, 11, NULL, 0, reply);
      if (n == 9 && reply[0] == 0) {
        /* No valid bank: a restorable empty eight-pad bank records that state.
         */
        bytes = calloc(512, 1);
        if (!bytes)
          return false;
        fm1_sample_put32(bytes, 0x53314d46);
        fm1_sample_put32(bytes + 4, 1);
        fm1_sample_put32(bytes + 8, 512);
        fm1_sample_put32(bytes + 20, 32000);
        for (unsigned i = 0; i < 16; i++)
          fm1_sample_put32(bytes + 32 + i * 28, 512);
        fm1_sample_put32(bytes + 12, fm1_adapter_crc(bytes, 512));
        *data = bytes;
        *size = 512;
        return true;
      }
      if (n != 9 || reply[0] != 2)
        return false;
      length = fm1_sample_u32(reply + 1);
    } else {
      args[0] = object >= 200 ? 1 : object >= 100 ? 2 : 0;
      args[1] = object >= 200 ? object - 200 : object >= 100 ? object - 100 : 0;
      int n = open_rpc(j, 1, args, 2, reply);
      if (n != 2)
        return false;
      length = reply[0] | reply[1] << 8;
    }
    if (!length || length > FM1_EXCHANGE_MAX)
      return false;
    bytes = heap_caps_malloc(length, MALLOC_CAP_SPIRAM | MALLOC_CAP_8BIT);
    if (!bytes)
      return false;
    for (unsigned offset = 0; ok && offset < length;) {
      unsigned n = length - offset;
      if (n > 64)
        n = 64;
      if (object == 32) {
        fm1_sample_put32(args, offset);
        args[4] = n;
        ok = open_rpc(j, 15, args, 5, reply) == (int)n;
      } else {
        args[0] = offset;
        args[1] = offset >> 8;
        args[2] = n;
        ok = open_rpc(j, 2, args, 3, reply) == (int)n;
      }
      if (ok)
        memcpy(bytes + offset, reply, n);
      offset += n;
      file_progress(offset, length);
    }
    if (object != 32)
      open_rpc(j, 6, NULL, 0, reply);
  } else if (p == FW_X0X) {
    args[0] = object - 32;
    memset(args + 1, 0, 3);
    args[4] = 112;
    int n = fl(j, 16, args, 5, reply);
    uint8_t header[224];
    if (n < 4 || reply[0] != args[0] ||
        fm1_group_unpack(reply + 4, n - 4, header, sizeof(header)) != 224)
      return false;
    if (memcmp(header, "FSMP", 4))
      length = 0;
    else
      length = 512 + fm1_sample_u32(header + 16);
    if (length > 81920)
      return false;
    bytes = heap_caps_malloc(length ? length : 1,
                             MALLOC_CAP_SPIRAM | MALLOC_CAP_8BIT);
    if (!bytes)
      return false;
    for (unsigned offset = 0; ok && offset < length;) {
      unsigned n = length - offset;
      if (n > 224)
        n = 224;
      n = (n + 1) & ~1u;
      args[1] = offset & 127;
      args[2] = (offset >> 7) & 127;
      args[3] = (offset >> 14) & 127;
      args[4] = n / 2;
      int got = fl(j, 16, args, 5, reply);
      uint8_t chunk[224];
      ok = got >= 4 && !memcmp(reply, args, 4) &&
           fm1_group_unpack(reply + 4, got - 4, chunk, sizeof(chunk)) == n;
      unsigned take = length - offset;
      if (take > n)
        take = n;
      if (ok)
        memcpy(bytes + offset, chunk, take);
      offset += take;
      file_progress(offset, length);
    }
  } else {
    unsigned list = p == FW_FELUCCA ? 65 : 34, get = list + 1;
    int n = fl(j, list, NULL, 0, reply);
    unsigned head = p == FW_FELUCCA ? 3 : 2;
    if (n < (int)head || reply[head - 2])
      return false;
    unsigned count = reply[head - 1];
    if (n != (int)(head + count * 11))
      return false;
    bool found = false;
    for (unsigned i = 0; i < count; i++) {
      uint8_t *item = reply + head + i * 11;
      if (item[0] == object) {
        length = fm1_u35(item + 1);
        crc = fm1_u35(item + 6);
        found = true;
        break;
      }
    }
    if (!found && p == FW_SLOOP && (object == 10 || object == 11)) {
      /* BK_LIST deliberately omits default MIDI and absent theme. */
      if (object == 10) {
        bytes = calloc(40, 1);
        if (!bytes)
          return false;
        bytes[32] = 1;
        bytes[33] = 2;
        bytes[34] = 3;
        bytes[35] = 10;
        bytes[36] = 2;
        *data = bytes;
        *size = 40;
        return true;
      }
      found = true;
      length = 0;
      crc = 0;
    }
    if (!found || length > FM1_EXCHANGE_MAX)
      return false;
    check_crc = true;
    bytes = heap_caps_malloc(length ? length : 1,
                             MALLOC_CAP_SPIRAM | MALLOC_CAP_8BIT);
    if (!bytes)
      return false;
    args[0] = object;
    for (unsigned offset = 0; ok && offset < length;) {
      unsigned n = length - offset;
      if (n > 256)
        n = 256;
      fm1_put_u35(args + 1, offset);
      args[6] = n & 127;
      args[7] = n >> 7;
      int got = fl(j, get, args, 8, reply);
      ok = got >= 9 && reply[0] == object && !reply[1] &&
           fm1_u35(reply + 2) == offset &&
           (unsigned)(reply[7] | reply[8] << 7) == n &&
           fm1_group_unpack(reply + 9, got - 9, bytes + offset, n) == n;
      offset += n;
      file_progress(offset, length);
    }
  }
  if (ok && check_crc)
    ok = fm1_adapter_crc(bytes, length) == crc;
  if (!ok) {
    free(bytes);
    return false;
  }
  *data = bytes;
  *size = length;
  return true;
}
static bool write_sample(const job_t *j, unsigned slot, const uint8_t *data,
                         unsigned length) {
  uint8_t args[768], reply[1024];
  args[0] = slot;
  if (!length) {
    int n = fl(j, 14, args, 1, reply);
    return n == 2 && reply[0] == slot && !reply[1];
  }
  if (!fm1_fsmp_validate(data, length,
                         fm1_adapter_profile(j->identity)->id == FW_SLOOP ? 2
                                                                          : 1))
    return false;
  int n = fl(j, 11, args, 1, reply);
  if (n != 2 || reply[0] != slot || reply[1])
    return false;
  for (unsigned offset = 512; offset < length; offset += 256) {
    unsigned take = length - offset;
    if (take > 256)
      take = 256;
    args[1] = offset & 127;
    args[2] = (offset >> 7) & 127;
    args[3] = (offset >> 14) & 127;
    unsigned count =
        fm1_group_pack(data + offset, take, args + 4, sizeof(args) - 4);
    n = fl(j, 12, args, count + 4, reply);
    if (n != 5 || memcmp(reply, args, 4) || reply[4])
      return false;
    file_progress(offset + take, length);
  }
  unsigned count = fm1_group_pack(data, 480, args + 1, sizeof(args) - 1);
  n = fl(j, 13, args, count + 1, reply);
  return n == 2 && reply[0] == slot && !reply[1];
}
static bool write_object(const job_t *j, unsigned object, const uint8_t *data,
                         unsigned length) {
  fm1_firmware_t p = fm1_adapter_profile(j->identity)->id;
  if (!object_allowed(p, object))
    return false;
  uint8_t args[768], reply[1024];
  int n;
  if (p == FW_FELUCCA && object == 90) {
    if (!fm1_felucca_bank_validate(data, length))
      return false;
    n = fl(j, 74, NULL, 0, reply);
    if (n != 1 || reply[0])
      return false;
    unsigned voices = data[14];
    for (unsigned i = 0; i < voices; i++) {
      args[0] = i;
      memcpy(args + 1, data + 15 + 128 * i, 128);
      n = fl(j, 75, args, 129, reply);
      if (n != 2 || reply[0] != i || reply[1])
        return false;
      file_progress(i + 1, voices);
    }
    memcpy(args, data + 4, 10);
    fm1_put_u35(args + 10, fm1_adapter_crc(data + 15, voices * 128));
    n = fl(j, 76, args, 15, reply);
    if (n != 2 || reply[0] || reply[1] != voices)
      return false;
    n = fl(j, 77, NULL, 0, reply);
    if (n < 3 || reply[0] != 32 || !reply[1])
      return false;
    unsigned at = 2;
    while (at < (unsigned)n && reply[at])
      at++;
    if (at == (unsigned)n)
      return false;
    at++;
    for (unsigned i = 0; i < 32; i++) {
      if (at >= (unsigned)n || reply[at++] != (i < voices))
        return false;
      while (at < (unsigned)n && reply[at])
        at++;
      if (at++ >= (unsigned)n)
        return false;
    }
    return true;
  }
  if (p == FW_VA) {
    uint8_t frame[256];
    if (object < 128) {
      if (length != 187)
        return false;
      for (unsigned i = 0; i < 128; i++)
        if (data[i] > 127)
          return false;
      memcpy(frame, "\xf0\x43\x00\x7d\x04", 5);
      frame[5] = object;
      fm1_dx7_unpack(data, frame + 6);
      unsigned packed = fm1_group_pack(data + 128, 59, frame + 161, 68);
      if (packed != 68)
        return false;
      unsigned sum = 0;
      for (unsigned i = 6; i < 229; i++)
        sum += frame[i] ^ 255u;
      frame[229] = sum & 127;
      frame[230] = 0xf7;
      if (fm1_host_exchange(j->identity, j->connection, frame, 231, NULL, 0,
                            NULL, 0, 5000) < 0)
        return false;
      vTaskDelay(pdMS_TO_TICKS(3000));
      return true;
    }
    if (length != 2565)
      return false;
    unsigned pat = object - 128;
    for (unsigned part = 0; part < 16; part++) {
      args[0] = pat;
      args[1] = part;
      args[2] = 0;
      uint8_t raw[133];
      memcpy(raw, data, 5);
      memcpy(raw + 5, data + 5 + part * 128, 128);
      unsigned pack = fm1_group_pack(raw, 133, args + 3, sizeof(args) - 3);
      unsigned count =
          fm1_va_request(0x22, args, pack + 3, frame, sizeof(frame));
      if (va_rpc(j, frame, count, 0x52, pat | (part << 8) | 0x4000, reply,
                 sizeof(reply)) < 0)
        return false;
      file_progress(part + 1, 24);
    }
    for (unsigned part = 0; part < 8; part++) {
      args[0] = pat;
      args[1] = part;
      args[2] = part == 7;
      const uint8_t *locks = data + 2053 + part * 64;
      for (unsigned i = 0; i < 64; i += 2) {
        args[3 + i] = locks[i] == 255 ? 127 : locks[i];
        args[4 + i] = locks[i] == 255 ? 127 : locks[i + 1];
      }
      unsigned count = fm1_va_request(0x21, args, 67, frame, sizeof(frame));
      if (va_rpc(j, frame, count, 0x52, pat | (part << 8) | 0x8000, reply,
                 sizeof(reply)) < 0)
        return false;
      file_progress(17 + part, 24);
    }
    return true;
  }
  if (fm1_adapter_at(p)->open) {
    if (object == 32) {
      if (!fm1_sample_validate(data, length))
        return false;
      fm1_sample_put32(args, length);
      if (open_rpc(j, 12, args, 4, reply) < 0)
        return false;
    } else {
      args[0] = object >= 200 ? 1 : object >= 100 ? 2 : 0;
      args[1] = object >= 200 ? object - 200 : object >= 100 ? object - 100 : 0;
      args[2] = length;
      args[3] = length >> 8;
      if (length > 65535 || open_rpc(j, 3, args, 4, reply) < 0)
        return false;
    }
    bool ok = true;
    for (unsigned offset = 0; ok && offset < length;) {
      unsigned take = length - offset;
      if (take > 64)
        take = 64;
      if (object == 32) {
        fm1_sample_put32(args, offset);
        memcpy(args + 4, data + offset, take);
        ok = open_rpc(j, 13, args, take + 4, reply) >= 0;
      } else {
        args[0] = offset;
        args[1] = offset >> 8;
        memcpy(args + 2, data + offset, take);
        ok = open_rpc(j, 4, args, take + 2, reply) >= 0;
      }
      offset += take;
      file_progress(offset, length);
    }
    if (ok) {
      fm1_sample_put32(args, fm1_adapter_crc(data, length));
      ok = open_rpc(j, object == 32 ? 14 : 5, args, object == 32 ? 0 : 4,
                    reply) >= 0;
    }
    if (!ok)
      open_rpc(j, object == 32 ? 16 : 6, NULL, 0, reply);
    return ok;
  }
  if (object >= 32)
    return write_sample(j, object - 32, data, length);
  unsigned put = p == FW_FELUCCA ? 67 : 36;
  args[0] = 0;
  args[1] = object;
  fm1_put_u35(args + 2, length);
  fm1_put_u35(args + 7, fm1_adapter_crc(data, length));
  n = fl(j, put, args, 12, reply);
  if (n != 3 || reply[0] || reply[1] != object || reply[2])
    return false;
  bool ok = true;
  for (unsigned offset = 0; ok && offset < length;) {
    unsigned take = length - offset;
    if (take > 256)
      take = 256;
    args[0] = 1;
    args[1] = object;
    fm1_put_u35(args + 2, offset);
    unsigned count =
        fm1_group_pack(data + offset, take, args + 7, sizeof(args) - 7);
    n = fl(j, put, args, count + 7, reply);
    ok = n == 3 && reply[0] == 1 && reply[1] == object && !reply[2];
    offset += take;
    file_progress(offset, length);
  }
  args[0] = ok ? 2 : 3;
  args[1] = object;
  n = fl(j, put, args, 2, reply);
  return ok && n == 3 && reply[0] == 2 && reply[1] == object && !reply[2];
}
static bool save_file(const job_t *j, unsigned object, const uint8_t *data,
                      unsigned length, char name[64]) {
  uint8_t header[64];
  if (!fm1_exchange_header(header, j->identity, object, data, length))
    return false;
  mkdir(EXCHANGE_DIR, 0775);
  char path[180];
  int fd = -1;
  for (unsigned attempt = 0; attempt < 1000 && fd < 0; attempt++) {
    snprintf(name, 64, "%s-%03u-%06u.m1x", j->identity, object,
             ++file_sequence);
    snprintf(path, sizeof(path), EXCHANGE_DIR "/%s", name);
    fd = open(path, O_WRONLY | O_CREAT | O_EXCL, 0666);
  }
  if (fd < 0)
    return false;
  FILE *f = fdopen(fd, "wb");
  if (!f) {
    close(fd);
    unlink(path);
    return false;
  }
  bool ok =
      fwrite(header, 1, 64, f) == 64 && fwrite(data, 1, length, f) == length;
  if (fclose(f))
    ok = false;
  if (ok) {
    f = fopen(path, "rb");
    uint8_t block[512];
    ok = f && fread(block, 1, 64, f) == 64 && !memcmp(block, header, 64);
    for (unsigned at = 0; ok && at < length;) {
      unsigned take = length - at;
      if (take > sizeof(block))
        take = sizeof(block);
      ok = fread(block, 1, take, f) == take && !memcmp(block, data + at, take);
      at += take;
    }
    if (f) {
      if (ok)
        ok = fgetc(f) == EOF;
      if (fclose(f))
        ok = false;
    }
  }
  if (!ok)
    unlink(path);
  return ok;
}
static bool file_job(const job_t *j) {
  if (j->kind == JOB_CANCEL) {
    bool valid;
    lock();
    valid = state.file_prepared && state.file_token == j->page;
    unlock();
    if (valid)
      files_clear();
    return valid;
  }
  if (!fm1_storage_external_begin())
    return false;
  bool ok = false;
  if (j->kind == JOB_FILES) {
    file_page = j->a;
    ok = files_scan();
  }
  if (j->kind == JOB_EXPORT) {
    uint8_t *data = NULL;
    unsigned length = 0;
    char name[64];
    report("读取设备并保存到 SD；设备可能停止播放。");
    if (read_object(j, j->a, &data, &length))
      ok = save_file(j, j->a, data, length, name);
    free(data);
    if (ok) {
      lock();
      snprintf(state.file_backup, 64, "%s", name);
      unlock();
      files_scan();
      report("设备对象已读取并保存到 SD。");
    }
  }
  if (j->kind == JOB_PREPARE) {
    files_clear();
    char path[180];
    snprintf(path, sizeof(path), EXCHANGE_DIR "/%.63s", j->data);
    FILE *f = fopen(path, "rb");
    if (f) {
      fseek(f, 0, SEEK_END);
      long bytes = ftell(f);
      rewind(f);
      if (bytes >= 64 && (unsigned long)bytes <= 64 + FM1_EXCHANGE_MAX) {
        prepared_file =
            heap_caps_malloc(bytes, MALLOC_CAP_SPIRAM | MALLOC_CAP_8BIT);
        unsigned object = 0;
        ok =
            prepared_file &&
            fread(prepared_file, 1, bytes, f) == (size_t)bytes &&
            fm1_exchange_validate(prepared_file, bytes, j->identity, &object) &&
            object_allowed(fm1_adapter_profile(j->identity)->id, object);
        fm1_firmware_t p = fm1_adapter_profile(j->identity)->id;
        if (ok && p == FW_FELUCCA && object == 90)
          ok = fm1_felucca_bank_validate(prepared_file + 64, bytes - 64);
        if (ok && object >= 32 && object <= 35 && !fm1_adapter_at(p)->open &&
            p != FW_VA && bytes > 64)
          ok = fm1_fsmp_validate(prepared_file + 64, bytes - 64,
                                 p == FW_SLOOP ? 2 : 1);
        if (ok) {
          prepared_size = bytes;
          lock();
          state.file_prepared = true;
          state.file_connection = j->connection;
          state.file_object = object;
          state.file_size = bytes - 64;
          state.file_token = ++file_generation;
          snprintf(state.file_name, 64, "%.63s", j->data);
          unlock();
        }
      }
      fclose(f);
    }
    if (!ok)
      files_clear();
    else
      report("文件身份与校验通过；确认后先备份旧对象，再写入并读回。");
  }
  if (j->kind == JOB_CONFIRM) {
    lock();
    bool valid = state.file_prepared && state.file_token == j->page &&
                 state.file_connection == j->connection;
    unsigned object = state.file_object;
    unlock();
    if (valid && prepared_file && prepared_size >= 64 &&
        fm1_adapter_profile(j->identity)->id == FW_FELUCCA && object == 90) {
      report("写入 FM6 音色库；设备以 CRC 提交，协议没有整库读回。");
      ok = write_object(j, object, prepared_file + 64, prepared_size - 64);
      files_clear();
      report(ok ? "设备已通过 CRC "
                  "提交，目录已核对；协议不支持整库读回，源文件保留。"
                : "音色库未确认完成；请读取目录核对，勿重复盲写。");
    } else if (valid && prepared_file && prepared_size >= 64) {
      uint8_t *old = NULL;
      unsigned old_size = 0;
      char backup[64];
      report("正在备份目标旧对象。");
      if (read_object(j, object, &old, &old_size) &&
          save_file(j, object, old, old_size, backup)) {
        lock();
        snprintf(state.file_backup, 64, "%s", backup);
        unlock();
        free(old);
        old = NULL;
        report("正在写入；保持 USB 连接与供电。");
        if (write_object(j, object, prepared_file + 64, prepared_size - 64)) {
          report("正在完整读回比较。");
          ok = read_object(j, object, &old, &old_size) &&
               old_size == prepared_size - 64 &&
               !memcmp(old, prepared_file + 64, old_size);
        }
      }
      free(old);
      files_clear();
      files_scan();
      report(ok ? "已写入且逐字节读回一致；旧对象备份保留在 SD。"
                : "写入未完成或读回不一致；设备可能已变化，旧备份保留。不得重复"
                  "盲写。");
    }
  }
  fm1_storage_external_end();
  return ok;
}
