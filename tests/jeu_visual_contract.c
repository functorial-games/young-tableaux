/* JDT + painter contract: actual console event -> typed projection -> pixels.
   Identical viewport/scroll isolates mathematical image changes from UI reflow. */
#include "console.h"
#include "paint.h"
#include <math.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define W 320
#define H 192
#define PIXELS ((size_t)W * (size_t)H)
#define CHECK(condition) do { if (!(condition)) { \
    fprintf(stderr, "JDT_VISUAL_FAIL line %d: %s\n", __LINE__, #condition); \
    return 1; } } while (0)

static int painted(Console *console, ControlKind kind, const void *projection,
                   uint32_t *pixels)
{
    Controls *ui = calloc(1, sizeof(*ui));
    if (!ui) return 0;
    controls_init(ui);
    controls_begin(ui, W, H, false);
    controls_add_at(ui, 0, kind, "", (Rect){4,4,W-8,H-8}, projection);
    controls_end(ui);
    Canvas canvas = {W,H,W,pixels,0,H};
    paint_controls(&canvas, ui, console->french);
    free(ui);
    return 1;
}

static size_t changed_pixels(const uint32_t *before, const uint32_t *after)
{
    size_t changed = 0;
    for (size_t i=0; i<PIXELS; ++i)
        if (before[i] != after[i]) ++changed;
    return changed;
}

/* Check the exact projection objects given to production console_layout, not
   a parallel copy of the mathematical trace or a string-only status change. */
static int ui_has_projection(Console *console, Controls *ui, ControlKind kind,
                             const void *projection)
{
    console_layout(console, ui, 576, 1152);
    for (int i=0; i<ui->count; ++i)
        if (ui->controls[i].kind == kind && ui->controls[i].projection == projection)
            return 1;
    return 0;
}

