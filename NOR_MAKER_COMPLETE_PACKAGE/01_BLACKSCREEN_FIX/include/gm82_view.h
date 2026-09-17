#ifndef GM82_VIEW_H
#define GM82_VIEW_H

#include "gm82_runtime.h"
#include <stdint.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef struct {
    int enabled;
    double view_x, view_y;
    double view_w, view_h;
    double port_x, port_y;
    double port_w, port_h;
    int follow_object;   /* object_index to follow, -1 = none */
    double hborder, vborder;
    double hspeed, vspeed; /* max follow speed, -1 = unlimited */
} gm82_view;

typedef struct {
    gm82_view views[8];
    int current; /* active view index for drawing */
} gm82_view_state;

void gm82_view_init(gm82_view_state *vs, int room_w, int room_h);
void gm82_view_update(gm82_view_state *vs, gm82_runtime *rt);

/* Map world → screen for soft render clipping */
void gm82_view_world_to_screen(const gm82_view *v, double wx, double wy, int *sx, int *sy);

#ifdef __cplusplus
}
#endif

#endif
