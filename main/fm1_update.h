#pragma once
#include <stdbool.h>
#include <stdint.h>
typedef enum { UPDATE_IDLE,UPDATE_PREPARING,UPDATE_READY,UPDATE_RUNNING,UPDATE_DONE,UPDATE_ERROR } fm1_update_stage_t;
typedef struct {
    fm1_update_stage_t stage;
    uint32_t revision,token,requests,bytes,source_connection;
    uint32_t elapsed_ms,check_ms,write_ms;
    unsigned progress,phase,step,steps;
    bool fast_transfer;
    char name[64],source[32],target[21],active_target[21],title[48],sha256[65],backup[64],message[120];
    bool custom,automatic;
} fm1_update_state_t;
void fm1_update_start(void);
void fm1_update_snapshot(fm1_update_state_t *out);
bool fm1_update_prepare(const char *name);
/* One user action: prepare and install, bound to the session at admission. */
bool fm1_update_install(const char *name);
bool fm1_update_confirm(uint32_t token);
bool fm1_update_cancel(uint32_t token);
bool fm1_update_dismiss(void);
unsigned fm1_update_stack_min(void);
