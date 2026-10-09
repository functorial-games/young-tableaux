#ifndef YOUNG_CONSOLE_H
#define YOUNG_CONSOLE_H
#include "young_math.h"
#include "advanced.h"
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
 SET_SKEW_TABLEAU,SET_ENTRY,SET_RECORDING,SET_SEED,SET_INNER_FUNCTION,FIELD_COUNT, CHOOSE_ORIENTATION=100,CHOOSE_STANDARD,CHOOSE_INSERTION,
 CHOOSE_CONTENT,STEP_MINUS,STEP_PLUS,CHOOSE_TABLEAU_KIND,
 RSK_START=500,RSK_PREV,RSK_NEXT,RSK_END,
 JDT_RESET=520,JDT_STEP,JDT_SLIDE,JDT_RECTIFY,OP_BASE=1000 };
#define SCRIPT_LAYOUT_MAX 32
typedef enum {
    SCRIPT_LABEL, SCRIPT_SEPARATOR, SCRIPT_FIELD, SCRIPT_SHAPE, SCRIPT_TABLEAU,
    SCRIPT_OUTPUT, SCRIPT_HOOKS, SCRIPT_FACTS, SCRIPT_WEGERT,
    SCRIPT_PLOT_CONTROLS
} ScriptLayoutKind;
typedef struct { ScriptLayoutKind kind; int arg; char text[128]; } ScriptLayoutItem;
typedef enum { RSK_PERMUTATION_INPUT, RSK_WORD_INPUT, RSK_BIWORD_INPUT, RSK_MATRIX_INPUT } RSKInputKind;
typedef struct {
    char fields[FIELD_COUNT][512];
    char output[12][UI_TEXT];
    char scripted_facts[UI_TEXT];
    TileProjection partition,conjugate,tableau,p,q,hooks,jeu_tiles,rsk_before_p,jeu_before_tiles;
    TileRowProjection editable_rows[YT_DIM];
    WegertProjection wegert, jeu_wegert;
    RSKPlotProjection rsk_plot;
    JeuState jeu;
    ScriptLayoutItem script_layout[SCRIPT_LAYOUT_MAX];
    int script_layout_count;
    DiagramState diagram;
    bool partition_ok,tableau_ok,rsk_ok,jeu_loaded,jeu_ok,rsk_has_before,jeu_has_before,french,decreasing;
    RSKInputKind rsk_input_kind;
    InsertionConvention insertion;
    ContentConvention content_convention;
    TableauKind tableau_kind;
    int rsk_step,rsk_total;
    YoungRNG rng;
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
