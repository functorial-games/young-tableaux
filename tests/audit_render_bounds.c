/* Public RSK input -> console layout -> actual raster painter.
 * Compile with signed-overflow traps; run in a subprocess. */
#include "console.h"
#include "paint.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
int main(int argc,char **argv)
{
#ifndef RSK_PLOT_MAX
    (void)argc; (void)argv;
    puts("UNVERIFIED: this source predates the RSK companion plot");
    return 77;
#else
    if(argc!=3) return 2;
    Console *console=malloc(sizeof(*console));
    Controls *controls=malloc(sizeof(*controls));
    uint32_t *pixels=calloc(576U*1152U,sizeof(*pixels));
    if(!console || !controls || !pixels) return 2;
    console_init(console);controls_init(controls);
    Operation operation=!strcmp(argv[1],"word")?RSKWord:RSKBiword;
    int field=operation==RSKWord?SET_WORD:SET_BIWORD;
    snprintf(console->fields[field],sizeof(console->fields[field]),"%s",argv[2]);
    console_run(console,operation);
    if(!console->rsk_ok) return 3;
    console_layout(console,controls,576,1152);
    int found=0;
    for(int index=0;index<controls->count;++index) if(controls->controls[index].kind==RSK_PLOT) {
        controls->scroll=controls->controls[index].rect.y;found=1;break;
    }
    if(!found) return 4;
    Canvas canvas={576,1152,576,pixels,0,1152};
    paint_controls(&canvas,controls,false);
    raster_destroy();free(pixels);free(controls);free(console);
    puts("PASS renderer completed without signed-overflow trap");return 0;
#endif
}
