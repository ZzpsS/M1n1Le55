#include "fm1_adapter_ui.h"
#include "fm1_action.h"
#include "fm1_adapter.h"
#include "fm1_host.h"
#include "fm1_object.h"
#include "fm1_ui.h"
#include "m1_theme.h"
#include <stdio.h>
#include <string.h>

static lv_obj_t *panel, *content, *message, *heading, *track_label;
static fm1_adapter_state_t current;
static fm1_adapter_state_t next_state;
static uint32_t confirm_connection;
static bool file_mode;
static unsigned file_index, object_index, object_ids[144], object_count;
static uint32_t drawn_file_token;
static unsigned tab, scope, first, channel, cc_page, program, project_slot;
static bool rebuilding, dirty, confirm;
static uint32_t drawn_page;
static fm1_firmware_t drawn_profile;
static uint32_t drawn_connection;
static char drawn_identity[32];
typedef struct {
  const char *label;
  uint8_t cc, max;
  bool momentary;
} cc_t;
static const cc_t va_cc[] = {{"Volume", 7, 127, 0},
                             {"Brightness / Cutoff", 74, 127, 0},
                             {"Feedback / Resonance", 71, 127, 0},
                             {"Attack", 73, 127, 0},
                             {"Decay", 75, 127, 0},
                             {"Sustain", 70, 127, 0},
                             {"Release", 72, 127, 0},
                             {"LFO speed", 76, 127, 0},
                             {"LFO pitch depth", 77, 127, 0},
                             {"LFO delay", 78, 127, 0},
                             {"FM algorithm", 58, 127, 0},
                             {"VA waveform", 24, 127, 0},
                             {"VA super", 25, 127, 0},
                             {"VA detune", 26, 127, 0},
                             {"VA drift", 27, 127, 0},
                             {"VA sub", 28, 127, 0},
                             {"VA noise", 29, 127, 0},
                             {"VA PWM", 30, 127, 0},
                             {"VA filter type", 31, 127, 0},
                             {"Filter envelope", 52, 127, 0},
                             {"Filter decay", 53, 127, 0},
                             {"Filter shape", 54, 127, 0},
                             {"Filter velocity", 55, 127, 0},
                             {"Filter key tracking", 56, 127, 0},
                             {"LFO to cutoff", 57, 127, 0},
                             {"Knob 1", 85, 127, 0},
                             {"Knob 2", 86, 127, 0},
                             {"Knob 3", 87, 127, 0},
                             {"Knob 4", 88, 127, 0},
                             {"Algorithm -", 89, 127, 1},
                             {"Algorithm +", 90, 127, 1},
                             {"Preset -", 116, 127, 1},
                             {"Preset +", 117, 127, 1},
                             {"Select up", 118, 127, 1},
                             {"Select down", 119, 127, 1},
                             {"FX", 102, 127, 1},
                             {"SEL", 103, 127, 1},
                             {"ENV", 104, 127, 1},
                             {"LFO", 105, 127, 1},
                             {"EDIT", 106, 127, 1},
                             {"GLOBAL", 107, 127, 1},
                             {"HOME", 108, 127, 1},
                             {"ARP", 110, 127, 1},
                             {"SEQ", 111, 127, 1},
                             {"PLAY", 112, 127, 1},
                             {"REC", 113, 127, 1},
                             {"OCT -", 114, 127, 1},
                             {"OCT +", 115, 127, 1},
                             {"Mod wheel", 1, 127, 0},
                             {"Breath", 2, 127, 0},
                             {"Foot", 4, 127, 0},
                             {"Sustain pedal", 64, 127, 0}};
static const cc_t common_cc[] = {{"Level", 7, 127, 0},
                                 {"Pan", 10, 127, 0},
                                 {"Expression", 11, 127, 0},
                                 {"Modulation", 1, 127, 0},
                                 {"Cutoff / brightness", 74, 127, 0},
                                 {"Resonance", 71, 127, 0},
                                 {"Attack", 73, 127, 0},
                                 {"Decay", 75, 127, 0},
                                 {"Release", 72, 127, 0},
                                 {"Glide", 5, 127, 0},
                                 {"Sustain", 64, 127, 0},
                                 {"Reverb send", 91, 127, 0},
                                 {"Chorus send", 93, 127, 0},
                                 {"Delay send", 94, 127, 0},
                                 {"Macro 1", 16, 127, 0},
                                 {"Macro 2", 17, 127, 0},
                                 {"Macro 3", 18, 127, 0},
                                 {"Macro 4", 19, 127, 0},
                                 {"Reset controllers", 121, 0, 1},
                                 {"All notes off", 123, 0, 1},
                                 {"All sound off", 120, 0, 1}};
typedef struct {
  lv_obj_t *label, *slider, *minus, *plus, *choice;
  unsigned item;
  uint32_t connection, page;
  fm1_firmware_t profile;
  uint8_t parameter;
  char identity[32];
  int min, max;
  bool known, dragging, remote, action, object, manual;
} row_t;
static row_t rows[4];
static int cc_values[128];
static bool cc_known[128];
static fm1_action_t tool;
static int tool_values[FM1_ACTION_FIELDS];
static unsigned tool_index, tool_page;
static unsigned tool_initial_track = 255;
static fm1_firmware_t tool_profile;
static bool tool_loaded, tool_pending, tool_read, tool_confirm;
static bool native_object_mode;
static bool context_read_pending;
static uint32_t context_read_revision;
static uint32_t tool_revision, tool_connection;
static char tool_result[600];
static char tool_detail[8192];
static bool tool_show_result;
static fm1_object_state_t object_edit, object_next;
static fm1_object_field_t object_fields[192];
static unsigned edit_section, edit_track, edit_step, edit_page, edit_count,
    edit_target;
static bool manual_mode;
static int manual_values[4] = {0, 74, 64, 120};
static unsigned note_base = 36;
static void render(void);
static void event(lv_event_t *e);
enum {
  A_CONSOLE = 90,
  A_TAB = 100,
  A_SCOPE = 110,
  A_TRACK = 120,
  A_PAGE_PREV = 130,
  A_PAGE_NEXT,
  A_REFRESH,
  A_CH_DOWN,
  A_CH_UP,
  A_CC_PREV,
  A_CC_NEXT,
  A_PC_DOWN,
  A_PC_UP,
  A_PROGRAM,
  A_START,
  A_CONTINUE,
  A_STOP,
  A_PANIC,
  A_PROJECT_DOWN,
  A_PROJECT_UP,
  A_PROJECT_QUERY,
  A_PROJECT_LOAD,
  A_PROJECT_SAVE,
  A_CONFIRM,
  A_CANCEL,
  A_ORIGINAL,
  A_FILE_MODE,
  A_FILE_SCAN,
  A_FILES_PREV,
  A_FILES_NEXT,
  A_FILE_READ,
  A_FILE_PREPARE,
  A_FILE_CONFIRM,
  A_FILE_CANCEL,
  A_FILE_LAST,
  A_TOOL_READ,
  A_TOOL_EXECUTE,
  A_TOOL_CONFIRM,
  A_TOOL_CANCEL,
  A_TOOL_PREV,
  A_TOOL_NEXT,
  A_TOOL_RESULT,
  A_OBJ_OPEN,
  A_OBJ_READ,
  A_OBJ_STAGE,
  A_OBJ_PREV,
  A_OBJ_NEXT,
  A_OBJ_TRACK_DOWN,
  A_OBJ_TRACK_UP,
  A_OBJ_STEP_DOWN,
  A_OBJ_STEP_UP,
  A_MANUAL_MODE,
  A_MANUAL_SEND,
  A_CLOCK_ON,
  A_CLOCK_OFF,
  A_OCT_DOWN,
  A_OCT_UP,
  A_NOTE = 200,
  A_CC_PRESS = 300,
  A_FILE_SELECT = 500
};
_Static_assert(A_OCT_UP < A_NOTE, "UI action ranges must not overlap");
static lv_obj_t *button(const char *s, int x, int y, int w, int h,
                        unsigned action, bool primary) {
  return m1_button(panel, s, x, y, w, h, event, action, primary);
}
static bool allowed(void) { return current.connected && !current.busy; }
static void invalidate_session(void) {
  memset(cc_known, 0, sizeof(cc_known));
  confirm = tool_loaded = tool_confirm = tool_pending = false;
  tool_profile = FW_UNKNOWN;
}
/* Buttons retain the session of the visible surface, even between UI ticks.
 * The shared adapter scratch snapshot also supplies the request baseline. */
