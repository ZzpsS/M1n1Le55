#pragma once
#include <stdbool.h>
#include "lvgl.h"
void fm1_ui_start(void);
bool fm1_ui_page(unsigned view,unsigned op);
bool fm1_ui_edit(unsigned parameter,unsigned value);
unsigned fm1_ui_view(void);
unsigned fm1_ui_operator(void);
/* Legacy views 0..4 = stock editor, 5 = device, 6 = console, 7 = samples.
 * New views: 8 = firmware library, 9 = sound/sample hub.
 * Main routes: 0 = firmware, 1 = console, 2 = sound/samples, 3 = device. */
void fm1_ui_route(unsigned route);
bool fm1_ui_extension_view(unsigned page);
bool fm1_ui_object_view(unsigned object, unsigned section, unsigned track, unsigned step);
bool fm1_ui_action_view(unsigned index);
void fm1_ui_notice(const char *message);
typedef enum { FM1_MANAGER_DEVICE, FM1_MANAGER_FIRMWARE, FM1_MANAGER_SAMPLES } fm1_manager_mode_t;
void fm1_manager_open(lv_obj_t *parent,fm1_manager_mode_t mode);
void fm1_manager_close(void);
void fm1_manager_tick(void);
bool fm1_manager_section(unsigned section);
bool fm1_manager_select(unsigned index);
