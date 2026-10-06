#ifndef YOUNG_NEARBY_H
#define YOUNG_NEARBY_H
#include "console.h"
enum {
    ADDABLE_BASE=200,
    SHAPE_UNDO=300, SHAPE_RESET=301,
    REMOVABLE_BASE=500,
    PLOT_LEFT=410, PLOT_RIGHT, PLOT_UP, PLOT_DOWN,
    PLOT_ZOOM_IN, PLOT_ZOOM_OUT, PLOT_RESET
};
void nearby_shape(Console *console, Controls *ui);
void nearby_plot_controls(Console *console, Controls *ui);
bool nearby_event(Console *console, Controls *ui, ControlEvent event);
#endif
