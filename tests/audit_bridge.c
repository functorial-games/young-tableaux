/* Observation only: exercise the same console/event/layout owners as Android. */
#include "console.h"
#include <stdlib.h>
#include <string.h>
static Console console;
static Controls controls;
void audit_reset(void) { console_init(&console); controls_init(&controls); }
int audit_set(int field,const char *text) {
    if(field<=0 || field>=FIELD_COUNT || strlen(text)>=512) return 0;
    strcpy(console.fields[field],text); return 1;
}
void audit_edit(int field,int key) { controls.focus=field; console_key(&console,&controls,key); }
void audit_run(int operation) { console_event(&console,&controls,(ControlEvent){EVENT_ACTIVATE,OP_BASE+operation}); }
void audit_event(int id) { console_event(&console,&controls,(ControlEvent){EVENT_ACTIVATE,id}); }
const char *audit_output(int section) { return console.output[section]; }
int audit_visible(int id) {
    console_layout(&console,&controls,576,1152);
    for(int index=0;index<controls.count;++index) if(controls.controls[index].id==id) return 1;
    return 0;
}
int audit_rendered(int section) {
    console_layout(&console,&controls,576,1152);
    for(int index=0;index<controls.count;++index)
        if(controls.controls[index].kind==OUTPUT && !strcmp(controls.controls[index].text,console.output[section])) return 1;
    return 0;
}
const TileProjection *audit_tiles(int which) {
    switch(which) {
    case 0:return &console.partition; case 1:return &console.tableau;
    case 2:return &console.p; case 3:return &console.q;
    case 4:return &console.rsk_before_p; case 5:return &console.jeu_tiles;
    case 6:return &console.jeu_before_tiles; default:return &console.hooks;
    }
}
const JeuState *audit_jeu(void) { return &console.jeu; }
int audit_rsk_step(void) { return console.rsk_step; }
void audit_decreasing(int enabled) { console.decreasing=enabled!=0; }
void audit_kind(int kind) { console.tableau_kind=(TableauKind)kind; }