int main(void)
{
    Console *console = calloc(1, sizeof(*console));
    Controls *ui = calloc(1, sizeof(*ui));
    uint32_t *first = calloc(PIXELS, sizeof(*first));
    uint32_t *second = calloc(PIXELS, sizeof(*second));
    uint32_t *third = calloc(PIXELS, sizeof(*third));
    CHECK(console && ui && first && second && third);
    console_init(console);
    controls_init(ui);
    CHECK(console->jeu_ok && console->jeu_loaded && console->partition_ok);
    CHECK(ui_has_projection(console, ui, DIAGRAM, &console->jeu_tiles));
    CHECK(ui_has_projection(console, ui, WEGERT, &console->wegert));
    CHECK(ui_has_projection(console, ui, WEGERT, &console->jeu_wegert));
    CHECK(console->jeu_wegert.valid);
    CHECK(memcmp(&console->wegert, &console->jeu_wegert, sizeof(console->wegert)) == 0);
    CHECK(painted(console, DIAGRAM, &console->jeu_tiles, first));
    CHECK(painted(console, WEGERT, &console->wegert, second));
    WegertProjection fixed_shape = console->wegert;
    CHECK(fixed_shape.valid && fixed_shape.n_lambda == 4);
    uint32_t *static_wegert = malloc(PIXELS * sizeof(*static_wegert));
    CHECK(static_wegert);
    memcpy(static_wegert, second, PIXELS * sizeof(*second));
    CHECK(painted(console, WEGERT, &console->jeu_wegert, second));
    CHECK(changed_pixels(static_wegert, second) == 0);

    /* Canonical λ=(3,2,1), μ=(1), entries 1,3;2,5;4.
       STEP creates a hole at (1,1), then moves 1 into it from (1,2). */
    console_event(console, ui, (ControlEvent){EVENT_ACTIVATE, JDT_STEP});
    CHECK(console->jeu_ok && console->jeu_has_before && console->jeu.active);
    CHECK(console->jeu.hole.row == 1 && console->jeu.hole.column == 2);
    CHECK(console->jeu.filling.entries[0][0] == 1);
    CHECK(console->jeu.filling.entries[0][1] == 0);
    CHECK(console->jeu_before_tiles.marks[0][0] == 2);
    CHECK(console->jeu_tiles.marks[0][1] == 2);
    CHECK(ui_has_projection(console, ui, DIAGRAM, &console->jeu_before_tiles));
    CHECK(ui_has_projection(console, ui, DIAGRAM, &console->jeu_tiles));
    CHECK(painted(console, DIAGRAM, &console->jeu_before_tiles, first));
    CHECK(painted(console, DIAGRAM, &console->jeu_tiles, third));
    size_t first_step_delta = changed_pixels(first, third);
    CHECK(first_step_delta > 20);

    /* Hook specialization depends on λ, not the path through a fixed skew shape. */
    CHECK(memcmp(&fixed_shape, &console->wegert, sizeof(fixed_shape)) == 0);
    CHECK(painted(console, WEGERT, &console->wegert, second));
    CHECK(changed_pixels(static_wegert, second) == 0);
    CHECK(painted(console, WEGERT, &console->jeu_wegert, second));
    CHECK(changed_pixels(static_wegert, second) == 0);
    CHECK(console->jeu_has_before);

    console_event(console, ui, (ControlEvent){EVENT_ACTIVATE, JDT_STEP});
    CHECK(console->jeu.active && console->jeu.hole.row == 1 && console->jeu.hole.column == 3);
    CHECK(console->jeu.filling.entries[0][1] == 3);
    CHECK(console->jeu.filling.entries[0][2] == 0);
    CHECK(painted(console, DIAGRAM, &console->jeu_tiles, first));
    CHECK(changed_pixels(first, third) > 20);

    console_event(console, ui, (ControlEvent){EVENT_ACTIVATE, JDT_STEP});
    CHECK(!console->jeu.active && console->jeu_ok);
    CHECK(console->jeu.filling.shape.outer.count == 3);
    CHECK(console->jeu.filling.shape.outer.rows[0] == 2);
    CHECK(console->jeu.filling.shape.inner.count == 0);
    CHECK(console->jeu.filling.entries[0][0] == 1);
    CHECK(console->jeu.filling.entries[0][1] == 3);
    CHECK(console->jeu.filling.entries[1][0] == 2);
    CHECK(console->jeu.filling.entries[1][1] == 5);
    CHECK(console->jeu.filling.entries[2][0] == 4);
    CHECK(painted(console, DIAGRAM, &console->jeu_tiles, third));
    CHECK(changed_pixels(first, third) > 20);
    CHECK(painted(console, WEGERT, &console->wegert, second));
    CHECK(changed_pixels(static_wegert, second) == 0);
    CHECK(console->jeu_wegert.valid);
    CHECK(memcmp(&console->wegert, &console->jeu_wegert, sizeof(console->wegert)) != 0);
    CHECK(ui_has_projection(console, ui, WEGERT, &console->jeu_wegert));
    CHECK(painted(console, WEGERT, &console->jeu_wegert, second));
    size_t jeu_shape_delta = changed_pixels(static_wegert, second);
    CHECK(jeu_shape_delta > 20);

    /* The independent whole-slide command and three STEP events agree. */
    TileProjection completed = console->jeu_tiles;
    console_event(console, ui, (ControlEvent){EVENT_ACTIVATE, JDT_RESET});
    CHECK(console->jeu_ok && console->jeu.filling.shape.inner.count == 1);
    console_event(console, ui, (ControlEvent){EVENT_ACTIVATE, JDT_RECTIFY});
    CHECK(console->jeu_ok && !console->jeu.active);
    CHECK(memcmp(&completed, &console->jeu_tiles, sizeof(completed)) == 0);
    CHECK(painted(console, WEGERT, &console->jeu_wegert, second));
    CHECK(changed_pixels(static_wegert, second) == jeu_shape_delta);
    CHECK(painted(console, DIAGRAM, &console->jeu_tiles, first));
    CHECK(changed_pixels(first, third) == 0);

    /* Changing λ changes its hook specialization AND the actual raster. */
    ui->focus = SET_LAMBDA;
    console_key(console, ui, 15); /* clear */
    console_key(console, ui, 1);  /* 2 */
    console_key(console, ui, 10); /* comma */
    console_key(console, ui, 0);  /* 1 */
    CHECK(strcmp(console->fields[SET_LAMBDA], "2,1") == 0);
    CHECK(console->partition_ok && console->wegert.n_lambda == 1);
    CHECK(memcmp(&fixed_shape, &console->wegert, sizeof(fixed_shape)) != 0);
    CHECK(painted(console, WEGERT, &console->wegert, second));
    size_t shape_delta = changed_pixels(static_wegert, second);
    CHECK(shape_delta > 20);

    /* A failure message must not substitute for moving mathematical pixels. */
    console_event(console, ui, (ControlEvent){EVENT_ACTIVATE, JDT_RESET});
    CHECK(!console->jeu_ok);
    CHECK(painted(console, DIAGRAM, &console->jeu_tiles, first));
    CHECK(changed_pixels(first, third) > 20);

    printf("JDT_VISUAL PASS: STEP changes %zu tableau pixels; corner removal changes %zu jeu-outer Wegert pixels; editing λ changes %zu reference Wegert pixels\n",
           first_step_delta, jeu_shape_delta, shape_delta);
    free(static_wegert);
    free(third);
    free(second);
    free(first);
    free(ui);
    free(console);
    return 0;
}
