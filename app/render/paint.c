#include "paint.h"
#include <stdio.h>
#include <string.h>
#include <math.h>
#define BG 0xff211b17U
#define FG 0xffeeeae3U
#define BLUE 0xffc7ac76U
#define WEGERT_TAU 6.28318530717958647692f
#define WEGERT_LOG_10 2.30258509299404568402f
static size_t wrapped_token_bytes(const char *text)
{
    size_t prefix=0U;
    const unsigned char *glyph=(const unsigned char *)text;
    if((text[0]=='_' || text[0]=='^') && text[1] && text[1]!='\n') {
        prefix=1U; glyph=(const unsigned char *)(text+1);
    }
    if(glyph[0]<0x80U) return prefix+1U;
    if((glyph[0]&0xe0U)==0xc0U && glyph[1]) return prefix+2U;
    if((glyph[0]&0xf0U)==0xe0U && glyph[1] && glyph[2]) return prefix+3U;
    if((glyph[0]&0xf8U)==0xf0U && glyph[1] && glyph[2] && glyph[3]) return prefix+4U;
    return prefix+1U;
}

static void wrapped(Canvas *b,const char *text,int x,int y,int scale,int columns,uint32_t color)
{
    char line[1024];
    size_t used=0U;
    int visual=0;
    const char *p=text;

    while(*p) {
        if(*p=='\n') {
            line[used]=0;
            if(used && y+7*scale>b->clip_top && y<b->clip_bottom)
                raster_text(b,line,x,y,scale,color);
            used=0U; visual=0; y+=10*scale; ++p;
            continue;
        }

        size_t bytes=wrapped_token_bytes(p);
        if(visual>=columns || used+bytes>=sizeof(line)-1U) {
            line[used]=0;
            if(used && y+7*scale>b->clip_top && y<b->clip_bottom)
                raster_text(b,line,x,y,scale,color);
            used=0U; visual=0; y+=10*scale;
        }

        memcpy(line+used,p,bytes);
        used+=bytes; ++visual; p+=bytes;
    }

    line[used]=0;
    if(used && y+7*scale>b->clip_top && y<b->clip_bottom)
        raster_text(b,line,x,y,scale,color);
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
bool wegert_phase_log(const WegertProjection *projection,double real,double imag,
                      double *phase,double *log_modulus)
{
    if(!projection || !projection->valid || !isfinite(real) || !isfinite(imag)) return false;
    double radius=hypot(real,imag), angle=atan2(imag,real);
    if(!isfinite(radius) || (radius==0.0 && projection->n_lambda)) return false;
    double log_radius=log(fmax(radius,1.0e-300));
    *phase=projection->n_lambda*angle;
    *log_modulus=projection->n_lambda*log_radius;
    bool outside=radius>1.0;
    double step_real=outside?(real/radius)/radius:real;
    double step_imag=outside?-(imag/radius)/radius:imag;
    double power_real=1.0,power_imag=0.0;
    for(int hook=1;hook<=projection->max_hook;++hook) {
        double next_real=power_real*step_real-power_imag*step_imag;
        power_imag=power_real*step_imag+power_imag*step_real;
        power_real=next_real;
        int count=projection->hook_counts[hook];
        if(!count) continue;
        double delta_real=1.0-power_real,delta_imag=-power_imag;
        double delta=hypot(delta_real,delta_imag);
        if(delta==0.0) return false;
        double factor_phase=atan2(delta_imag,delta_real),factor_log=log(delta);
        /* 1-z^h = -z^h (1-z^(-h)) outside the unit circle. */
        if(outside) { factor_phase+=3.14159265358979323846+hook*angle; factor_log+=hook*log_radius; }
        *phase-=count*factor_phase; *log_modulus-=count*factor_log;
    }
    return isfinite(*phase) && isfinite(*log_modulus);
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
    double half_height=projection->half_height,aspect=(double)r.w/r.h;
    for(int py=start;py<local_bottom;py+=sample) {
        double imag=projection->center_imag+half_height*(1.0-2.0*py/(r.h-1));
        for(int px=0;px<r.w;px+=sample) {
            double real=projection->center_real+half_height*aspect*(2.0*px/(r.w-1)-1.0);
            double phase,log_modulus;
            uint32_t color=BG;
            if(wegert_phase_log(projection,real,imag,&phase,&log_modulus))
                color=wegert_color((float)remainder(phase,6.28318530717958647692),
                                   (float)remainder(log_modulus,2.30258509299404568402));
            else if(real==0.0 && imag==0.0 && projection->n_lambda) color=FG;
            int width=px+sample<=r.w?sample:r.w-px;
            int height=py+sample<=r.h?sample:r.h-py;
            raster_rect(b,r.x+px,r.y+py,width,height,color);
        }
    }
}
static void paint_rsk_plot(Canvas *b,Rect r,const RSKPlotProjection *projection,int scale)
{
    if(!projection || projection->count<=0 || r.w<12*scale || r.h<12*scale) return;
    int minimum=projection->values[0],maximum=projection->values[0];
    for(int i=1;i<projection->count;++i) {
        if(projection->values[i]<minimum) minimum=projection->values[i];
        if(projection->values[i]>maximum) maximum=projection->values[i];
    }
    if(minimum==maximum) { --minimum; ++maximum; }
    int left=r.x+10*scale,right=r.x+r.w-6*scale;
    int top=r.y+5*scale,bottom=r.y+r.h-12*scale;
    if(right<=left || bottom<=top) return;
    raster_rect(b,left,bottom,right-left+1,scale,0xff584330U);
    raster_rect(b,left,top,scale,bottom-top+1,0xff584330U);
    int width=right-left,height=bottom-top;
    int point=scale>1?3*scale:3;
    for(int i=0;i<projection->count;++i) {
        int x=projection->count==1?left+width/2:left+i*width/(projection->count-1);
        long long numerator=(long long)(projection->values[i]-minimum)*height;
        int y=bottom-(int)(numerator/(maximum-minimum));
        uint32_t color=0xff584330U;
        if(i+1<projection->step) color=BLUE;
        else if(i+1==projection->step) color=FG;
        raster_rect(b,x-point/2,y-point/2,point,point,color);
    }
    char label[64];
    snprintf(label,sizeof(label),"insertions 1..%d",projection->count);
    raster_text(b,label,left,bottom+3*scale,scale,FG);
    snprintf(label,sizeof(label),"%d",maximum);
    raster_text(b,label,r.x+2*scale,top,scale,FG);
    snprintf(label,sizeof(label),"%d",minimum);
    raster_text(b,label,r.x+2*scale,bottom-7*scale,scale,FG);
}
void paint_controls(Canvas *b,const Controls *u,bool reverse)
{
    b->clip_top=0; b->clip_bottom=b->height;
    raster_rect(b,0,0,b->width,b->height,BG);
    b->clip_bottom=u->height;
    int scale=u->scale;
    for(int i=0;i<u->count;++i) {
        const Control *c=&u->controls[i]; Rect r=c->rect; r.y-=u->scroll;
        if(r.y+r.h<=0 || r.y>=u->height) continue;
        if(c->kind==SEPARATOR) { raster_rect(b,r.x,r.y,r.w,r.h,BLUE); continue; }
        if(c->kind==FIELD || c->kind==BUTTON || c->kind==CHOICE || c->kind==DISABLED_BUTTON) {
            uint32_t color=c->kind==FIELD?0xff3b332bU:0xff584330U;
            if(c->kind==DISABLED_BUTTON) color=0xff2c2824U;
            else if(u->pressed==c->id || u->focus==c->id) color=0xff775d3eU;
            raster_rect(b,r.x,r.y,r.w,r.h,color);
        }
        if(c->kind==HERO) {
            int size=3*scale;
            while(size>scale && raster_text_width(c->text,size)>r.w-8*scale) --size;
            raster_text(b,c->text,r.x+(r.w-raster_text_width(c->text,size))/2,
                        r.y+(r.h-7*size)/2,size,FG);
            continue;
        }
        if(c->kind==BUTTON || c->kind==DISABLED_BUTTON) {
            int size=scale, length=(int)strlen(c->text);
            while(size>1 && length*6*size>r.w-4*scale) --size;
            raster_text(b,c->text,r.x+(r.w-raster_text_width(c->text,size))/2,
                        r.y+(r.h-7*size)/2,size,c->kind==DISABLED_BUTTON?0xff80786dU:FG);
            continue;
        }
        if(c->kind==ROW_BLOCKS) {
            const TileRowProjection *row=c->projection;
            int count=row->count, cell=r.h, available=r.w-4*scale;
            if(row->columns*cell>available) cell=available/row->columns;
            if(cell<1) cell=1;
            for(int column=0;column<count;++column)
                raster_rect(b,r.x+2*scale+column*cell,r.y+(r.h-cell)/2,
                            cell>1?cell-1:1,cell>1?cell-1:1,BLUE);
        } else if(c->kind==DIAGRAM) {
            const TileProjection *p=c->projection;
            int max=1;
            for(int row=0;row<p->count;++row)
                if(p->starts[row]+p->rows[row]>max) max=p->starts[row]+p->rows[row];
            int cell=12*scale, available=r.w-8*scale;
            if(cell*max>available) cell=available/max;
            if(cell<1) cell=1;
            for(int row=0;row<p->count;++row) for(int col=0;col<p->rows[row];++col) {
                int y=r.y+4*scale+(reverse?p->count-1-row:row)*12*scale;
                int x=r.x+4*scale+(p->starts[row]+col)*cell;
                uint32_t tile=p->marks[row][col]?FG:BLUE;
                raster_rect(b,x,y,cell-1,10*scale,tile);
                if(p->numbers && p->values[row][col]) {
                    char number[16]; snprintf(number,sizeof(number),"%d",p->values[row][col]);
                    int small=scale>1?scale-1:1;
                    if(raster_text_width(number,small)<cell) raster_text(b,number,x+1,y+scale,small,BG);
                }
            }
        } else if(c->kind==WEGERT) {
            paint_wegert(b,r,(const WegertProjection *)c->projection);
        } else if(c->kind==RSK_PLOT) {
            paint_rsk_plot(b,r,(const RSKPlotProjection *)c->projection,scale);
        } else {
            int columns=(r.w-8*scale)/(6*scale); if(columns<1) columns=1;
            wrapped(b,c->text,r.x+4*scale,r.y+4*scale,scale,columns,c->kind==OUTPUT?BLUE:FG);
        }
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
    int scale=u->scale,top=b->height-90*u->scale;
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
