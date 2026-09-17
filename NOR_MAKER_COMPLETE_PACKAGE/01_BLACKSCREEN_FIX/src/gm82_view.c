#define _POSIX_C_SOURCE 200809L
#include "gm82_view.h"
#include <string.h>

void gm82_view_init(gm82_view_state *vs, int room_w, int room_h) {
    memset(vs, 0, sizeof(*vs));
    vs->current = 0;
    vs->views[0].enabled = 1;
    vs->views[0].view_x = 0;
    vs->views[0].view_y = 0;
    vs->views[0].view_w = room_w > 640 ? 640 : (double)room_w;
    vs->views[0].view_h = room_h > 480 ? 480 : (double)room_h;
    vs->views[0].port_w = vs->views[0].view_w;
    vs->views[0].port_h = vs->views[0].view_h;
    vs->views[0].follow_object = -1;
    vs->views[0].hborder = 64;
    vs->views[0].vborder = 64;
    vs->views[0].hspeed = -1;
    vs->views[0].vspeed = -1;
}

void gm82_view_update(gm82_view_state *vs, gm82_runtime *rt) {
    if (!vs || !rt) return;
    for (int i = 0; i < 8; i++) {
        gm82_view *v = &vs->views[i];
        if (!v->enabled || v->follow_object < 0) continue;

        /* find first instance of follow_object */
        gm82_instance *target = NULL;
        for (int j = 0; j < rt->instance_count; j++) {
            if (rt->instances[j].alive &&
                rt->instances[j].object_index == v->follow_object) {
                target = &rt->instances[j];
                break;
            }
        }
        if (!target) continue;

        double cx = target->x;
        double cy = target->y;
        double left = v->view_x + v->hborder;
        double right = v->view_x + v->view_w - v->hborder;
        double top = v->view_y + v->vborder;
        double bottom = v->view_y + v->view_h - v->vborder;

        if (cx < left) v->view_x -= (left - cx);
        if (cx > right) v->view_x += (cx - right);
        if (cy < top) v->view_y -= (top - cy);
        if (cy > bottom) v->view_y += (cy - bottom);

        /* clamp to room */
        if (v->view_x < 0) v->view_x = 0;
        if (v->view_y < 0) v->view_y = 0;
        if (v->view_x + v->view_w > rt->room_width)
            v->view_x = rt->room_width - v->view_w;
        if (v->view_y + v->view_h > rt->room_height)
            v->view_y = rt->room_height - v->view_h;
        if (v->view_x < 0) v->view_x = 0;
        if (v->view_y < 0) v->view_y = 0;
    }
}

void gm82_view_world_to_screen(const gm82_view *v, double wx, double wy, int *sx, int *sy) {
    if (!v || !sx || !sy) return;
    *sx = (int)(wx - v->view_x);
    *sy = (int)(wy - v->view_y);
}
