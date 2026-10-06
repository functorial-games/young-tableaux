#ifndef YOUNG_RASTER_H
#define YOUNG_RASTER_H
#include <stdint.h>
typedef struct { int width,height,stride; void *bits; int clip_top,clip_bottom; } Canvas;
void raster_rect(Canvas *buffer,int left,int top,int width,int height,uint32_t value);
void raster_text(Canvas *buffer,const char *text,int left,int top,int scale,uint32_t value);
int raster_text_width(const char *text,int scale);
void raster_destroy(void);
#endif