static bool session_valid(bool allow_prepared) {
  fm1_state_t host;
  fm1_host_snapshot(&host);
  fm1_adapter_snapshot(&next_state);
  return host.connected && !host.updating && drawn_profile != FW_UNKNOWN &&
         host.connection_id == drawn_connection &&
         !strcmp(host.identity, drawn_identity) &&
         next_state.connected && !next_state.busy &&
         next_state.connection_id == drawn_connection &&
         next_state.profile == drawn_profile &&
         !strcmp(next_state.identity, drawn_identity) &&
         (allow_prepared || !next_state.file_prepared);
}
static bool session_action(unsigned a) {
  if ((a >= A_SCOPE && a < A_SCOPE + 3) ||
      (a >= A_TRACK && a < A_TRACK + 8) ||
      (a >= A_NOTE && a < A_NOTE + 12) ||
      (a >= A_CC_PRESS && a < A_CC_PRESS + 128))
    return true;
  switch (a) {
  case A_PAGE_PREV: case A_PAGE_NEXT: case A_REFRESH:
  case A_PROGRAM: case A_START: case A_CONTINUE: case A_STOP: case A_PANIC:
  case A_PROJECT_QUERY: case A_PROJECT_LOAD: case A_PROJECT_SAVE: case A_CONFIRM:
  case A_TOOL_READ: case A_TOOL_EXECUTE: case A_TOOL_CONFIRM:
  case A_OBJ_OPEN: case A_OBJ_READ: case A_OBJ_STAGE:
  case A_MANUAL_SEND: case A_CLOCK_ON: case A_CLOCK_OFF:
  case A_FILE_READ: case A_FILE_PREPARE: case A_FILE_CONFIRM: case A_FILE_LAST:
    return true;
  default:
    return false;
  }
}
static void accepted(bool ok) {
  fm1_ui_notice(ok ? "请求已受理。"
                   : "未发送：设备忙碌、连接变化或控制不可用。");
}
static void send(unsigned status, unsigned a, unsigned b, unsigned size,
                 unsigned gate) {
  uint8_t m[] = {status, a, b};
  accepted(fm1_adapter_midi(m, size, gate));
}
static void read_page(unsigned track) {
  accepted(fm1_adapter_view(scope, first, track));
}
static void project(unsigned op) {
  const fm1_adapter_profile_t *p = fm1_adapter_at(current.profile);
  if (p->parameters) {
    uint8_t args[] = {op, project_slot};
    accepted(fm1_adapter_command(9, args, 2));
  } else if (p->open) {
    uint8_t slot = project_slot;
    accepted(fm1_adapter_open_command(op == 0 ? 8 : op == 1 ? 7 : 9, &slot, 1));
  }
}
static unsigned project_action;
static void event(lv_event_t *e) {
  unsigned a = (uintptr_t)lv_event_get_user_data(e);
  if (rebuilding || fm1_adapter_ui_dragging())
    return;
  if (session_action(a) && !session_valid(a == A_FILE_CONFIRM)) {
    accepted(false);
    dirty = true;
    return;
  }
  if (a >= A_TAB && a < A_TAB + 5) {
    if (a == A_TAB + 4 && tab == 4)
      native_object_mode = !native_object_mode;
    tab = a - A_TAB;
    confirm = false;
    render();
    return;
  }
  if (a >= A_SCOPE && a < A_SCOPE + 3) {
    scope = a - A_SCOPE;
    first = 0;
    read_page(255);
    return;
  }
  if (a >= A_TRACK && a < A_TRACK + 8) {
    first = 0;
    read_page(a - A_TRACK);
    return;
  }
  if (a >= A_NOTE && a < A_NOTE + 12) {
    send(0x90 | channel, note_base + a - A_NOTE, 96, 3, 180);
    return;
  }
  if (a >= A_FILE_SELECT && a < A_FILE_SELECT + 16) {
    file_index = a - A_FILE_SELECT;
    render();
    return;
  }
  if (a >= A_CC_PRESS && a < A_CC_PRESS + 128) {
    send(0xb0 | channel, a - A_CC_PRESS, 127, 3, 100);
    return;
  }
  switch (a) {
  case A_CONSOLE:
    fm1_ui_route(1);
    return;
  case A_TOOL_RESULT:
    tool_show_result = !tool_show_result;
    render();
    break;
  case A_MANUAL_MODE:
    manual_mode = !manual_mode;
    render();
    break;
  case A_MANUAL_SEND:
    if (manual_values[0] == 0)
      send(0xb0 | channel, manual_values[1], manual_values[2], 3, 0);
    else if (manual_values[0] == 1)
      send(0xe0 | channel, manual_values[1] & 127, manual_values[1] >> 7, 3, 0);
    else if (manual_values[0] == 2)
      send(0xd0 | channel, manual_values[2], 0, 2, 0);
    else if (manual_values[0] == 3)
      send(0xa0 | channel, manual_values[1], manual_values[2], 3, 0);
    else if (manual_values[0] == 4)
      send(0xf2, manual_values[1] & 127, manual_values[1] >> 7, 3, 0);
    else
      send((manual_values[0] == 5 ? 0x90 : 0x80) | channel, manual_values[1],
           manual_values[2], 3, 0);
    break;
  case A_CLOCK_ON:
    accepted(fm1_host_clock(current.connection_id, manual_values[3], true));
    render();
    break;
  case A_CLOCK_OFF:
    accepted(fm1_host_clock(current.connection_id, manual_values[3], false));
    render();
    break;
  case A_OCT_DOWN:
    note_base = note_base >= 12 ? note_base - 12 : 0;
    render();
    break;
  case A_OCT_UP:
    note_base = note_base + 12 <= 116 ? note_base + 12 : 116;
    render();
    break;
  case A_OBJ_OPEN:
    if (object_index < object_count) {
      edit_target = object_ids[object_index];
      native_object_mode = true;
      accepted(fm1_adapter_object_read(edit_target));
      tab = 4;
      render();
    }
    break;
  case A_OBJ_READ:
    accepted(fm1_adapter_object_read(edit_target));
    break;
  case A_OBJ_STAGE:
    if (!context_read_pending && object_edit.loaded && object_edit.object == edit_target &&
        object_edit.connection == current.connection_id && fm1_adapter_object_stage(&object_edit)) {
      tab = 3;
      file_mode = true;
      render();
    } else
      accepted(false);
    break;
  case A_OBJ_PREV:
    if (edit_page)
      edit_page--;
    render();
    break;
  case A_OBJ_NEXT:
    if ((edit_page + 1) * 4 < edit_count)
      edit_page++;
    render();
    break;
  case A_OBJ_TRACK_DOWN:
    if (edit_track)
      edit_track--;
    edit_page = 0;
    render();
    break;
  case A_OBJ_TRACK_UP:
    if (edit_track < 7)
      edit_track++;
    edit_page = 0;
    render();
    break;
  case A_OBJ_STEP_DOWN:
    if (edit_step)
      edit_step--;
    edit_page = 0;
    render();
    break;
  case A_OBJ_STEP_UP:
    {
      unsigned maximum = 63;
      if (fm1_adapter_at(current.profile)->open && edit_target >= 100) {
        bool loaded = !context_read_pending && object_edit.loaded &&
                      object_edit.object == edit_target && object_edit.size == 223;
        maximum = loaded && object_edit.data[0] == 5 ? 7 :
                  loaded && object_edit.data[0] == 2 ? 11 : 0;
      }
      if (edit_step < maximum) edit_step++;
    }
    edit_page = 0;
    render();
    break;
  case A_TOOL_PREV:
    if (tool_page)
      tool_page--;
    render();
    break;
  case A_TOOL_NEXT:
    if ((tool_page + 1) * 4 < tool.count)
      tool_page++;
    render();
    break;
  case A_TOOL_CANCEL:
    tool_confirm = false;
    render();
    break;
  case A_TOOL_EXECUTE:
    if (tool.writes) {
      tool_confirm = true;
      tool_connection = current.connection_id;
      render();
      break;
    }
    /* fall through */
  case A_TOOL_READ:
  case A_TOOL_CONFIRM: {
    if (a == A_TOOL_CONFIRM && tool_connection != current.connection_id)
      break;
    bool read = a == A_TOOL_READ;
    uint32_t revision = next_state.result_revision;
    uint8_t args[768];
    unsigned size = 0;
    bool ok = fm1_action_encode(&tool, tool_values, read, args, &size);
    if (ok && !strcmp(tool.kind, "mmc"))
      ok = fm1_adapter_mmc(tool.command);
    else if (ok)
      ok = !strcmp(tool.kind, "open")
               ? fm1_adapter_open_command(tool.command, args, size)
               : fm1_adapter_command(read ? tool.read_command : tool.command,
                                     args, size);
    if (ok) {
      tool_pending = true;
      tool_read = read;
      tool_revision = revision;
      tool_connection = drawn_connection;
    }
    tool_confirm = false;
    accepted(ok);
    render();
    break;
  }
  case A_PAGE_PREV:
    if (first >= 4)
      first -= 4;
    read_page(255);
    break;
  case A_PAGE_NEXT:
    first += 4;
    read_page(255);
    break;
  case A_REFRESH:
    read_page(255);
    break;
  case A_CH_DOWN:
    if (channel)
      channel--;
    memset(cc_known, 0, sizeof(cc_known));
    render();
    break;
  case A_CH_UP:
    if (channel < 15)
      channel++;
    memset(cc_known, 0, sizeof(cc_known));
    render();
    break;
  case A_CC_PREV:
    if (cc_page)
      cc_page--;
    render();
    break;
  case A_CC_NEXT:
    cc_page++;
    render();
    break;
  case A_PC_DOWN:
    if (program)
      program--;
    render();
    break;
  case A_PC_UP:
    if (program < 127)
      program++;
    render();
    break;
  case A_PROGRAM:
    send(0xc0 | channel, program, 0, 2, 0);
    break;
  case A_START:
    send(0xfa, 0, 0, 1, 0);
    break;
  case A_CONTINUE:
    send(0xfb, 0, 0, 1, 0);
    break;
  case A_STOP:
    send(0xfc, 0, 0, 1, 0);
    break;
  case A_PANIC:
    if (current.profile != FW_FOMNI)
      send(0xb0 | channel, 123, 0, 3, 0);
    break;
  case A_PROJECT_DOWN:
    if (project_slot)
      project_slot--;
    confirm = false;
    render();
    break;
  case A_PROJECT_UP:
    if (project_slot < (fm1_adapter_at(current.profile)->open ? 15 : 3))
      project_slot++;
    confirm = false;
    render();
    break;
  case A_PROJECT_QUERY:
    project(2);
    break;
  case A_PROJECT_LOAD:
  case A_PROJECT_SAVE:
    project_action = a == A_PROJECT_SAVE ? 1 : 0;
    confirm_connection = current.connection_id;
    confirm = true;
    render();
    break;
  case A_CONFIRM:
    confirm = false;
    if (confirm_connection == current.connection_id && allowed())
      project(project_action);
    render();
    break;
  case A_CANCEL:
    confirm = false;
    render();
    break;
  case A_ORIGINAL:
    fm1_ui_page(0, 0);
    break;
  case A_FILE_MODE:
    file_mode = !file_mode;
    render();
    break;
  case A_FILE_SCAN:
    accepted(fm1_adapter_files_scan());
    break;
  case A_FILES_PREV:
    file_index = 0;
    accepted(
        fm1_adapter_files_page(current.file_page ? current.file_page - 1 : 0));
    break;
  case A_FILES_NEXT:
    file_index = 0;
    accepted(fm1_adapter_files_page(current.file_page + 1));
    break;
  case A_FILE_READ:
    if (object_index < object_count)
      accepted(fm1_adapter_file_read(object_ids[object_index]));
    break;
  case A_FILE_PREPARE:
    if (file_index < current.file_count)
      accepted(fm1_adapter_file_prepare(current.files[file_index]));
    break;
  case A_FILE_CONFIRM:
    accepted(fm1_adapter_file_confirm(drawn_file_token));
    break;
  case A_FILE_CANCEL:
    accepted(fm1_adapter_file_cancel(drawn_file_token));
    break;
  case A_FILE_LAST:
    accepted(fm1_adapter_file_prepare(current.file_backup));
    break;
  default:
    break;
  }
}
static void submit(row_t *row, int value) {
  if (row->manual) {
    manual_values[row->item] = value;
    dirty = true;
    return;
  }
  if (row->object) {
    fm1_object_edit(object_edit.data, object_edit.size,
                    object_fields + row->item, value);
    dirty = true;
    return;
  }
  if (row->action) {
    tool_values[row->item] = value;
    if (tool.fields[row->item].offset < tool.read_length)
      tool_loaded = false;
    dirty = true;
    return;
  }
  fm1_adapter_snapshot(&next_state);
  bool same = next_state.connected && !next_state.busy && !next_state.file_prepared &&
              next_state.connection_id == row->connection && next_state.profile == row->profile &&
              !strcmp(next_state.identity,row->identity);
  if (row->remote)
    accepted(same && next_state.page_revision == row->page && row->item < next_state.count &&
             next_state.rows[row->item].known && next_state.rows[row->item].id == row->parameter &&
             fm1_adapter_set(row->item, value, row->page));
  else {
    uint8_t m[] = {0xb0 | channel, row->item, value};
    bool ok = same && fm1_adapter_midi(m, 3, 0);
    if (ok) {
      cc_values[row->item] = value;
      cc_known[row->item] = true;
    }
    accepted(ok);
  }
  dirty = true;
}
static void slider(lv_event_t *e) {
  if (rebuilding)
    return;
  row_t *r = lv_event_get_user_data(e);
  lv_event_code_t code = lv_event_get_code(e);
  if (code == LV_EVENT_PRESSED)
    r->dragging = true;
  if (code == LV_EVENT_RELEASED) {
    r->dragging = false;
    submit(r, lv_slider_get_value(r->slider));
  }
  if (code == LV_EVENT_PRESS_LOST)
    r->dragging = false;
}
static void nudge(lv_event_t *e) {
  unsigned id = (uintptr_t)lv_event_get_user_data(e);
  row_t *r = &rows[id / 2];
  int v = lv_slider_get_value(r->slider) + (id % 2 ? 1 : -1);
  if (v >= r->min && v <= r->max)
    submit(r, v);
}
static void option(lv_event_t *e) {
  if (!rebuilding) {
    row_t *r = lv_event_get_user_data(e);
    submit(r, r->min + lv_dropdown_get_selected(r->choice));
  }
}
static void control(unsigned row, const char *name, int min, int max, int value,
                    bool known, const char *options, unsigned item,
                    bool remote) {
  row_t *r = rows + row;
  *r = (row_t){
      .item = item, .min = min, .max = max, .known = known, .remote = remote,
      .connection = current.connection_id, .profile = current.profile, .page = current.page_revision,
      .parameter = remote && item < current.count ? current.rows[item].id : 0};
  snprintf(r->identity,sizeof(r->identity),"%s",current.identity);
  int x = (row % 2) * 584, y = (row / 2) * 164;
  r->label = m1_label(content, name, x, y, 548, NULL, M1_MUTED);
  if (options && options[0]) {
    r->choice = lv_dropdown_create(content);
    lv_obj_set_pos(r->choice, x, y + 44);
    lv_obj_set_size(r->choice, 548, 80);
    lv_dropdown_set_options(r->choice, options);
    lv_dropdown_set_selected(r->choice, known ? value - min : 0);
    lv_obj_set_style_bg_color(r->choice, lv_color_hex(M1_SURFACE), 0);
    lv_obj_set_style_text_color(r->choice, lv_color_hex(M1_FG), 0);
    lv_obj_set_style_radius(r->choice, 2, 0);
    lv_obj_t *list = lv_dropdown_get_list(r->choice);
    lv_obj_set_style_bg_color(list, lv_color_hex(M1_SURFACE), 0);
    lv_obj_set_style_text_color(list, lv_color_hex(M1_FG), 0);
    lv_dropdown_set_symbol(r->choice, "v");
    lv_obj_add_event_cb(r->choice, option, LV_EVENT_VALUE_CHANGED, r);
    m1_enabled(r->choice, allowed() && known);
    return;
  }
  r->minus = m1_button(content, "-", x, y + 44, 80, 80, nudge, row * 2, false);
  r->plus = m1_button(content, "+", x + 468, y + 44, 80, 80, nudge, row * 2 + 1,
                      false);
  r->slider = lv_slider_create(content);
  lv_obj_set_pos(r->slider, x + 112, y + 80);
  lv_obj_set_size(r->slider, 324, 8);
  lv_slider_set_range(r->slider, min, max);
  lv_slider_set_value(r->slider, known ? value : min, LV_ANIM_OFF);
  lv_obj_set_ext_click_area(r->slider, 32);
  lv_obj_set_style_bg_color(r->slider, lv_color_hex(M1_LINE), LV_PART_MAIN);
  lv_obj_set_style_bg_color(r->slider, lv_color_hex(M1_GREEN),
                            LV_PART_INDICATOR);
  lv_obj_set_style_bg_color(r->slider, lv_color_hex(M1_FG), LV_PART_KNOB);
  lv_obj_set_style_radius(r->slider, 0, LV_PART_KNOB);
  lv_obj_add_event_cb(r->slider, slider, LV_EVENT_ALL, r);
  m1_enabled(r->slider, allowed() && (!remote || known));
  m1_enabled(r->minus, allowed() && known);
  m1_enabled(r->plus, allowed() && known);
}
static void parameters(void) {
  const fm1_adapter_profile_t *p = fm1_adapter_at(current.profile);
  if (!p->parameters) {
    m1_label(panel, "此固件通过演奏控制或工程交换扩展。", 0, 162, 1128,
             &m1_font_28, M1_FG);
    m1_label(panel, p->gap, 0, 234, 1128, NULL, M1_MUTED);
    if (current.profile == FW_STOCK)
      button("打开六算子编辑器", 0, 350, 400, 88, A_ORIGINAL, true);
    return;
  }
  const char *scopes[] = {"轨道参数", "全局参数", "设备设置"};
  unsigned total = scope == 2 ? current.menu_count
                   : scope    ? current.globals
                              : current.parameters;
  for (unsigned i = 0; i < (current.profile == FW_FELUCCA ? 3u : 2u); i++) {
    lv_obj_t *b = button(scopes[i], i * 188, 86, 176, 64, A_SCOPE + i, false);
    m1_selected(b, scope == i);
    m1_enabled(b, allowed());
  }
  for (unsigned i = 0; i < 4; i++) {
    char s[12];
    snprintf(s, sizeof(s), "T%u", i + 1);
    lv_obj_t *b = button(s, 596 + i * 96, 86, 80, 64, A_TRACK + i, false);
    m1_selected(b, current.selected == i);
    m1_enabled(b, allowed());
  }
  button("读取", 1000, 86, 132, 64, A_REFRESH, false);
  content = m1_panel(panel, 0, 174, 1144, 320, true);
  for (unsigned i = 0; i < current.count; i++) {
    fm1_remote_parameter_t *r = current.rows + i;
    char label[100];
    if (r->known)
      snprintf(label, sizeof(label), "%s   %d %s", r->name, r->value, r->unit);
    else
      snprintf(label, sizeof(label), "%s   --", r->name);
    control(i, label, r->minimum, r->maximum, r->value, r->known, r->options, i,
            true);
  }
  if (!current.count)
    m1_label(content, "点击读取，从设备获取参数和范围。", 0, 24, 1120, NULL,
             M1_MUTED);
  char page[80];
  snprintf(page, sizeof(page), "%u / %u    |    %s", total ? first / 4 + 1 : 0,
           (total + 3) / 4, current.version);
  m1_label(panel, page, 220, 514, 680, &lv_font_montserrat_16, M1_MUTED);
  lv_obj_t *b = button("上一页", 0, 504, 180, 48, A_PAGE_PREV, false);
  m1_enabled(b, allowed() && first >= 4);
  b = button("下一页", 952, 504, 180, 48, A_PAGE_NEXT, false);
  m1_enabled(b, allowed() && first + 4 < total);
}
static void channel_control(void) {
  button("-", 0, 86, 64, 64, A_CH_DOWN, false);
  char s[80];
  snprintf(s, sizeof(s), "MIDI  %02u", channel + 1);
  m1_label(panel, s, 80, 102, 200, NULL, M1_MUTED);
  button("+", 252, 86, 64, 64, A_CH_UP, false);
}
static const cc_t *cc_list(unsigned *count) {
  if (current.profile == FW_VA) {
    *count = sizeof(va_cc) / sizeof(*va_cc);
    return va_cc;
  }
  static cc_t filtered[32];
  *count = 0;
  for (unsigned i = 0; i < sizeof(common_cc) / sizeof(*common_cc); i++) {
    unsigned c = common_cc[i].cc;
    bool supported = false;
    if (current.profile == FW_JANGADA)
      supported =
          c == 1 || c == 11 || c == 64 || c >= 120 || (c >= 16 && c <= 19);
    else if (current.profile == FW_SLOOP)
      supported = c == 5 || c == 7 || c == 10 || c == 64 ||
                  (c >= 71 && c <= 75) || c == 91 || c == 93 || c == 94;
    else if (current.profile == FW_FELUCCA)
      supported = !(c >= 16 && c <= 19);
    else if (fm1_adapter_at(current.profile)->open)
      supported = c == 1 || c == 7 || c == 10 || c == 11 || c == 64 ||
                  c == 71 || c == 72 || c == 74 || c == 75 || c == 91 ||
                  c >= 120;
    else
      supported = c == 1 || c == 7 || c == 11 || c == 64 || c >= 120;
    if (supported)
      filtered[(*count)++] = common_cc[i];
  }
  return filtered;
}
static void controllers(void) {
  channel_control();
  m1_label(panel, "最近受理值；实际发送结果见下方状态。", 348, 102, 548,
           &m1_font_20, M1_MUTED);
  button(manual_mode ? "预定义控制" : "更多 MIDI", 920, 86, 212, 64,
         A_MANUAL_MODE, false);
  if (manual_mode) {
    const char *labels[] = {"消息 · 0CC 1弯音 2压力 3键压 4位置 5按下 6释放",
                            "CC / 音符 / 14位数值", "CC 值 / 压力值",
                            "时钟 BPM"};
    const int min[] = {0, 0, 0, 30}, max[] = {6, 16383, 127, 300};
    content = m1_panel(panel, 0, 174, 1144, 320, true);
    for (unsigned i = 0; i < 4; i++) {
      char label[140];
      snprintf(label, sizeof(label), "%s  %d", labels[i], manual_values[i]);
      control(i, label, min[i], max[i], manual_values[i], true, NULL, i, false);
      rows[i].manual = true;
      lv_obj_set_style_text_font(rows[i].label, &m1_font_20, 0);
    }
    lv_obj_t *b = button("发送 MIDI", 0, 504, 320, 48, A_MANUAL_SEND, true);
    m1_enabled(b, allowed() && ((manual_values[0] == 0 ||
                                 manual_values[0] == 3 || manual_values[0] >= 5)
                                    ? manual_values[1] < 128
                                    : true));
    b = button("启动时钟 · 24 PPQN", 352, 504, 376, 48, A_CLOCK_ON, false);
    m1_enabled(b, allowed());
    fm1_state_t host;fm1_host_snapshot(&host);m1_selected(b,host.clock_enabled);
    b = button("停止时钟", 760, 504, 372, 48, A_CLOCK_OFF, false);
    m1_enabled(b, allowed());
    return;
  }
  if (current.profile == FW_FOMNI || current.profile == FW_X0X) {
    m1_label(panel, "此固件未开放这些 CC；使用演奏页。", 0, 224, 1120,
             &m1_font_28, M1_FG);
    return;
  }
  unsigned count;
  const cc_t *list = cc_list(&count);
  if (cc_page * 4 >= count)
    cc_page = 0;
  content = m1_panel(panel, 0, 174, 1144, 320, true);
  for (unsigned i = 0; i < 4 && cc_page * 4 + i < count; i++) {
    const cc_t *c = list + cc_page * 4 + i;
    if (c->momentary) {
      m1_label(content, c->label, (i % 2) * 584, (i / 2) * 164, 548, NULL,
               M1_MUTED);
      lv_obj_t *b =
          m1_button(content, "触发", (i % 2) * 584, (i / 2) * 164 + 44, 548, 80,
                    event, A_CC_PRESS + c->cc, false);
      m1_enabled(b, allowed());
    } else {
      char label[100];
      if (cc_known[c->cc])
        snprintf(label, sizeof(label), "%s   %d", c->label, cc_values[c->cc]);
      else
        snprintf(label, sizeof(label), "%s   --", c->label);
      control(i, label, 0, c->max, cc_values[c->cc], cc_known[c->cc], NULL,
              c->cc, false);
    }
  }
  lv_obj_t *b = button("上一页", 0, 504, 180, 48, A_CC_PREV, false);
  m1_enabled(b, cc_page > 0);
  char s[60];
  snprintf(s, sizeof(s), "%u / %u", cc_page + 1, (count + 3) / 4);
  m1_label(panel, s, 530, 514, 200, &lv_font_montserrat_16, M1_MUTED);
  b = button("下一页", 952, 504, 180, 48, A_CC_NEXT, false);
  m1_enabled(b, (cc_page + 1) * 4 < count);
}
static void performance(void) {
  channel_control();
  const char *help = current.profile == FW_FOMNI
                         ? "通道 1：拨弦  /  通道 2：和弦"
                     : current.profile == FW_OPEN || current.profile >= FW_PROBE
                         ? "轨道 1–7：通道 1–7  /  轨道 8：通道 10"
                         : "按设备 MIDI 设置选择接收通道。";
  m1_label(panel, help, 360, 102, 772, NULL, M1_MUTED);
  const char *notes[] = {"C",  "C#", "D",  "D#", "E",  "F",
                         "F#", "G",  "G#", "A",  "A#", "B"};
  for (unsigned i = 0; i < 12; i++) {
    char label[20];
    snprintf(label, sizeof(label), "%s\n%u", notes[(note_base + i) % 12],
             note_base + i);
    lv_obj_t *b = button(label, i * 95, 174, 84, 128, A_NOTE + i, false);
    m1_enabled(b, allowed());
  }
  button("八度 -", 0, 318, 180, 56, A_OCT_DOWN, false);
  button("八度 +", 200, 318, 180, 56, A_OCT_UP, false);
  m1_label(panel, "音符范围可达 0–127；X0X 鼓与切片从 36 开始。", 412, 334, 720,
           &m1_font_20, M1_MUTED);
  const char *transport[] = {"开始", "继续", "停止", "释放音符"};
  unsigned actions[] = {A_START, A_CONTINUE, A_STOP, A_PANIC};
  for (unsigned i = 0; i < 4; i++) {
    lv_obj_t *b =
        button(i == 3 && current.profile == FW_FOMNI ? "和弦面板" : transport[i],
               i * 288, 392, 268, 64,
               i == 3 && current.profile == FW_FOMNI ? A_CONSOLE : actions[i], i == 2);
    m1_enabled(b, (i == 3 && current.profile == FW_FOMNI) || allowed());
  }
  if (current.profile != FW_FOMNI && current.profile != FW_X0X) {
    button("-", 0, 472, 80, 72, A_PC_DOWN, false);
    char text[64];
    snprintf(text, sizeof(text), "PROGRAM  %03u", program + 1);
    m1_label(panel, text, 104, 492, 400, &lv_font_montserrat_16, M1_MUTED);
    button("+", 480, 472, 80, 72, A_PC_UP, false);
    lv_obj_t *b = button("选择预设", 608, 472, 524, 72, A_PROGRAM, false);
    m1_enabled(b, allowed());
  } else
    m1_label(panel, "音符为短按试听；外部同步还需要 MIDI 时钟。", 0, 482, 1132,
             NULL, M1_MUTED);
}
static void projects(void) {
  const fm1_adapter_profile_t *p = fm1_adapter_at(current.profile);
  if (!p->parameters && !p->open) {
    m1_label(panel, p->gap, 0, 162, 1128, &m1_font_28, M1_FG);
    return;
  }
  char name[80];
  snprintf(name, sizeof(name), "PROJECT  %02u", project_slot + 1);
  m1_label(panel, name, 0, 110, 900, &m1_pixel_48, M1_FG);
  button("-", 660, 88, 128, 80, A_PROJECT_DOWN, false);
  button("+", 804, 88, 128, 80, A_PROJECT_UP, false);
  if (confirm) {
    m1_label(panel,
             project_action ? "保存将覆盖这个设备工程槽位。"
                            : "载入将替换当前未保存的工程。",
             0, 236, 1132, &m1_font_28, M1_FG);
    button("返回", 0, 416, 320, 88, A_CANCEL, false);
    lv_obj_t *b = button(project_action ? "确认保存" : "确认载入", 760, 416,
                         372, 88, A_CONFIRM, true);
    m1_enabled(b, allowed());
    return;
  }
  m1_label(panel, "设备工程槽位。载入或覆盖前核对目标。", 0, 210, 1132, NULL,
           M1_MUTED);
  lv_obj_t *b = button("查询槽位", 0, 300, 356, 88, A_PROJECT_QUERY, false);
  m1_enabled(b, allowed());
  b = button("载入工程", 388, 300, 356, 88, A_PROJECT_LOAD, false);
  m1_enabled(b, allowed());
  b = button("保存工程", 776, 300, 356, 88, A_PROJECT_SAVE, false);
  m1_enabled(b, allowed());
  if (current.result_revision) {
    if (p->parameters && current.result_command == 9 &&
        current.result_size >= 3) {
      const uint8_t *r = current.result;
      char s[100];
      snprintf(s, sizeof(s), "槽位 %u  /  %s%s", r[1] + 1,
               r[2] ? "有工程" : "空槽",
               current.result_size > 3 && r[3] ? " · 设备拒绝操作" : "");
      m1_label(panel, s, 0, 442, 1132, NULL, M1_MUTED);
    } else if (p->open && current.result_command == 9 &&
               current.result_size == 12) {
      char s[13] = {0};
      memcpy(s, current.result, 12);
      m1_label(panel, s, 0, 442, 1132, NULL, M1_MUTED);
    }
  }
}
static void object_selected(lv_event_t *e) {
  if (!rebuilding)
    object_index = lv_dropdown_get_selected(lv_event_get_target_obj(e));
}
static void files(void) {
  if (current.file_prepared) {
    drawn_file_token = current.file_token;
    m1_label(panel, "替换设备对象", 0, 110, 1100, &m1_font_28, M1_FG);
    m1_label(panel, current.file_name, 0, 174, 1132, NULL, M1_YELLOW);
    char text[160];
    snprintf(text, sizeof(text), "%s  /  对象 %u  /  %u bytes",
             current.identity, current.file_object, current.file_size);
    m1_label(panel, text, 0, 232, 1132, NULL, M1_MUTED);
    m1_label(
        panel,
        current.profile == FW_FELUCCA && current.file_object == 90
            ? "替换整个 FM6 库。协议无法备份旧库或读回；以设备 CRC 和目录核对。"
            : "先备份旧内容，再写入并完整读回。部分操作会停止播放。",
        0, 294, 1132, NULL, M1_MUTED);
    lv_obj_t *b = button("返回", 0, 430, 320, 88, A_FILE_CANCEL, false);
    m1_enabled(b, !current.busy);
    b = button("确认替换", 752, 430, 380, 88, A_FILE_CONFIRM, true);
    m1_enabled(b,
               allowed() && current.file_connection == current.connection_id);
    return;
  }
  m1_label(panel, "SD / fm1 / exchange", 0, 100, 600, &lv_font_montserrat_16,
           M1_MUTED);
  lv_obj_t *b = button("刷新文件", 0, 144, 240, 64, A_FILE_SCAN, false);
  m1_enabled(b, allowed());
  b = button("<", 256, 144, 80, 64, A_FILES_PREV, false);
  m1_enabled(b, allowed() && current.file_page > 0);
  char pg[30];
  snprintf(pg, sizeof(pg), "%u/%u", current.file_page + 1,
           current.file_pages ? current.file_pages : 1);
  m1_label(panel, pg, 350, 164, 98, &lv_font_montserrat_16, M1_MUTED);
  b = button(">", 468, 144, 80, 64, A_FILES_NEXT, false);
  m1_enabled(b, allowed() && current.file_page + 1 < current.file_pages);
  content = m1_panel(panel, 0, 228, 548, 312, true);
  for (unsigned i = 0; i < current.file_count; i++) {
    b = m1_button(content, current.files[i], 0, i * 84, 528, 76, event,
                  A_FILE_SELECT + i, false);
    lv_obj_set_style_text_font(lv_obj_get_child(b, 0), &lv_font_montserrat_16,
                               0);
    m1_selected(b, i == file_index);
  }
  if (!current.file_count)
    m1_label(content, "先读取设备对象，或将 .m1x 文件放入此目录。", 0, 10, 500,
             NULL, M1_MUTED);
  static char options[4096];
  options[0] = 0;
  unsigned used = 0;
  object_count = 0;
  const fm1_adapter_profile_t *p = fm1_adapter_at(current.profile);
  for (unsigned obj = 0; obj < 256; obj++) {
    char label[80] = {0};
    if (p->parameters) {
      if (p->id == FW_FELUCCA && obj == 90)
        snprintf(label, sizeof(label), "32 音色库 · 仅文件导入");
      else if (obj == 0)
        snprintf(label, sizeof(label), "当前工程");
      else if (obj == 1)
        snprintf(label, sizeof(label), "设备设置");
      else if (obj >= 2 && obj <= 5)
        snprintf(label, sizeof(label), "工程槽位 %u", obj - 1);
      else if (obj == 6 || obj == 7)
        snprintf(label, sizeof(label), "用户音色 %u–%u", (obj - 6) * 16 + 1,
                 (obj - 5) * 16);
      else if (obj >= 8 && obj <= (p->id == FW_FELUCCA ? 9 : 11))
        snprintf(label, sizeof(label), "%s %u",
                 p->id == FW_SLOOP ? (obj == 8    ? "FM6 音色库"
                                      : obj == 9  ? "鼓合成套件"
                                      : obj == 10 ? "MIDI 学习"
                                                  : "主题")
                                   : "FM6 音色库",
                 obj);
      else if (obj >= 32 && obj <= (p->id == FW_SLOOP ? 35 : 34))
        snprintf(label, sizeof(label), "采样槽位 %u", obj - 31);
    } else if (p->id == FW_X0X && obj >= 32 && obj <= 34)
      snprintf(label, sizeof(label), "采样槽位 %u", obj - 31);
    else if (p->open) {
      if (obj == 0)
        snprintf(label, sizeof(label), "当前八轨工程");
      else if (obj == 32)
        snprintf(label, sizeof(label), "全部八槽采样库");
      else if (obj >= 100 && obj < 108)
        snprintf(label, sizeof(label), "轨道 %u 音色", obj - 99);
      else if (obj >= 200 && obj < 208)
        snprintf(label, sizeof(label), "轨道 %u FM 音色", obj - 199);
    } else if (p->id == FW_VA && obj < 144)
      snprintf(label, sizeof(label),
               obj < 128 ? "音色槽位 %03u" : "Pattern %02u",
               obj < 128 ? obj + 1 : obj - 127);
    if (label[0]) {
      int count = snprintf(options + used, sizeof(options) - used, "%s%s",
                           object_count ? "\n" : "", label);
      if (count < 0 || (unsigned)count >= sizeof(options) - used)
        break;
      used += count;
      object_ids[object_count++] = obj;
    }
  }
  if (object_index >= object_count)
    object_index = 0;
  m1_label(panel, "选择设备对象", 592, 100, 340, NULL, M1_MUTED);
  lv_obj_t *list = lv_dropdown_create(panel);
  lv_obj_set_pos(list, 592, 144);
  lv_obj_set_size(list, 540, 72);
  lv_dropdown_set_options(list,
                          options[0] ? options : "此固件没有对象交换协议");
  lv_dropdown_set_selected(list, object_index);
  lv_obj_set_style_bg_color(list, lv_color_hex(M1_SURFACE), 0);
  lv_obj_set_style_text_color(list, lv_color_hex(M1_FG), 0);
  lv_dropdown_set_symbol(list, "v");
  lv_obj_set_style_bg_color(lv_dropdown_get_list(list),
                            lv_color_hex(M1_SURFACE), 0);
  lv_obj_set_style_text_color(lv_dropdown_get_list(list), lv_color_hex(M1_FG),
                              0);
  lv_obj_add_event_cb(list, object_selected, LV_EVENT_VALUE_CHANGED, NULL);
  b = button("读取到 SD", 592, 248, 260, 80, A_FILE_READ, false);
  m1_enabled(b, allowed() && object_count &&
                    !(p->id == FW_FELUCCA && object_ids[object_index] == 90));
  b = button("在 Tab5 编辑", 872, 248, 260, 80, A_OBJ_OPEN, false);
  m1_enabled(b, allowed() && object_count &&
                    (p->open || p->id == FW_VA ||
                     (p->id == FW_SLOOP && object_ids[object_index] >= 9 &&
                      object_ids[object_index] <= 11)) &&
                    !(p->open && object_ids[object_index] == 32));
  b = button("准备写入选中文件", 592, 352, 540, 80, A_FILE_PREPARE, true);
  m1_enabled(b, allowed() && file_index < current.file_count);
  b = button("准备恢复最近备份", 592, 456, 540, 80, A_FILE_LAST, false);
  m1_enabled(b, allowed() && current.file_backup[0]);
}
static void tool_select(lv_event_t *e) {
  if (rebuilding || current.busy)
    return;
  tool_index = lv_dropdown_get_selected(lv_event_get_target_obj(e));
  tool_profile = FW_UNKNOWN;
  render();
}
static void object_section_changed(lv_event_t *e) {
  if (!rebuilding) {
    edit_section = lv_dropdown_get_selected(lv_event_get_target_obj(e));
    edit_page = 0;
    render();
  }
}
static void object_editor(void) {
  fm1_adapter_object_snapshot(&object_next);
  if (context_read_pending && object_next.loaded && object_next.object == edit_target &&
      object_next.connection == current.connection_id && object_next.revision != context_read_revision)
    context_read_pending = false;
  if (object_edit.revision != object_next.revision ||
      object_edit.connection != object_next.connection) {
    object_edit = object_next;
    edit_page = 0;
  }
  bool valid =
      object_edit.loaded && object_edit.connection == current.connection_id &&
      object_edit.object == edit_target && !context_read_pending;
  lv_obj_t *list = lv_dropdown_create(panel);
  lv_obj_set_pos(list, 0, 86);
  lv_obj_set_size(list, 480, 64);
  bool track_voice = fm1_adapter_at(current.profile)->open && edit_target >= 100;
  lv_dropdown_set_options(list, track_voice ? "当前轨音色" :
                          "工程设置\n单步音符\n和弦\n参数锁\n琶音\n轨道设置");
  if (track_voice) edit_section = 0;
  lv_dropdown_set_selected(list, edit_section);
  lv_dropdown_set_symbol(list, "v");
  lv_obj_set_style_bg_color(list, lv_color_hex(M1_SURFACE), 0);
  lv_obj_set_style_text_color(list, lv_color_hex(M1_FG), 0);
  lv_obj_set_style_bg_color(lv_dropdown_get_list(list),
                            lv_color_hex(M1_SURFACE), 0);
  lv_obj_set_style_text_color(lv_dropdown_get_list(list), lv_color_hex(M1_FG),
                              0);
  lv_obj_add_event_cb(list, object_section_changed, LV_EVENT_VALUE_CHANGED,
                      NULL);
  bool drum_variant = track_voice && valid && object_edit.size == 223 && object_edit.data[0] == 2;
  m1_enabled(button("-", 512, 86, 60, 64, A_OBJ_TRACK_DOWN, false), !track_voice || drum_variant);
  m1_enabled(button("+", 744, 86, 60, 64, A_OBJ_TRACK_UP, false), !track_voice || drum_variant);
  unsigned part_max = track_voice ? (valid && object_edit.size == 223 && object_edit.data[0] == 5 ? 7 :
                                    valid && object_edit.size == 223 && object_edit.data[0] == 2 ? 11 : 0) : 63;
  if (track_voice && valid && edit_step > part_max) edit_step = part_max;
  m1_enabled(button("-", 832, 86, 60, 64, A_OBJ_STEP_DOWN, false), edit_step > 0 && (!track_voice || (valid && part_max)));
  m1_enabled(button("+", 1072, 86, 60, 64, A_OBJ_STEP_UP, false), edit_step < part_max && (!track_voice || valid));
  char text[120];
  snprintf(text, sizeof(text), track_voice ? "VOICE T%u" : "T%u",
           track_voice ? edit_target % 100 + 1 : edit_track + 1);
  if (drum_variant) snprintf(text, sizeof(text), "VAR %u", edit_track % 2 + 1);
  m1_label(panel, text, 600, 102, 130, NULL, M1_MUTED);
  snprintf(text, sizeof(text), track_voice ? "PART %02u" : "STEP %02u", edit_step + 1);
  m1_label(panel, text, 904, 104, 160, &lv_font_montserrat_16, M1_MUTED);
  content = m1_panel(panel, 0, 174, 1144, 320, true);
  edit_count = valid ? fm1_object_fields(current.profile, object_edit.object,
                                         object_edit.data, object_edit.size,
                                         edit_section, edit_track, edit_step,
                                         object_fields, 192)
                     : 0;
  if (edit_page * 4 >= edit_count)
    edit_page = 0;
  for (unsigned i = 0; i < 4 && edit_page * 4 + i < edit_count; i++) {
    unsigned id = edit_page * 4 + i;
    const fm1_object_field_t *f = object_fields + id;
    int v = fm1_object_value(object_edit.data, f);
    char label[120];
    snprintf(label, sizeof(label), "%s  %d", f->name, v);
    control(i, label, f->minimum, f->maximum, v, true, NULL, id, false);
    rows[i].object = true;
    lv_obj_set_style_text_font(rows[i].label, &m1_font_20, 0);
  }
  if (!edit_count)
    m1_label(content,
             valid ? "此对象在当前分类没有字段；工程文件页可选择音色或图案。"
                   : "先在工程文件页选择对象，或读取当前工程。",
             0, 24, 1120, NULL, M1_MUTED);
  lv_obj_t *b = button("上一页", 0, 504, 144, 48, A_OBJ_PREV, false);
  m1_enabled(b, edit_page > 0);
  b = button("下一页", 156, 504, 144, 48, A_OBJ_NEXT, false);
  m1_enabled(b, (edit_page + 1) * 4 < edit_count);
  snprintf(text, sizeof(text), "%u / %u", edit_count ? edit_page + 1 : 0,
           (edit_count + 3) / 4);
  m1_label(panel, text, 316, 507, 275, &lv_font_montserrat_16, M1_MUTED);
  m1_label(panel, "点编排切换工具", 316, 530, 275, &m1_font_20, M1_MUTED);
  b = button("重新读取", 600, 504, 200, 48, A_OBJ_READ, false);
  m1_enabled(b, allowed());
  b = button("保存副本并准备写入", 820, 504, 312, 48, A_OBJ_STAGE, true);
  m1_enabled(b, allowed() && valid && edit_count);
}
static void action_tools(void) {
  unsigned count = fm1_action_count(current.profile);
  if (native_object_mode) {
    object_editor();
    return;
  }
  if (!count) {
    if (fm1_adapter_at(current.profile)->open || current.profile == FW_VA) {
      object_editor();
      return;
    }
    m1_label(panel, "此固件使用参数、演奏及工程交换入口。", 0, 188, 1128,
             &m1_font_28, M1_FG);
    m1_label(panel, fm1_adapter_at(current.profile)->gap, 0, 266, 1128, NULL,
             M1_MUTED);
    return;
  }
  if (tool_index >= count)
    tool_index = 0;
  if (tool_profile != current.profile) {
    fm1_action_build(current.profile, tool_index, &tool);
    tool_profile = current.profile;
    tool_page = 0;
    tool_loaded = false;
    tool_pending = false;
    tool_confirm = false;
    tool_result[0] = 0;
    tool_detail[0] = 0;
    tool_show_result = false;
    for (unsigned i = 0; i < tool.count; i++)
      tool_values[i] = tool.fields[i].initial;
    if (tool_initial_track < 4)
      for (unsigned i = 0; i < tool.count; i++)
        if (!strcmp(tool.fields[i].name, "轨道 (0–3)"))
          tool_values[i] = tool_initial_track;
    tool_initial_track = 255;
  }
  if (tool_confirm) {
    m1_label(panel, tool.name, 0, 110, 1132, &m1_font_28, M1_FG);
    m1_label(panel, current.identity, 0, 176, 1132, NULL, M1_YELLOW);
    m1_label(panel, "将应用刚才各页中的输入值。涉及存储的操作会覆盖对应槽位。",
             0, 240, 1132, NULL, M1_MUTED);
    m1_label(panel, "载入、清空或保存操作可能停止播放。", 0, 294, 1132, NULL,
             M1_MUTED);
    button("返回修改", 0, 430, 320, 88, A_TOOL_CANCEL, false);
    lv_obj_t *b = button("确认应用", 752, 430, 380, 88, A_TOOL_CONFIRM, true);
    m1_enabled(b, allowed() && tool_connection == current.connection_id);
    return;
  }
  static char options[8192];
  unsigned used = 0;
  options[0] = 0;
  static fm1_action_t item;
  for (unsigned i = 0; i < count; i++) {
    fm1_action_build(current.profile, i, &item);
    int n = snprintf(options + used, sizeof(options) - used, "%s%s",
                     i ? "\n" : "", item.name);
    if (n < 0 || (unsigned)n >= sizeof(options) - used)
      break;
    used += n;
  }
  lv_obj_t *list = lv_dropdown_create(panel);
  lv_obj_set_pos(list, 0, 86);
  lv_obj_set_size(list, 520, 64);
  lv_dropdown_set_options(list, options);
  lv_dropdown_set_selected(list, tool_index);
  lv_dropdown_set_symbol(list, "v");
  lv_obj_set_style_bg_color(list, lv_color_hex(M1_SURFACE), 0);
  lv_obj_set_style_text_color(list, lv_color_hex(M1_FG), 0);
  lv_obj_set_style_bg_color(lv_dropdown_get_list(list),
                            lv_color_hex(M1_SURFACE), 0);
  lv_obj_set_style_text_color(lv_dropdown_get_list(list), lv_color_hex(M1_FG),
                              0);
  lv_obj_add_event_cb(list, tool_select, LV_EVENT_VALUE_CHANGED, NULL);
  m1_enabled(list, allowed());
  if (tool.read_command) {
    lv_obj_t *b = button("读取", 540, 86, 140, 64, A_TOOL_READ, false);
    m1_enabled(b, allowed());
  }
  lv_obj_t *b = button(tool.writes ? "应用输入" : "查询", 860, 86, 272, 64,
                       A_TOOL_EXECUTE, tool.writes);
  m1_enabled(b, allowed() && (!tool.read_command || tool_loaded));
  b = button(tool_show_result ? "输入" : "结果", 700, 86, 140, 64,
             A_TOOL_RESULT, false);
  m1_enabled(b, tool_detail[0]);
  content = m1_panel(panel, 0, 174, 1144, 320, true);
  if (tool_show_result) {
    m1_label(content, tool_detail, 0, 0, 1116, &m1_font_20, M1_FG);
    return;
  }
  for (unsigned i = 0; i < 4 && tool_page * 4 + i < tool.count; i++) {
    unsigned id = tool_page * 4 + i;
    const fm1_action_field_t *f = tool.fields + id;
    char label[128];
    snprintf(label, sizeof(label), "%s  %d", f->name, tool_values[id]);
    control(i, label, f->minimum, f->maximum, tool_values[id], true, NULL, id,
            false);
    rows[i].action = true;
    lv_obj_set_style_text_font(rows[i].label, &m1_font_20, 0);
  }
  if (!tool.count)
    m1_label(content,
             tool_result[0] ? tool_result
                            : "点击查询读取设备；写入前会再次显示确认。",
             0, 12, 1116, NULL, M1_MUTED);
  b = button("上一页", 0, 504, 180, 48, A_TOOL_PREV, false);
  m1_enabled(b, tool_page > 0);
  char page[140];
  snprintf(page, sizeof(page), "%u / %u  ·  %.96s",
           tool.count ? tool_page + 1 : 0, (tool.count + 3) / 4,
           tool_result[0] ? tool_result
           : tool_loaded  ? "已读取；修改输入后应用"
                          : "输入值；读取后再编辑");
  m1_label(panel, page, 204, 512, 722, &m1_font_20, M1_MUTED);
  b = button("下一页", 952, 504, 180, 48, A_TOOL_NEXT, false);
  m1_enabled(b, (tool_page + 1) * 4 < tool.count);
}
static void render(void) {
  if (!panel)
    return;
  if (fm1_adapter_ui_dragging()) {
    dirty = true;
    return;
  }
  rebuilding = true;
  lv_obj_clean(panel);
  memset(rows, 0, sizeof(rows));
  content = NULL;
  fm1_adapter_snapshot(&current);
  if (drawn_connection != current.connection_id || drawn_profile != current.profile ||
      strcmp(drawn_identity, current.identity))
    invalidate_session();
  drawn_page = current.page_revision;
  drawn_profile = current.profile;
  drawn_connection = current.connection_id;
  snprintf(drawn_identity, sizeof(drawn_identity), "%s", current.identity);
  const fm1_adapter_profile_t *p = fm1_adapter_at(current.profile);
  button("返回", 0, 0, 88, 64, A_CONSOLE, false);
  heading = m1_label(panel, p->name, 108, 18, 308, &m1_font_24, M1_FG);
  lv_obj_set_height(heading,36);lv_label_set_long_mode(heading,LV_LABEL_LONG_DOT);
  const char *tabs[] = {"参数", "控制", "演奏", "工程", "编排"};
  for (unsigned i = 0; i < 5; i++) {
    lv_obj_t *b = button(tabs[i], 436 + i * 140, 0, 128, 64, A_TAB + i, false);
    m1_selected(b, tab == i);
  }
  if (tab == 0)
    parameters();
  else if (tab == 1)
    controllers();
  else if (tab == 2)
    performance();
  else if (tab == 4)
    action_tools();
  else {
    if (file_mode || current.file_prepared)
      files();
    else
      projects();
    if (!current.file_prepared)
      button(file_mode ? "设备槽位" : "文件交换", 960, 82, 172, 48, A_FILE_MODE,
             false);
  }
  rebuilding = false;
  dirty = false;
}
void fm1_adapter_ui_open(lv_obj_t *parent) {
  panel = parent;
  tab = 0;
  scope = 0;
  first = 0;
  cc_page = 0;
  fm1_adapter_snapshot(&current);
  edit_target = current.profile == FW_VA ? 128 : 0;
  context_read_pending = false;
  tool_initial_track = 255;
  native_object_mode =
      fm1_adapter_at(current.profile)->open || current.profile == FW_VA;
  confirm = false;
  memset(cc_known, 0, sizeof(cc_known));
  fm1_adapter_visible(true);
  render();
}
void fm1_adapter_ui_close(void) {
  panel = content = message = heading = track_label = NULL;
  memset(rows, 0, sizeof(rows));
  fm1_adapter_visible(false);
}
bool fm1_adapter_ui_page(unsigned page) {
  if (!panel || page > 5 || fm1_adapter_ui_dragging())
    return false;
  tab = page == 5 ? 3 : page;
  if (page == 5)
    file_mode = true;
  confirm = tool_confirm = false;
  render();
  return true;
}
bool fm1_adapter_ui_object(unsigned object, unsigned section, unsigned track, unsigned step) {
  if (!panel || fm1_adapter_ui_dragging() || section > 5 || track > 7 || step > 63)
    return false;
  fm1_adapter_snapshot(&current);
  if (!fm1_adapter_at(current.profile)->open ||
      !(object == 0 || (object >= 100 && object < 108) || (object >= 200 && object < 208)))
    return false;
  tab = 4;
  native_object_mode = true;
  edit_target = object;
  edit_section = section;
  edit_track = object >= 100 ? 0 : track;
  edit_step = step;
  edit_page = 0;
  /* The editor must not show a previous track's cached object during the read. */
  fm1_adapter_object_snapshot(&object_next);
  context_read_revision = object_next.revision;
  context_read_pending = true;
  memset(&object_edit, 0, sizeof(object_edit));
  accepted(fm1_adapter_object_read(object));
  render();
  return true;
}
bool fm1_adapter_ui_action(unsigned index) {
  if (!panel || fm1_adapter_ui_dragging()) return false;
  fm1_adapter_snapshot(&current);
  if (index >= fm1_action_count(current.profile)) return false;
  tab = 4;
  native_object_mode = false;
  tool_index = index;
  tool_initial_track = current.selected;
  tool_profile = FW_UNKNOWN;
  confirm = tool_confirm = tool_loaded = tool_pending = false;
  render();
  return true;
}
bool fm1_adapter_ui_dragging(void) {
  if (!panel) return false;
  for (unsigned i = 0; i < 4; i++)
    if (rows[i].dragging || (rows[i].choice && lv_dropdown_is_open(rows[i].choice)))
      return true;
  return false;
}
void fm1_adapter_ui_tick(void) {
  if (!panel)
    return;
  bool dragging = fm1_adapter_ui_dragging();
  fm1_adapter_snapshot(&next_state);
  bool changed = current.busy != next_state.busy ||
                 current.connected != next_state.connected ||
                 current.result_revision != next_state.result_revision;
  if (drawn_connection != next_state.connection_id || drawn_profile != next_state.profile ||
      strcmp(drawn_identity,next_state.identity)) {
    invalidate_session();
    dirty = true;
  }
  current = next_state;
  if (tool_pending && !current.busy && current.result_revision != tool_revision &&
      current.connection_id == tool_connection) {
    bool ok = current.result_command ==
                  (tool_read ? tool.read_command : tool.command) &&
              fm1_action_decode(&tool, current.result, current.result_size,
                                tool_read, tool_values);
    tool_pending = false;
    if (tool_read)
      tool_loaded = ok;
    if (ok)
      snprintf(tool_result, sizeof(tool_result),
               tool_read ? "设备数据已读取" : "设备已回复 · %u 字节",
               current.result_size);
    else
      snprintf(tool_result, sizeof(tool_result),
               "回复拒绝或不匹配；未确认成功");
    fm1_action_summary(current.profile, current.result_command, current.result,
                       current.result_size, tool_detail, sizeof(tool_detail));
    if (ok && !strcmp(tool.kind, "mmc")) {
      snprintf(tool_result, sizeof(tool_result),
               "MMC 已发送；设备无状态读回。");
      snprintf(tool_detail, sizeof(tool_detail),
               "MMC 已通过 USB 发送；请在设备确认播放或录音状态。");
    }
    if (!ok) {
      int rc = tool_read ? tool.read_status : tool.status;
      snprintf(tool_result, sizeof(tool_result), "未确认全部完成 · 状态 %d",
               rc >= 0 && (unsigned)rc < current.result_size
                   ? current.result[rc]
                   : -1);
    }
    dirty = true;
  }
  if (tool_pending && !current.busy) {
    tool_pending = false;
    tool_loaded = false;
    snprintf(tool_result, sizeof(tool_result),
             "请求未完成；请检查连接后重新读取。");
    dirty = true;
  }
  if (!dragging && (drawn_page != current.page_revision ||
                    drawn_profile != current.profile || changed || dirty)) {
    render();
    return;
  }
  if (tab != 0 || !content)
    return;
  for (unsigned i = 0; i < current.count; i++) {
    row_t *r = rows + i;
    fm1_remote_parameter_t *p = current.rows + i;
    if (!r->label || r->dragging)
      continue;
    if (p->known)
      lv_label_set_text_fmt(r->label, "%s   %d %s", p->name, p->value, p->unit);
    else
      lv_label_set_text_fmt(r->label, "%s   --", p->name);
    if (r->slider) {
      if (p->known)
        lv_slider_set_value(r->slider, p->value, LV_ANIM_OFF);
      m1_enabled(r->slider, allowed() && p->known);
      m1_enabled(r->minus, allowed() && p->known);
      m1_enabled(r->plus, allowed() && p->known);
    }
    if (r->choice) {
      rebuilding = true;
      if (p->known && !lv_dropdown_is_open(r->choice))
        lv_dropdown_set_selected(r->choice, p->value - p->minimum);
      m1_enabled(r->choice, allowed() && p->known);
      rebuilding = false;
    }
  }
}
