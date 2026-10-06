#ifndef YOUNG_CONTROLS_H
#define YOUNG_CONTROLS_H
#include <stdbool.h>
#include <stdint.h>
#define UI_MAX 320
#define UI_TEXT 16384
#define WEGERT_HOOK_MAX 127
typedef enum { LABEL, SEPARATOR, FIELD, BUTTON, CHOICE, OUTPUT, DIAGRAM, WEGERT } ControlKind;
typedef struct { int x,y,w,h; } Rect;
/* Output projection: generic ragged rows of tiles, optional integer labels. */
typedef struct { int count,rows[64],values[64][64]; bool numbers; } TileProjection;
/* Principal-specialization projection. Hook multiplicities determine the
 * denominator factors (1-z^h); n_lambda determines the numerator z^n. */
typedef struct {
    bool valid;
    int n_lambda;
    int max_hook;
    uint16_t hook_counts[WEGERT_HOOK_MAX+1];
} WegertProjection;
/* Identity is semantic, never inferred from array index or rectangle. */
typedef struct { int id; ControlKind kind; Rect rect; char text[UI_TEXT]; const void *projection; } Control;
typedef struct {
    Control controls[UI_MAX]; int count, width, height, scale, content, scroll;
    int focus, pressed, pointer, start_x, start_y, last_y; bool dragging;
} Controls;
typedef enum { EVENT_NONE, EVENT_ACTIVATE, EVENT_FOCUS } EventKind;
typedef struct { EventKind kind; int id; } ControlEvent;
typedef enum { TOUCH_DOWN, TOUCH_MOVE, TOUCH_UP, TOUCH_CANCEL } Touch;
void controls_init(Controls *ui);
void controls_begin(Controls *ui, int width, int height, bool keyboard);
void controls_add(Controls *ui,int id,ControlKind kind,const char *text,int height,const void *projection);
void controls_end(Controls *ui);
void controls_scroll(Controls *ui,int offset);
int controls_hit(const Controls *ui,int x,int y);
ControlEvent controls_touch(Controls *ui,Touch touch,int pointer,int x,int y);
int controls_text_height(const Controls *ui,const char *text);
#endif
