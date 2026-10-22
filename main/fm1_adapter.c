#include "fm1_adapter.h"
#include "bsp/esp-bsp.h"
#include "esp_heap_caps.h"
#include "esp_timer.h"
#include "fm1_exchange_file.h"
#include "fm1_host.h"
#include "fm1_object.h"
#include "fm1_operation.h"
#include "fm1_sample_format.h"
#include "fm1_storage.h"
#include "freertos/FreeRTOS.h"
#include "freertos/queue.h"
#include "freertos/semphr.h"
#include "freertos/task.h"
#include <dirent.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <unistd.h>

enum {
  JOB_VIEW = 1,
  JOB_SET,
  JOB_MIDI,
  JOB_CHORD,
  JOB_FL,
  JOB_OPEN,
  JOB_FILES,
  JOB_EXPORT,
  JOB_PREPARE,
  JOB_CONFIRM,
  JOB_CANCEL,
  JOB_OBJECT_READ,
  JOB_OBJECT_STAGE
};
typedef struct {
  unsigned kind, a, b, c, size;
  uint32_t page, connection;
  char identity[32];
  uint8_t data[768];
  fm1_object_state_t *object_payload;
} job_t;
static SemaphoreHandle_t guard;
static QueueHandle_t jobs;
static fm1_adapter_state_t state;
static unsigned token;
static int64_t next_poll;
static fm1_object_state_t object_state;
/* Single worker scratch avoids nesting several 6 KiB snapshots on its stack. */
static fm1_adapter_state_t view_state, set_state, poll_state;
static void lock(void) { xSemaphoreTake(guard, portMAX_DELAY); }
static void unlock(void) { xSemaphoreGive(guard); }
static void report(const char *message) {
  lock();
  snprintf(state.message, sizeof(state.message), "%s", message);
  state.revision++;
  unlock();
}
void fm1_adapter_snapshot(fm1_adapter_state_t *out) {
  if (!guard) {
    memset(out, 0, sizeof(*out));
    return;
  }
  lock();
  *out = state;
  unlock();
  fm1_state_t host;
  fm1_host_snapshot(&host);
  out->connected = host.connected && !host.updating &&
                   host.connection_id == out->connection_id &&
                   !strcmp(host.identity, out->identity);
  if (!out->connected) {
    out->ready = false;
    for (unsigned i = 0; i < ADAPTER_ROWS; i++)
      out->rows[i].known = false;
  }
}
void fm1_adapter_visible(bool visible) {
  if (!guard)
    return;
  lock();
  state.visible = visible;
  unlock();
}
static bool enqueue(job_t *job) {
  if (!jobs)
    return false;
  fm1_state_t host;
  fm1_host_snapshot(&host);
  if (!host.connected || host.updating ||
      fm1_adapter_profile(host.identity)->id == FW_UNKNOWN ||
      (job->kind == JOB_CHORD &&
       fm1_adapter_profile(host.identity)->id != FW_FOMNI))
    return false;
  /* Own the operation before publishing busy or queueing. An updater must not
   * preempt a file transaction between its backup, write, abort and readback. */
  if (!fm1_operation_acquire(FM1_OPERATION_ADAPTER))
    return false;
  lock();
  bool busy = state.busy || (state.file_prepared && job->kind != JOB_CONFIRM &&
                             job->kind != JOB_CANCEL);
  if (!busy)
    state.busy = true;
  unlock();
  if (busy) {
    fm1_operation_release(FM1_OPERATION_ADAPTER);
    return false;
  }
  job->connection = host.connection_id;
  snprintf(job->identity, sizeof(job->identity), "%s", host.identity);
  if (xQueueSend(jobs, job, 0) == pdTRUE)
    return true;
  lock();
  state.busy = false;
  unlock();
  fm1_operation_release(FM1_OPERATION_ADAPTER);
  return false;
}
bool fm1_adapter_view(unsigned scope, unsigned first, unsigned track) {
  if (scope > 2 || first > 127 || (track > 7 && track != 255))
    return false;
  job_t j = {.kind = JOB_VIEW, .a = scope, .b = first, .c = track};
  return enqueue(&j);
}
bool fm1_adapter_native_view(const fm1_parameter_ref_t *refs,unsigned count,unsigned track) {
  if(!refs||!count||count>ADAPTER_ROWS||(track>7&&track!=255))return false;
  fm1_state_t host;fm1_host_snapshot(&host);
  const fm1_adapter_profile_t *profile=fm1_adapter_profile(host.identity);
  if(!profile->fl||!profile->parameters)return false;
  for(unsigned i=0;i<count;i++)
    if(refs[i].scope>2||refs[i].id>127||(refs[i].scope==2&&profile->id!=FW_FELUCCA))return false;
  job_t j={.kind=JOB_VIEW,.a=3,.c=track,.size=count};
  memcpy(j.data,refs,count*sizeof(*refs));return enqueue(&j);
}
bool fm1_adapter_set(unsigned row, int value, uint32_t revision) {
  if (row >= ADAPTER_ROWS || value < -8192 || value > 8191)
    return false;
  job_t j = {.kind = JOB_SET,
             .a = row,
             .b = (unsigned)(value + 8192),
             .page = revision};
  return enqueue(&j);
}
bool fm1_adapter_midi(const uint8_t *data, unsigned n, unsigned gate) {
  uint8_t p[4];
  if (!fm1_midi_packet(data, n, p) || gate > 2000)
    return false;
  job_t j = {.kind = JOB_MIDI, .size = n, .a = gate};
  memcpy(j.data, data, n);
  return enqueue(&j);
}
bool fm1_adapter_chord(const uint8_t *notes, unsigned count, unsigned gate_ms) {
  if (!notes || count < 2 || count > 8 || !gate_ms || gate_ms > 2000)
    return false;
  unsigned pitch_classes = 0;
  for (unsigned i = 0; i < count; i++) {
    if (notes[i] > 127)
      return false;
    for (unsigned k = 0; k < i; k++)
      if (notes[k] == notes[i])
        return false;
    pitch_classes |= 1u << (notes[i] % 12);
  }
  if (!(pitch_classes & (pitch_classes - 1)))
    return false;
  job_t j = {.kind = JOB_CHORD, .size = count, .a = gate_ms};
  memcpy(j.data, notes, count);
  return enqueue(&j);
}
bool fm1_adapter_mmc(unsigned command) {
  if (command != 1 && command != 2 && command != 3 && command != 6 &&
      command != 7 && command != 9)
    return false;
  job_t j = {.kind = JOB_MIDI, .size = 6, .b = command};
  uint8_t m[] = {0xf0, 0x7f, 0x7f, 6, command, 0xf7};
  memcpy(j.data, m, 6);
  return enqueue(&j);
}
bool fm1_adapter_command(unsigned command, const uint8_t *data, unsigned n) {
  if (n > 768 || (!data && n) || command > 127)
    return false;
  job_t j = {.kind = JOB_FL, .a = command, .size = n};
  if (n)
    memcpy(j.data, data, n);
  return enqueue(&j);
}
bool fm1_adapter_open_command(unsigned command, const uint8_t *data,
                              unsigned n) {
  if (n > 128 || (!data && n) || command > 35)
    return false;
  job_t j = {.kind = JOB_OPEN, .a = command, .size = n};
  if (n)
    memcpy(j.data, data, n);
  return enqueue(&j);
}
static int fl(const job_t *job, unsigned command, const uint8_t *data,
              unsigned n, uint8_t *reply) {
  uint8_t frame[1024];
  unsigned size = fm1_fl_frame(command, data, n, frame, sizeof(frame));
  if (!size)
    return -1;
  int got = fm1_host_exchange(job->identity, job->connection, frame, size,
                              frame, 5, reply, 1024, 5000);
  if (got < 6 || reply[got - 1] != 0xf7)
    return -1;
  for (int i = 5; i < got - 1; i++)
    if (reply[i] > 127)
      return -1;
  memmove(reply, reply + 5, got - 6);
  return got - 6;
}
static int open_rpc(const job_t *j, unsigned command, const uint8_t *data,
                    unsigned n, uint8_t *reply) {
  uint8_t frame[192], wire[192], decoded[65], prefix[8];
  token = (token + 1) & 16383;
  unsigned size = fm1_sample_encode(frame, command, token, data, n);
  if (!size)
    return -1;
  memcpy(prefix, frame, 8);
  prefix[5] |= 64;
  int got = fm1_host_exchange(j->identity, j->connection, frame, size, prefix,
                              8, wire, sizeof(wire), 5000);
  int count =
      got > 0 ? fm1_sample_decode(wire, got, command, token, decoded) : -1;
  if (count < 1 || decoded[0])
    return -1;
  memcpy(reply, decoded + 1, count - 1);
  return count - 1;
}
#include "fm1_adapter_files.c"
void fm1_adapter_object_snapshot(fm1_object_state_t *out) {
  if (!guard) {
    memset(out, 0, sizeof(*out));
    return;
  }
  lock();
  *out = object_state;
  unlock();
  fm1_state_t host;
  fm1_host_snapshot(&host);
  if (!host.connected || host.updating || out->connection != host.connection_id)
    out->loaded = false;
}
bool fm1_adapter_object_read(unsigned object) {
  if (object > 255)
    return false;
  job_t j = {.kind = JOB_OBJECT_READ, .a = object};
  return enqueue(&j);
}
bool fm1_adapter_object_stage(const fm1_object_state_t *edited) {
  if (!edited || !edited->loaded || edited->size > FM1_OBJECT_CAPACITY)
    return false;
  fm1_object_state_t *copy =
      heap_caps_malloc(sizeof(*copy), MALLOC_CAP_SPIRAM | MALLOC_CAP_8BIT);
  if (!copy)
    return false;
  *copy = *edited;
  job_t j = {.kind = JOB_OBJECT_STAGE, .object_payload = copy};
  if (enqueue(&j))
    return true;
  free(copy);
  return false;
}
static bool object_job(const job_t *j) {
  bool ok = false;
  if (!fm1_storage_external_begin())
    return false;
  if (j->kind == JOB_OBJECT_READ) {
    lock();
    object_state.loaded = false;
    object_state.revision++;
    unlock();
    uint8_t *data = NULL;
    unsigned size = 0;
    if (read_object(j, j->a, &data, &size) && size <= FM1_OBJECT_CAPACITY) {
      if (fm1_adapter_profile(j->identity)->id == FW_SLOOP && j->a == 11 &&
          size == 0) {
        free(data);
        data = calloc(40, 1);
        if (!data) {
          fm1_storage_external_end();
          return false;
        }
        size = 40;
        memcpy(data, "THM2M1N1LE55", 12);
        for (unsigned i = 0; i < 9; i++) {
          data[12 + i * 2] = 0xff;
          data[13 + i * 2] = 0xff;
        }
        data[32] = data[33] = 255;
        fm1_sample_put32(data + 36, fm1_adapter_crc(data, 36));
      }
      lock();
      object_state.loaded = true;
      object_state.revision++;
      object_state.connection = j->connection;
      object_state.object = j->a;
      object_state.size = size;
      memcpy(object_state.data, data, size);
      unlock();
      ok = true;
    }
    free(data);
    report(ok ? "已读取编辑副本；修改不会立即写入设备。"
              : "读取失败；未提供新的编辑副本。");
  } else if (j->object_payload) {
    const fm1_object_state_t *o = j->object_payload;
    lock();
    bool valid = object_state.loaded && o->revision == object_state.revision &&
                 o->connection == j->connection &&
                 o->object == object_state.object &&
                 o->size == object_state.size;
    unlock();
    if (valid) {
      char name[64];
      ok = save_file(j, o->object, o->data, o->size, name);
      if (ok) {
        files_clear();
        prepared_size = 64 + o->size;
        prepared_file = heap_caps_malloc(prepared_size,
                                         MALLOC_CAP_SPIRAM | MALLOC_CAP_8BIT);
        if (prepared_file) {
          fm1_exchange_header(prepared_file, j->identity, o->object, o->data,
                              o->size);
          memcpy(prepared_file + 64, o->data, o->size);
          lock();
          state.file_prepared = true;
          state.file_connection = j->connection;
          state.file_object = o->object;
          state.file_size = o->size;
          state.file_token = ++file_generation;
          snprintf(state.file_name, 64, "%s", name);
          unlock();
        } else
          ok = false;
      }
    }
    report(ok ? "编辑副本已保存；在工程文件页确认写入并校验。"
              : "编辑副本未就绪或连接已变化。");
  }
  fm1_storage_external_end();
  return ok;
}
static bool dump(const job_t *job, fm1_adapter_state_t *s) {
  uint8_t data[1024];
  int n = fl(job, 27, NULL, 0, data);
  if (n < 2 || data[0] != s->selected)
    return false;
  n = fl(job, 4, NULL, 0, data);
  if (n < 2 + 2 * (s->parameters + s->globals))
    return false;
  s->engine = data[0];
  s->preset = data[1];
  for (unsigned i = 0; i < s->count; i++)
    if (s->rows[i].described&&s->rows[i].scope<2) {
      unsigned offset =
          2 + 2 * (s->rows[i].id + (s->rows[i].scope ? s->parameters : 0));
      s->rows[i].value = fm1_v14(data + offset);
      s->rows[i].known = true;
    }
  return true;
}
static bool view(const job_t *job) {
#define s view_state
  fm1_adapter_snapshot(&s);
  const fm1_adapter_profile_t *profile = fm1_adapter_profile(job->identity);
  if (!profile->fl)
    return false;
  uint8_t data[1024];
  int n = fl(job, 1, NULL, 0, data);
  if (n < 0 || !fm1_adapter_parse_info(&s, data, n))
    return false;
  s.scope = job->a;
  s.native_view = job->size>0;
  s.first = job->b;
  s.count = 0;
  memset(s.rows, 0, sizeof(s.rows));
  if (profile->parameters) {
    uint8_t tr = job->c;
    n = fl(job, 27, &tr, tr == 255 ? 0 : 1, data);
    if (n < 2 || data[0] >= s.tracks)
      return false;
    s.selected = data[0];
    unsigned total = s.native_view ? job->size : s.scope == 2 ? s.menu_count
                     : s.scope    ? s.globals
                                  : s.parameters;
    if (!s.native_view&&s.first >= total)
      s.first = 0;
    bool needs_dump=false;
    for (unsigned i = 0; i < ADAPTER_ROWS && (s.native_view?i:s.first+i) < total; i++) {
      const fm1_parameter_ref_t *refs=(const fm1_parameter_ref_t *)job->data;
      unsigned scope=s.native_view?refs[i].scope:s.scope,id=s.native_view?refs[i].id:s.first+i;
      if(scope>2||id>=(scope==2?s.menu_count:scope?s.globals:s.parameters)||
         (scope==2&&profile->id!=FW_FELUCCA))return false;
      uint8_t args[] = {scope, id};
      n = fl(job, scope == 2 ? 72 : 5, scope == 2 ? args + 1 : args,
             scope == 2 ? 1 : 2, data);
      if (n < 2 || data[0] != (scope == 2 ? id : scope) ||
          (scope != 2 && data[1] != id) ||
          !fm1_adapter_parse_desc(&s.rows[i], data, n, scope == 2))
        return false;
      needs_dump|=scope<2;
      s.count++;
    }
    if (needs_dump && !dump(job, &s))
      return false;
  }
  lock();
  bool visible = state.visible, busy = state.busy;
  uint32_t revision = state.revision + 1, page = state.page_revision + 1;
  state = s;
  state.visible = visible;
  state.busy = busy;
  state.revision = revision;
  state.page_revision = page;
  unlock();
  report(profile->parameters ? "已读取设备参数；写入后显示设备回复。"
                             : "此固件提供采样管理；参数编辑协议未开放。");
  return true;
#undef s
}
static bool set(const job_t *job) {
#define s set_state
  fm1_adapter_snapshot(&s);
  if (job->page != s.page_revision || !s.ready || job->a >= s.count)
    return false;
  fm1_remote_parameter_t *p = s.rows + job->a;
  int value = (int)job->b - 8192;
  if (!p->described || !p->known || value < p->minimum || value > p->maximum)
    return false;
  uint8_t args[4], reply[1024];
  int n;
  if (p->scope == 2) {
    args[0] = p->id;
    fm1_put_v14(args + 1, value);
    n = fl(job, 73, args, 3, reply);
    if (n != 4 || reply[0] || reply[1] != p->id)
      return false;
    value = fm1_v14(reply + 2);
  } else {
    args[0] = p->scope ? p->scope : s.selected;
    args[1] = p->id;
    fm1_put_v14(args + 2, value);
    n = fl(job, p->scope ? 3 : 31, args, 4, reply);
    if (n != 4 || reply[0] != args[0] || reply[1] != p->id)
      return false;
    value = fm1_v14(reply + 2);
  }
  lock();
  state.rows[job->a].value = value;
  state.rows[job->a].known = true;
  state.revision++;
  unlock();
  /* Changing engine invalidates its descriptors. Explicit refresh rereads them.
   */
  if (p->scope == 1) {
    job_t refresh = *job;
    refresh.a = s.scope;
    refresh.b = s.first;
    refresh.c = 255;
    if(s.native_view){refresh.size=s.count;fm1_parameter_ref_t *refs=(fm1_parameter_ref_t *)refresh.data;
      for(unsigned i=0;i<s.count;i++)refs[i]=(fm1_parameter_ref_t){s.rows[i].scope,s.rows[i].index};}
    return view(&refresh);
  }
  report("设备已回复参数值。");
  return true;
#undef s
}
static bool chord_session(const job_t *j) {
  fm1_state_t host;
  fm1_host_snapshot(&host);
  return host.connected && !host.updating &&
         host.connection_id == j->connection &&
         !strcmp(host.identity, j->identity);
}
static bool chord(const job_t *j) {
  if (fm1_adapter_profile(j->identity)->id != FW_FOMNI) {
    report("和弦试听仅支持 FoMni。");
    return false;
  }
  unsigned attempted = 0;
  bool sent = true, released = true, changed = false;
  for (unsigned i = 0; i < j->size; i++) {
    if (!chord_session(j)) {
      sent = false;
      changed = true;
      break;
    }
    /* A failed transfer may already have reached the device. Include that
     * attempted note in cleanup, but never send the rest of the chord. */
    uint8_t note[] = {0x91, j->data[i], 96};
    attempted = i + 1;
    if (fm1_host_exchange(j->identity, j->connection, note, sizeof(note),
                          NULL, 0, NULL, 0, 1500) < 0) {
      sent = false;
      break;
    }
  }
  if (sent) {
    unsigned gate_ticks = pdMS_TO_TICKS(j->a);
    vTaskDelay(gate_ticks ? gate_ticks : 1);
  }
  for (unsigned i = 0; i < attempted; i++) {
    if (!chord_session(j)) {
      released = false;
      changed = true;
      break;
    }
    uint8_t note[] = {0x81, j->data[i], 0};
    if (fm1_host_exchange(j->identity, j->connection, note, sizeof(note),
                          NULL, 0, NULL, 0, 1500) < 0)
      released = false; /* Continue releasing other attempted notes. */
  }
  if (changed)
    report(attempted ? "USB 连接已变化，停止发送；原设备可能仍在发声，请在设备上停止。"
                     : "连接已变化，和弦未发送；请重新连接后再试。");
  else if (!sent || !released)
    report("和弦发送未完整完成；已尝试释放所有可能发出的音符，请核对设备。");
  else
    report("和弦与释放音符已通过 USB 发送；实际发声需听音确认。");
  return sent && released;
}
static bool execute(const job_t *j) {
  if (j->kind == JOB_CHORD)
    return chord(j);
  fm1_state_t h;
  fm1_host_snapshot(&h);
  if (h.updating || !h.connected || h.connection_id != j->connection ||
      strcmp(h.identity, j->identity))
    return false;
  /* Several SLOOP versions share FM-1_900. Static v2.7 commands and object
   * layouts require the advertised v12 protocol, not just the USB name. */
  if (fm1_adapter_profile(j->identity)->id == FW_SLOOP &&
      ((j->kind == JOB_FL && j->a >= 33) || j->kind == JOB_CONFIRM ||
       j->kind == JOB_EXPORT || j->kind >= JOB_OBJECT_READ)) {
    uint8_t info[1024];
    int n = fl(j, 1, NULL, 0, info);
    memset(&view_state, 0, sizeof(view_state));
    view_state.profile = FW_SLOOP;
    if (n < 0 || !fm1_adapter_parse_info(&view_state, info, n) ||
        view_state.protocol < 12) {
      report("此功能需要目录中的 SLOOP 2.7 / 协议 12；设备能力未确认。");
      return false;
    }
  }
  if (j->kind >= JOB_OBJECT_READ)
    return object_job(j);
  if (j->kind >= JOB_FILES)
    return file_job(j);
  if (j->kind == JOB_VIEW)
    return view(j);
  if (j->kind == JOB_SET)
    return set(j);
  if (j->kind == JOB_MIDI) {
    int n = fm1_host_exchange(j->identity, j->connection, j->data, j->size,
                              NULL, 0, NULL, 0, 1500);
    if (n < 0)
      return false;
    if (j->a) {
      vTaskDelay(pdMS_TO_TICKS(j->a));
      uint8_t release[3];
      memcpy(release, j->data, j->size);
      if ((release[0] & 0xf0) == 0x90)
        release[0] = (release[0] & 15) | 0x80;
      if (j->size == 3)
        release[2] = 0;
      if (fm1_host_exchange(j->identity, j->connection, release, j->size, NULL,
                            0, NULL, 0, 1500) < 0)
        return false;
    }
    report("已通过 USB 发送；此操作没有状态读回。");
    if (j->b) {
      lock();
      state.result_size = 0;
      state.result_command = j->b;
      state.result_revision++;
      unlock();
    }
    return true;
  }
  uint8_t reply[1024];
  int n = j->kind == JOB_OPEN ? open_rpc(j, j->a, j->data, j->size, reply)
                              : fl(j, j->a, j->data, j->size, reply);
  if (n < 0)
    return false;
  lock();
  memcpy(state.result, reply, n);
  state.result_size = n;
  state.result_command = j->a;
  state.result_revision++;
  state.revision++;
  unlock();
  report("设备已回复；请核对操作结果。");
  return true;
}
static void worker(void *arg) {
  (void)arg;
  for (;;) {
    fm1_state_t host;
    fm1_host_snapshot(&host);
    lock();
    bool changed = host.connection_id != state.connection_id ||
                   strcmp(host.identity, state.identity);
    if (changed) {
      bool visible = state.visible;
      uint32_t rev = state.revision + 1;
      memset(&state, 0, sizeof(state));
      state.visible = visible;
      state.revision = rev;
      state.connection_id = host.connection_id;
      snprintf(state.identity, sizeof(state.identity), "%s", host.identity);
      state.profile = fm1_adapter_profile(host.identity)->id;
      snprintf(state.message, sizeof(state.message), "%s",
               fm1_adapter_at(state.profile)->gap);
    }
    state.connected = host.connected && !host.updating;
    unlock();
    if (changed) {
      files_clear();
      lock();
      object_state.loaded = false;
      object_state.revision++;
      unlock();
    }
    job_t job;
    if (xQueueReceive(jobs, &job, pdMS_TO_TICKS(100)) == pdTRUE) {
      if (!execute(&job) && job.kind != JOB_CONFIRM && job.kind != JOB_CHORD)
        report("操作未确认：连接、协议回复或参数已变化。未自动重发。");
      free(job.object_payload);
      lock();
      state.busy = false;
      unlock();
      fm1_operation_release(FM1_OPERATION_ADAPTER);
      next_poll = esp_timer_get_time() + 1000000;
    } else if (esp_timer_get_time() >= next_poll) {
#define s poll_state
      fm1_adapter_snapshot(&s);
      bool poll_values=false;
      for(unsigned i=0;i<s.count;i++)poll_values|=s.rows[i].scope<2;
      if (s.visible && s.connected && s.ready && poll_values &&
          fm1_adapter_at(s.profile)->parameters &&
          fm1_operation_acquire(FM1_OPERATION_ADAPTER)) {
        job_t poll = {.connection = s.connection_id};
        snprintf(poll.identity, sizeof(poll.identity), "%s", s.identity);
        if (dump(&poll, &s)) {
          lock();
          if (state.page_revision == s.page_revision && !state.busy) {
            if (state.engine != s.engine) {
              for (unsigned i = 0; i < state.count; i++)
                state.rows[i].known = false;
              state.ready = false;
            } else {
              memcpy(state.rows, s.rows, sizeof(s.rows));
              state.revision++;
            }
          }
          unlock();
        } else {
          lock();
          for (unsigned i = 0; i < state.count; i++)
            state.rows[i].known = false;
          unlock();
        }
        fm1_operation_release(FM1_OPERATION_ADAPTER);
      }
      next_poll = esp_timer_get_time() + 1000000;
#undef s
    }
  }
}
void fm1_adapter_start(void) {
  guard = xSemaphoreCreateMutex();
  jobs = xQueueCreate(2, sizeof(job_t));
  if (guard && jobs)
    xTaskCreate(worker, "fm1_adapter", 32768, NULL, 4, NULL);
}
