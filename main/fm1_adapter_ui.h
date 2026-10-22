#pragma once
#include "lvgl.h"
void fm1_adapter_ui_open(lv_obj_t *parent);
void fm1_adapter_ui_close(void);
void fm1_adapter_ui_tick(void);
bool fm1_adapter_ui_dragging(void);
bool fm1_adapter_ui_page(unsigned page);
bool fm1_adapter_ui_object(unsigned object, unsigned section, unsigned track, unsigned step);
bool fm1_adapter_ui_action(unsigned index);
