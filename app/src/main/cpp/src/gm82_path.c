#define _POSIX_C_SOURCE 200809L
#include "gm82_path.h"
#include <string.h>
#include <math.h>

void gm82_path_list_init(gm82_path_list *L) {
    memset(L, 0, sizeof(*L));
}

int gm82_path_add(gm82_path_list *L, const char *name, int closed) {
    if (!L || L->count >= GM82_PATH_MAX) return -1;
    gm82_path *p = &L->items[L->count];
    memset(p, 0, sizeof(*p));
    if (name) strncpy(p->name, name, sizeof(p->name)-1);
    p->closed = closed ? 1 : 0;
    return L->count++;
}

int gm82_path_add_point(gm82_path_list *L, int path_index, double x, double y, double speed) {
    if (!L || path_index < 0 || path_index >= L->count) return 0;
    gm82_path *p = &L->items[path_index];
    if (p->count >= GM82_PATH_MAX_POINTS) return 0;
    p->points[p->count].x = x;
    p->points[p->count].y = y;
    p->points[p->count].speed = speed > 0 ? speed : 1.0;
    p->count++;
    return 1;
}

int gm82_path_advance(const gm82_path *path, double *x, double *y, double *pos, double step) {
    if (!path || path->count < 2 || !x || !y || !pos) return 0;
    double p = *pos;
    int n = path->count;
    int i0 = (int)p;
    if (i0 < 0) i0 = 0;
    if (i0 >= n - 1 && !path->closed) {
        *x = path->points[n-1].x;
        *y = path->points[n-1].y;
        return 0;
    }
    if (i0 >= n) i0 = i0 % n;
    int i1 = i0 + 1;
    if (i1 >= n) i1 = path->closed ? 0 : n - 1;
    double frac = p - (int)p;
    double x0 = path->points[i0].x, y0 = path->points[i0].y;
    double x1 = path->points[i1].x, y1 = path->points[i1].y;
    double dx = x1 - x0, dy = y1 - y0;
    double len = sqrt(dx*dx + dy*dy);
    if (len < 0.001) {
        *pos = p + 1.0;
        *x = x1; *y = y1;
        return 1;
    }
    double spd = path->points[i0].speed;
    double move = step * spd;
    double need = (1.0 - frac) * len;
    if (move < need) {
        double t = frac + move / len;
        *x = x0 + dx * t;
        *y = y0 + dy * t;
        *pos = (double)i0 + t;
        return 1;
    }
    *pos = (double)i0 + 1.0;
    *x = x1; *y = y1;
    return 1;
}
