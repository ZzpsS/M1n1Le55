#pragma once
#include "lvgl.h"
#define M1_BG 0x000000
#define M1_SURFACE 0x1A1A1E
#define M1_LINE 0x36363C
#define M1_FG 0xF2F2F2
#define M1_MUTED 0xC4C4CC
#define M1_DIM 0x76767E
#define M1_BLUE 0x287CFF
#define M1_GREEN 0x1ECC70
#define M1_YELLOW 0xFFC618
#define M1_ORANGE 0xFF621A
/* Content is authored on a 1048-unit grid and fills the 1144 px work area. */
#define M1_CONTENT_X 112
#define M1_CONTENT_W 1144
static inline int m1_wide(int x){return (x*M1_CONTENT_W+524)/1048;}
LV_FONT_DECLARE(m1_font_20);
LV_FONT_DECLARE(m1_font_24);
LV_FONT_DECLARE(m1_font_28);
LV_FONT_DECLARE(m1_pixel_32);
LV_FONT_DECLARE(m1_pixel_48);
lv_obj_t *m1_label(lv_obj_t *parent,const char *text,int x,int y,int width,const lv_font_t *font,uint32_t color);
lv_obj_t *m1_button(lv_obj_t *parent,const char *text,int x,int y,int w,int h,lv_event_cb_t cb,uintptr_t data,bool primary);
lv_obj_t *m1_panel(lv_obj_t *parent,int x,int y,int w,int h,bool scroll);
void m1_selected(lv_obj_t *button,bool selected);
void m1_rule(lv_obj_t *parent,int x,int y,int w);
void m1_enabled(lv_obj_t *obj,bool enabled);
