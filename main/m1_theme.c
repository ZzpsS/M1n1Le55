#include "m1_theme.h"

lv_obj_t *m1_label(lv_obj_t *parent,const char *text,int x,int y,int width,const lv_font_t *font,uint32_t color)
{
    lv_obj_t *o=lv_label_create(parent);lv_label_set_text(o,text);lv_obj_set_pos(o,x,y);
    if(width>0)lv_obj_set_width(o,width);
    lv_obj_set_style_text_font(o,font?font:&m1_font_24,0);
    lv_obj_set_style_text_color(o,lv_color_hex(color),0);
    return o;
}
lv_obj_t *m1_panel(lv_obj_t *parent,int x,int y,int w,int h,bool scroll)
{
    lv_obj_t *o=lv_obj_create(parent);lv_obj_remove_style_all(o);
    lv_obj_set_pos(o,x,y);lv_obj_set_size(o,w,h);lv_obj_set_style_bg_opa(o,LV_OPA_TRANSP,0);
    if(scroll){lv_obj_set_scroll_dir(o,LV_DIR_VER);lv_obj_set_scrollbar_mode(o,LV_SCROLLBAR_MODE_AUTO);lv_obj_set_style_bg_color(o,lv_color_hex(M1_MUTED),LV_PART_SCROLLBAR);lv_obj_set_style_bg_opa(o,LV_OPA_60,LV_PART_SCROLLBAR);lv_obj_set_style_width(o,4,LV_PART_SCROLLBAR);}
    else lv_obj_set_scrollable(o,false);
    return o;
}
lv_obj_t *m1_button(lv_obj_t *parent,const char *text,int x,int y,int w,int h,lv_event_cb_t cb,uintptr_t data,bool primary)
{
    lv_obj_t *o=lv_button_create(parent);lv_obj_remove_style_all(o);
    lv_obj_set_pos(o,x,y);lv_obj_set_size(o,w,h);lv_obj_set_style_radius(o,2,0);
    lv_obj_set_style_bg_opa(o,LV_OPA_COVER,0);lv_obj_set_style_bg_color(o,lv_color_hex(primary?M1_FG:M1_BG),0);
    lv_obj_set_style_border_width(o,1,0);lv_obj_set_style_border_color(o,lv_color_hex(primary?M1_FG:M1_LINE),0);
    lv_obj_set_style_bg_color(o,lv_color_hex(primary?M1_MUTED:M1_SURFACE),LV_STATE_PRESSED);
    lv_obj_set_style_outline_width(o,2,LV_STATE_FOCUSED);lv_obj_set_style_outline_color(o,lv_color_hex(M1_FG),LV_STATE_FOCUSED);
    lv_obj_set_style_opa(o,LV_OPA_40,LV_STATE_DISABLED);
    lv_obj_t *l=m1_label(o,text,0,0,w-16,&m1_font_24,primary?M1_BG:M1_FG);
    lv_label_set_long_mode(l,LV_LABEL_LONG_DOT);lv_obj_set_style_text_align(l,LV_TEXT_ALIGN_CENTER,0);lv_obj_center(l);
    if(cb)lv_obj_add_event_cb(o,cb,LV_EVENT_CLICKED,(void *)data);
    return o;
}
void m1_selected(lv_obj_t *o,bool selected)
{
    lv_obj_set_style_bg_color(o,lv_color_hex(selected?M1_SURFACE:M1_BG),0);
    lv_obj_set_style_border_color(o,lv_color_hex(selected?M1_FG:M1_LINE),0);
}
void m1_enabled(lv_obj_t *o,bool enabled)
{if(enabled)lv_obj_remove_state(o,LV_STATE_DISABLED);else lv_obj_add_state(o,LV_STATE_DISABLED);}
void m1_rule(lv_obj_t *parent,int x,int y,int w)
{
    lv_obj_t *o=m1_panel(parent,x,y,w,1,false);lv_obj_set_style_bg_opa(o,LV_OPA_COVER,0);
    lv_obj_set_style_bg_color(o,lv_color_hex(M1_LINE),0);
}
