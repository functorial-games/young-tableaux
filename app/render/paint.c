#include "paint.h"
#include <stdio.h>
#include <string.h>
#include <math.h>
#define BG 0xff211b17U
#define FG 0xffeeeae3U
#define BLUE 0xffc7ac76U
#define WEGERT_TAU 6.28318530717958647692f
#define WEGERT_LOG_10 2.30258509299404568402f
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
static float clamp01(float value)
{
    if(value<0.0f) return 0.0f;
    if(value>1.0f) return 1.0f;
    return value;
}
static float positive_fract(float value) { return value-floorf(value); }
static float srgb_component(float linear)
{
    float value=fmaxf(linear,0.0f);
    if(value<=0.0031308f) return 12.92f*value;
    return 1.055f*powf(value,1.0f/2.4f)-0.055f;
}
static uint32_t wegert_color(float phase,float log_modulus)
{
    float hue_degrees=360.0f*positive_fract(phase/WEGERT_TAU);
    float band=positive_fract(log_modulus/WEGERT_LOG_10);
    float lightness=66.0f+4.0f*band+3.0f*positive_fract(hue_degrees/100.0f);
    float hue=hue_degrees*(WEGERT_TAU/360.0f);
    float u_star=45.0f*cosf(hue),v_star=45.0f*sinf(hue);
    const float white_u=0.19783982482140777f,white_v=0.46833630293240974f;
    float y=lightness>8.0f?powf((lightness+16.0f)/116.0f,3.0f):lightness/903.2962962962963f;
    float u_prime=u_star/(13.0f*lightness)+white_u;
    float v_prime=v_star/(13.0f*lightness)+white_v;
    float x=(9.0f*y*u_prime)/(4.0f*v_prime);
    float z=y*(12.0f-3.0f*u_prime-20.0f*v_prime)/(4.0f*v_prime);
    float red=clamp01(srgb_component(3.2404542f*x-1.5371385f*y-0.4985314f*z));
    float green=clamp01(srgb_component(-0.9692660f*x+1.8760108f*y+0.0415560f*z));
    float blue=clamp01(srgb_component(0.0556434f*x-0.2040259f*y+1.0572252f*z));
    uint32_t r=(uint32_t)lrintf(red*255.0f),g=(uint32_t)lrintf(green*255.0f),bl=(uint32_t)lrintf(blue*255.0f);
    return 0xff000000U|(bl<<16)|(g<<8)|r;
}
static void paint_wegert(Canvas *b,Rect r,const WegertProjection *projection)
{
    if(!projection || !projection->valid || r.w<2 || r.h<2) return;
    int local_top=b->clip_top>r.y?b->clip_top-r.y:0;
    int local_bottom=b->clip_bottom<r.y+r.h?b->clip_bottom-r.y:r.h;
    if(local_top<0) local_top=0;
    if(local_bottom>r.h) local_bottom=r.h;
    if(local_top>=local_bottom) return;
    const int sample=2;
    int start=(local_top/sample)*sample;
    float half_height=1.5f,aspect=(float)r.w/(float)r.h;
    for(int py=start;py<local_bottom;py+=sample) {
        float yi=half_height*(1.0f-2.0f*(float)py/(float)(r.h-1));
        for(int px=0;px<r.w;px+=sample) {
            float zr=half_height*aspect*(2.0f*(float)px/(float)(r.w-1)-1.0f);
            float zi=yi;
            float radius=fmaxf(hypotf(zr,zi),1.0e-12f);
            float phase=(float)projection->n_lambda*atan2f(zi,zr);
            float log_modulus=(float)projection->n_lambda*logf(radius);
            float pr=zr,pi=zi;
            for(int hook=1;hook<=projection->max_hook;++hook) {
                uint16_t multiplicity=projection->hook_counts[hook];
                if(multiplicity) {
                    float dr=1.0f-pr,di=-pi;
                    float delta=fmaxf(hypotf(dr,di),1.0e-12f);
                    phase-=(float)multiplicity*atan2f(di,dr);
                    log_modulus-=(float)multiplicity*logf(delta);
                }
                float next_r=pr*zr-pi*zi;
                pi=pr*zi+pi*zr;
                pr=next_r;
            }
            int width=px+sample<=r.w?sample:r.w-px;
            int height=py+sample<=r.h?sample:r.h-py;
            raster_rect(b,r.x+px,r.y+py,width,height,wegert_color(phase,log_modulus));
        }
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
        } else if(c->kind==WEGERT) {
            paint_wegert(b,r,(const WegertProjection *)c->projection);
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
