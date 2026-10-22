#pragma once
#include "lvgl.h"

/* Presentation only. Never use icon identity matching to authorize installs. */
lv_obj_t *m1_firmware_icon(lv_obj_t *parent,const char *identity,int size);
