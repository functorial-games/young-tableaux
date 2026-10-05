#ifndef YOUNG_PAINT_H
#define YOUNG_PAINT_H
#include "raster.h"
#include "controls.h"
void paint_controls(Canvas *canvas,const Controls *ui,bool reverse_rows);
void paint_keyboard(Canvas *canvas,const Controls *ui,const char *const *labels,int pressed);
#endif
