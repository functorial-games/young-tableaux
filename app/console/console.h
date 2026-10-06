#ifndef YOUNG_CONSOLE_H
#define YOUNG_CONSOLE_H
#include "math.h"
#include "controls.h"
typedef enum {
#define OP(symbol,section,label,input,output) symbol,
#include "operations.def"
#undef OP
    OP_COUNT
} Operation;
enum { SET_LAMBDA=1,SET_TABLEAU,SET_PERMUTATION,SET_MU,SET_NU,SET_CELL,
 SET_WORD,SET_BIWORD,SET_MATRIX,SET_N,SET_ALPHABET,SET_WEIGHT,SET_CHARACTERISTIC,
 SET_PRIME,SET_HECKE,SET_BASIS,SET_Q,SET_T,SET_VARIABLES,SET_COEFFICIENTS,
 SET_PATH,SET_LAW,SET_COXETER,SET_SECOND_PERMUTATION,SET_READING,SET_ACTION,
 FIELD_COUNT, CHOOSE_ORIENTATION=100,CHOOSE_STANDARD,CHOOSE_INSERTION,
 CHOOSE_CONTENT,STEP_MINUS,STEP_PLUS,CHOOSE_TABLEAU_KIND,OP_BASE=1000 };
typedef struct {
    char fields[FIELD_COUNT][512];
    char output[12][UI_TEXT];
    char scripted_facts[UI_TEXT];
    TileProjection partition,conjugate,tableau,p,q,hooks;
    WegertProjection wegert;
    bool partition_ok,tableau_ok,rsk_ok,french,decreasing;
    int insertion,content_convention,tableau_kind;
} Console;
typedef struct { const char *label,*input,*output; int section; } OperationInfo;
extern const OperationInfo operation_info[OP_COUNT];
void console_init(Console *c);
void console_layout(Console *c,Controls *ui,int width,int height);
void console_event(Console *c,Controls *ui,ControlEvent event);
void console_key(Console *c,Controls *ui,int key);
void console_run(Console *c,Operation operation);
int console_key_hit(const Controls *ui,int x,int y,int screen_height);
const char *console_key_label(int key);
#endif
