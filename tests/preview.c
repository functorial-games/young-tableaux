/* Hosted projection QA, never physical-phone evidence. Writes PPM frames. */
#include "console.h"
#include "paint.h"
#include <stdio.h>
#include <stdlib.h>
static void frame(Console *console,Controls *ui,Canvas *canvas,const char *path)
{
    console_layout(console,ui,canvas->width,canvas->height);
    paint_controls(canvas,ui,console->french);
    if(ui->focus) {
        const char *labels[20]; for(int k=0;k<20;++k) labels[k]=console_key_label(k);
        paint_keyboard(canvas,ui,labels,-1);
    }
    FILE *out=fopen(path,"wb"); if(!out) exit(1);
    fprintf(out,"P6\n%d %d\n255\n",canvas->width,canvas->height);
    const unsigned *pixels=canvas->bits;
    for(int y=0;y<canvas->height;++y) for(int x=0;x<canvas->width;++x) {
        unsigned pixel=pixels[y*canvas->stride+x];
        unsigned char rgb[]={(unsigned char)pixel,(unsigned char)(pixel>>8),(unsigned char)(pixel>>16)};
        fwrite(rgb,1,3,out);
    }
    fclose(out);
}
int main(int argc,char **argv)
{
    if(argc!=2) return 2;
    Console *c=calloc(1,sizeof(*c)); Controls *u=calloc(1,sizeof(*u));
    Canvas canvas={576,1152,576,calloc(576*1152,sizeof(unsigned)),0,1152};
    if(!c || !u || !canvas.bits) return 1;
    console_init(c); controls_init(u); char path[1024];
    snprintf(path,sizeof(path),"%s/launch.ppm",argv[1]); frame(c,u,&canvas,path);
    u->focus=SET_LAMBDA; snprintf(path,sizeof(path),"%s/edit.ppm",argv[1]); frame(c,u,&canvas,path); u->focus=0;
    console_layout(c,u,576,1152);
    for(int i=0;i<u->count;++i) if(u->controls[i].id==OP_BASE+RSKPermutation) controls_scroll(u,u->controls[i].rect.y-60);
    snprintf(path,sizeof(path),"%s/rsk.ppm",argv[1]); frame(c,u,&canvas,path);
    console_run(c,LittlewoodRichardsonCoefficient); console_layout(c,u,576,1152);
    for(int i=0;i<u->count;++i) if(u->controls[i].id==OP_BASE+LittlewoodRichardsonCoefficient) controls_scroll(u,u->controls[i].rect.y-60);
    snprintf(path,sizeof(path),"%s/lr.ppm",argv[1]); frame(c,u,&canvas,path);
    raster_destroy(); free(canvas.bits); free(u); free(c); return 0;
}
