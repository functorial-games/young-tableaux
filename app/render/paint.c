#include "paint.h"
#include <stdio.h>
#include <string.h>
#define BG 0xff211b17U
#define FG 0xffeeeae3U
#define BLUE 0xffc7ac76U
static void wrapped(Canvas *b,const char *text,int x,int y,int scale,int columns,uint32_t color)
{
    int col=0;
    for(const char *p=text;*p;++p) {
        if(*p=='\n') { y+=10*scale; col=0; continue; }
        if(col==columns) { y+=10*scale; col=0; }
        char one[]={*p,0};
        if(y+7*scale>b->clip_top && y<b->clip_bottom) raster_text(b,one,x+col*6*scale,y,scale,color);
        ++col;
    }
}
void paint_controls(Canvas *b,const Controls *u,bool reverse)
{
    b->clip_top=0; b->clip_bottom=b->height;
    raster_rect(b,0,0,b->width,b->height,BG);
    b->clip_bottom=u->height;
    int scale=u->scale, columns=(u->width-16*scale)/(6*scale); if(columns<1) columns=1;
    for(int i=0;i<u->count;++i) {
        const Control *c=&u->controls[i]; Rect r=c->rect; r.y-=u->scroll;
        if(r.y+r.h<=0 || r.y>=u->height) continue;
        if(c->kind==SEPARATOR) { raster_rect(b,r.x,r.y,r.w,r.h,BLUE); continue; }
        if(c->kind==FIELD || c->kind==BUTTON || c->kind==CHOICE) {
            uint32_t color=c->kind==FIELD?0xff3b332bU:0xff584330U;
            if(u->pressed==c->id || u->focus==c->id) color=0xff775d3eU;
            raster_rect(b,r.x,r.y,r.w,r.h,color);
        }
        if(c->kind==DIAGRAM) {
            const TileProjection *p=c->projection;
            int max=1; for(int row=0;row<p->count;++row) if(p->rows[row]>max) max=p->rows[row];
            int cell=12*scale, available=r.w-8*scale;
            if(cell*max>available) cell=available/max;
            if(cell<1) cell=1;
            for(int row=0;row<p->count;++row) for(int col=0;col<p->rows[row];++col) {
                int y=r.y+4*scale+(reverse?p->count-1-row:row)*12*scale;
                int x=r.x+4*scale+col*cell;
                raster_rect(b,x,y,cell-1,10*scale,BLUE);
                if(p->numbers) {
                    char number[16]; snprintf(number,sizeof(number),"%d",p->values[row][col]);
                    int small=scale>1?scale-1:1;
                    if(raster_text_width(number,small)<cell) raster_text(b,number,x+1,y+scale,small,BG);
                }
            }
        } else wrapped(b,c->text,8*scale,r.y+4*scale,scale,columns,c->kind==OUTPUT?BLUE:FG);
    }
    if(u->content>u->height) {
        int track=u->height, thumb=track*u->height/u->content; if(thumb<8*scale) thumb=8*scale;
        int top=(track-thumb)*u->scroll/(u->content-u->height);
        raster_rect(b,b->width-2*scale,top,2*scale,thumb,BLUE);
    }
    b->clip_bottom=b->height;
}
void paint_keyboard(Canvas *b,const Controls *u,const char *const *labels,int pressed)
{
    int scale=u->scale,top=b->height-90*scale;
    raster_rect(b,0,top,b->width,90*scale,0xff352b24U);
    for(int k=0;k<20;++k) {
        int left=(k%5)*b->width/5,right=(k%5+1)*b->width/5;
        int y=top+5*scale+(k/5)*20*scale;
        raster_rect(b,left+scale,y,right-left-2*scale,18*scale,k==pressed?0xff775d3eU:0xff584330U);
        int text_scale=scale,fit=(right-left-2*scale)/(int)(6*strlen(labels[k]));
        if(text_scale>fit) text_scale=fit;
        if(text_scale<1) text_scale=1;
        raster_text(b,labels[k],left+(right-left-raster_text_width(labels[k],text_scale))/2,y+5*scale,text_scale,FG);
    }
}
