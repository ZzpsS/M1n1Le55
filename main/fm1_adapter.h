#pragma once
#include "fm1_adapter_wire.h"
#define ADAPTER_ROWS 4
typedef struct { uint8_t scope,id; } fm1_parameter_ref_t;
typedef struct {
  bool described, known;
  uint8_t id, scope, index, format; /* index is MENU_DESC's index; id is MENU_SET's id. */
  int16_t value, minimum, maximum, initial;
  char name[40], unit[20], options[768];
} fm1_remote_parameter_t;
typedef struct {
  fm1_firmware_t profile;
  bool connected, busy, ready, visible, native_view;
  uint32_t connection_id, revision, page_revision;
  char identity[32], message[160], version[32];
  uint8_t engines, parameters, globals, steps, engine_base, tracks, protocol,
      menu_count;
  uint8_t selected, engine, preset, scope, first, count;
  char engine_names[768];
  fm1_remote_parameter_t rows[ADAPTER_ROWS];
  uint8_t result[1024];
  unsigned result_size, result_command;
  uint32_t result_revision;
  bool file_prepared;
  uint32_t file_token, file_connection, file_progress, file_total;
  unsigned file_object, file_size, file_count, file_page, file_pages;
  char file_name[64], file_backup[64], files[16][64];
} fm1_adapter_state_t;
void fm1_adapter_start(void);
void fm1_adapter_snapshot(fm1_adapter_state_t *out);
void fm1_adapter_visible(bool visible);
bool fm1_adapter_view(unsigned scope, unsigned first, unsigned track);
/* One copied request for the native page's ordered, possibly mixed-scope rows.
 * scope 0: track parameter; 1: global; 2: Felucca MENU descriptor index.
 * track 255 reads the selected track without changing it. */
bool fm1_adapter_native_view(const fm1_parameter_ref_t *refs,unsigned count,unsigned track);
bool fm1_adapter_set(unsigned row, int value, uint32_t page_revision);
bool fm1_adapter_midi(const uint8_t *message, unsigned size, unsigned gate_ms);
/* Enqueue one FoMni CH2 chord (2..8 unique notes, at least two pitch classes).
 * true means accepted, not confirmed playback. All releases stay on that USB
 * connection; gate_ms is 1..2000 and begins after the last Note On is sent. */
bool fm1_adapter_chord(const uint8_t *notes, unsigned count, unsigned gate_ms);
bool fm1_adapter_mmc(unsigned command);
/* Structured UI commands, filtered by the exact firmware's allowlist. */
bool fm1_adapter_command(unsigned command, const uint8_t *data, unsigned size);
bool fm1_adapter_open_command(unsigned command, const uint8_t *data,
                              unsigned size);
bool fm1_adapter_files_scan(void);
bool fm1_adapter_files_page(unsigned page);
bool fm1_adapter_file_read(unsigned object);
bool fm1_adapter_file_prepare(const char *name);
bool fm1_adapter_file_confirm(uint32_t token);
bool fm1_adapter_file_cancel(uint32_t token);

/* Bounded parsers shared by worker and native tests. */
bool fm1_adapter_parse_info(fm1_adapter_state_t *state, const uint8_t *data,
                            unsigned size);
bool fm1_adapter_parse_desc(fm1_remote_parameter_t *out, const uint8_t *data,
                            unsigned size, bool menu);
