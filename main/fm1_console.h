#pragma once
#include "lvgl.h"
#include <stdbool.h>
void fm1_console_open(lv_obj_t *parent);
void fm1_console_close(void);
void fm1_console_tick(void);
bool fm1_console_dragging(void);
bool fm1_console_page(unsigned page);
unsigned fm1_console_mode(void);
