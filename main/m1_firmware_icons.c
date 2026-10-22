#include "m1_firmware_icons.h"
#include "m1_theme.h"
#include <string.h>

/* Publisher marks retain their original terms. See licenses/firmware-icons/NOTICES.md.
 * The remaining pixel symbols are original M1n1Le55 artwork, not publisher logos. */
#include "m1_firmware_brand_data.inc"

enum { ICON_UNKNOWN,ICON_STOCK,ICON_FELUCCA,ICON_SLOOP,ICON_VA,
       ICON_FOMNI,ICON_X0X,ICON_JANGADA,ICON_OPEN,ICON_PROBE,ICON_DIAG };
typedef struct {lv_layer_t *layer;int x,y,size;} paint_t;
static int scale(paint_t *p,int value){return (value*p->size+120)/240;}
static void rect(paint_t *p,int x,int y,int w,int h,uint32_t color,int radius)
{
    lv_draw_rect_dsc_t d;lv_draw_rect_dsc_init(&d);d.bg_color=lv_color_hex(color);d.bg_opa=LV_OPA_COVER;d.radius=scale(p,radius);
    lv_area_t a={p->x+scale(p,x),p->y+scale(p,y),p->x+scale(p,x+w)-1,p->y+scale(p,y+h)-1};lv_draw_rect(p->layer,&d,&a);
}
static void line(paint_t *p,int x1,int y1,int x2,int y2,int width,uint32_t color)
{
    lv_draw_line_dsc_t d;lv_draw_line_dsc_init(&d);d.color=lv_color_hex(color);d.width=LV_MAX(1,scale(p,width));
    d.round_start=d.round_end=1;d.p1=(lv_point_precise_t){p->x+scale(p,x1),p->y+scale(p,y1)};
    d.p2=(lv_point_precise_t){p->x+scale(p,x2),p->y+scale(p,y2)};lv_draw_line(p->layer,&d);
}
static void triangle(paint_t *p,int x1,int y1,int x2,int y2,int x3,int y3,uint32_t color)
{
    lv_draw_triangle_dsc_t d;lv_draw_triangle_dsc_init(&d);d.color=lv_color_hex(color);
    d.p[0]=(lv_point_precise_t){p->x+scale(p,x1),p->y+scale(p,y1)};
    d.p[1]=(lv_point_precise_t){p->x+scale(p,x2),p->y+scale(p,y2)};
    d.p[2]=(lv_point_precise_t){p->x+scale(p,x3),p->y+scale(p,y3)};lv_draw_triangle(p->layer,&d);
}
static void draw(lv_event_t *e)
{
    lv_obj_t *obj=lv_event_get_target_obj(e);lv_area_t a;lv_obj_get_coords(obj,&a);
    paint_t p={lv_event_get_layer(e),a.x1,a.y1,lv_obj_get_width(obj)};unsigned kind=(uintptr_t)lv_event_get_user_data(e);
    if(kind==ICON_SLOOP){
        /* Original SLOOP SVG geometry, uniformly scaled from its 240-unit viewBox. */
        rect(&p,0,0,240,240,0,0);
        lv_draw_arc_dsc_t d;lv_draw_arc_dsc_init(&d);d.color=lv_color_hex(0xF2F2F2);d.width=scale(&p,12);
        d.center=(lv_point_t){p.x+scale(&p,120),p.y+scale(&p,120)};d.radius=scale(&p,106);d.start_angle=135;d.end_angle=405;d.rounded=1;lv_draw_arc(p.layer,&d);
        rect(&p,85,36,10,136,0xF2F2F2,5);
        const int top[]={40,71,102,133},left_top[]={101,126,151,177},right_bottom[]={121,147,172,197};
        const uint32_t colors[]={M1_BLUE,M1_GREEN,M1_YELLOW,M1_ORANGE};
        for(unsigned i=0;i<4;i++){triangle(&p,101,top[i],left_top[i],top[i],right_bottom[i],top[i]+25,colors[i]);triangle(&p,101,top[i],right_bottom[i],top[i]+25,101,top[i]+25,colors[i]);}
        rect(&p,56,182,128,12,0xF2F2F2,6);return;
    }
    if(kind==ICON_VA){
        for(unsigned i=1;i<sizeof(baudgirl_points)/sizeof(*baudgirl_points);i++)
            line(&p,baudgirl_points[i-1][0],baudgirl_points[i-1][1],baudgirl_points[i][0],baudgirl_points[i][1],18,0xC22483);
        return;
    }
    if(kind==ICON_FOMNI){
        rect(&p,0,0,240,240,0xFAF4E8,8);
        for(int i=0;i<5;i++){rect(&p,36+i*36,46,12,114,0x26A69A,0);rect(&p,30+i*36,174,24,24,i&1?0xE85880:0xF27848,0);}return;
    }
    if(kind==ICON_FELUCCA){
        rect(&p,52,168,136,20,0xF2F2F2,0);rect(&p,72,188,96,16,0xFF9630,0);
        rect(&p,112,32,12,124,0xF2F2F2,0);
        for(int i=0;i<6;i++){rect(&p,128,48+i*18,18+i*8,14,0xFF9630,0);rect(&p,100-i*10,68+i*15,10+i*10,11,0xC4C4CC,0);}return;
    }
    if(kind==ICON_X0X){
        const uint32_t colors[]={0xFF9628,0xFF4840,0x82F05A,0x46CDFF};
        for(int row=0;row<4;row++)for(int col=0;col<4;col++)rect(&p,30+col*48,30+row*48,36,36,(row+col)%3?colors[row]:0x36363C,0);
        return;
    }
    if(kind==ICON_JANGADA){
        for(int i=0;i<4;i++){rect(&p,30,42+i*44,180,8,0x36363C,0);rect(&p,50+(i%3)*46,30+i*44,24,30,i&1?0x26D9BD:0xFF14AA,0);}return;
    }
    if(kind==ICON_OPEN){
        rect(&p,76,76,88,88,0x287CFF,0);rect(&p,98,98,44,44,0xF2F2F2,0);
        for(int i=0;i<3;i++){rect(&p,80+i*30,38,12,28,M1_BLUE,0);rect(&p,80+i*30,174,12,28,M1_BLUE,0);rect(&p,38,80+i*30,28,12,M1_GREEN,0);rect(&p,174,80+i*30,28,12,M1_GREEN,0);}return;
    }
    if(kind==ICON_PROBE){
        const uint32_t c=0xB58AFF;
        rect(&p,114,30,12,180,c,0);
        for(int i=0;i<5;i++){rect(&p,126+i*12,42+i*12,12,12,c,0);rect(&p,126+i*12,102-i*12,12,12,c,0);rect(&p,126+i*12,138+i*12,12,12,c,0);rect(&p,126+i*12,198-i*12,12,12,c,0);rect(&p,54+i*12,60+i*12,12,12,c,0);rect(&p,54+i*12,168-i*12,12,12,c,0);}return;
    }
    if(kind==ICON_DIAG){
        rect(&p,24,42,192,144,0x36363C,0);rect(&p,36,54,168,120,M1_BG,0);
        rect(&p,42,114,42,12,M1_YELLOW,0);rect(&p,84,78,12,48,M1_YELLOW,0);rect(&p,96,78,24,12,M1_YELLOW,0);
        rect(&p,120,78,12,72,M1_YELLOW,0);rect(&p,132,138,24,12,M1_YELLOW,0);rect(&p,156,114,12,36,M1_YELLOW,0);rect(&p,168,114,30,12,M1_YELLOW,0);return;
    }
    for(int row=0;row<2;row++)for(int col=0;col<2;col++)rect(&p,40+col*88,40+row*88,68,68,M1_DIM,0);
}
lv_obj_t *m1_firmware_icon(lv_obj_t *parent,const char *identity,int size)
{
    static const struct {const char *id;unsigned kind;} icons[]={
        {"FM-1_015",ICON_STOCK},{"FM-1_916",ICON_FELUCCA},{"FM-1_900",ICON_SLOOP},{"FM-1_101",ICON_VA},
        {"FM-1_8000400",ICON_FOMNI},{"FM-1_9010100",ICON_X0X},{"FM-1_910",ICON_JANGADA},
        {"FM-1_8000012",ICON_OPEN},{"FM-1_8000912",ICON_PROBE},{"FM-1_8000913",ICON_DIAG}};
    unsigned kind=ICON_UNKNOWN;for(unsigned i=0;i<sizeof(icons)/sizeof(*icons);i++)if(!strcmp(identity,icons[i].id)){kind=icons[i].kind;break;}
    lv_obj_t *icon=m1_panel(parent,0,0,size,size,false);lv_obj_set_clickable(icon,false);
    if(kind==ICON_STOCK){lv_obj_t *image=lv_image_create(icon);lv_image_set_src(image,&mvave_image);lv_image_set_scale(image,size*256/mvave_image.header.w);lv_obj_center(image);}
    else lv_obj_add_event_cb(icon,draw,LV_EVENT_DRAW_MAIN,(void *)(uintptr_t)kind);
    return icon;
}
