#include "controls.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
void controls_init(Controls *u) { memset(u,0,sizeof(*u)); u->pointer = -1; }
void controls_begin(Controls *u,int w,int h,bool keyboard)
{
    u->count=0; u->width=w; u->scale=w/190; if(u->scale<1) u->scale=1;
    if(u->scale>6) u->scale=6;
    u->height=h-(keyboard?90*u->scale:0); if(u->height<1) u->height=1;
    u->content=8*u->scale;
}
int controls_text_height(const Controls *u,const char *text)
{
    int columns=(u->width-16*u->scale)/(6*u->scale); if(columns<1) columns=1;
    int lines=1,column=0;
    for(const char *p=text;*p;++p) {
        if(*p=='\n') { ++lines; column=0; }
        else { if(column==columns) { ++lines; column=0; } ++column; }
    }
    return (lines*10+8)*u->scale;
}
void controls_add(Controls *u,int id,ControlKind kind,const char *text,int h,const void *projection)
{
    if(u->count==UI_MAX) abort();
    Control *c=&u->controls[u->count++];
    c->id=id; c->kind=kind; c->projection=projection;
    snprintf(c->text,sizeof(c->text),"%s",text);
    if(!h) h=controls_text_height(u,text);
    c->rect=(Rect){4*u->scale,u->content,u->width-8*u->scale,h};
    u->content+=h+4*u->scale;
}
void controls_scroll(Controls *u,int offset)
{
    int maximum=u->content-u->height; if(maximum<0) maximum=0;
    if(offset<0) offset=0;
    if(offset>maximum) offset=maximum;
    u->scroll=offset;
}
void controls_end(Controls *u) { controls_scroll(u,u->scroll); }
int controls_hit(const Controls *u,int x,int y)
{
    if(y<0 || y>=u->height) return 0;
    y+=u->scroll;
    for(int i=0;i<u->count;++i) {
        const Control *c=&u->controls[i]; const Rect *r=&c->rect;
        if(c->kind!=FIELD && c->kind!=BUTTON && c->kind!=CHOICE) continue;
        if(x>=r->x && x<r->x+r->w && y>=r->y && y<r->y+r->h) return c->id;
    }
    return 0;
}
ControlEvent controls_touch(Controls *u,Touch t,int pointer,int x,int y)
{
    ControlEvent e={EVENT_NONE,0};
    if(t==TOUCH_CANCEL) { u->pointer=-1; u->pressed=0; u->dragging=false; return e; }
    if(t==TOUCH_DOWN) {
        if(u->pointer!=-1) return e;
        u->pointer=pointer; u->start_x=x; u->start_y=y; u->last_y=y;
        u->pressed=controls_hit(u,x,y); u->dragging=false; return e;
    }
    if(u->pointer!=pointer) return e;
    if(abs(y-u->start_y)>4*u->scale || abs(x-u->start_x)>4*u->scale) u->dragging=true;
    if(t==TOUCH_MOVE) { if(u->dragging) controls_scroll(u,u->scroll+u->last_y-y); u->last_y=y; }
    if(t==TOUCH_UP) {
        int hit=controls_hit(u,x,y);
        if(!u->dragging && hit && hit==u->pressed) {
            e=(ControlEvent){EVENT_ACTIVATE,hit};
            for(int i=0;i<u->count;++i) if(u->controls[i].id==hit && u->controls[i].kind==FIELD) {
                u->focus=hit; e.kind=EVENT_FOCUS; break;
            }
        }
        u->pointer=-1; u->pressed=0;
    }
    return e;
}
