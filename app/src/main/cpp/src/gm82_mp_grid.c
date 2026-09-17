#define _POSIX_C_SOURCE 200809L
#include "gm82_mp_grid.h"
#include <string.h>
#include <stdlib.h>

void gm82_mp_grid_world_init(gm82_mp_grid_world *w) {
    memset(w, 0, sizeof(*w));
}

int gm82_mp_grid_create(gm82_mp_grid_world *w, int left, int top, int hcells, int vcells, int cellw, int cellh) {
    if (hcells < 1 || vcells < 1 || hcells > GM82_MP_GRID_CELLS || vcells > GM82_MP_GRID_CELLS) return -1;
    for (int i = 0; i < GM82_MP_GRID_MAX; i++) {
        if (w->grids[i].used) continue;
        gm82_mp_grid *g = &w->grids[i];
        memset(g, 0, sizeof(*g));
        g->used = 1;
        g->left = left; g->top = top;
        g->w = hcells; g->h = vcells;
        g->cell_w = cellw > 0 ? cellw : 16;
        g->cell_h = cellh > 0 ? cellh : 16;
        return i;
    }
    return -1;
}

void gm82_mp_grid_destroy(gm82_mp_grid_world *w, int id) {
    if (id < 0 || id >= GM82_MP_GRID_MAX) return;
    memset(&w->grids[id], 0, sizeof(w->grids[id]));
}

void gm82_mp_grid_clear_all(gm82_mp_grid_world *w, int id, int solid) {
    if (id < 0 || id >= GM82_MP_GRID_MAX || !w->grids[id].used) return;
    memset(w->grids[id].cells, solid ? 1 : 0, sizeof(w->grids[id].cells));
}

void gm82_mp_grid_add_cell(gm82_mp_grid_world *w, int id, int cx, int cy, int solid) {
    if (id < 0 || id >= GM82_MP_GRID_MAX || !w->grids[id].used) return;
    gm82_mp_grid *g = &w->grids[id];
    if (cx < 0 || cy < 0 || cx >= g->w || cy >= g->h) return;
    g->cells[cy * GM82_MP_GRID_CELLS + cx] = solid ? 1 : 0;
}

static int cell_at(gm82_mp_grid *g, int cx, int cy) {
    if (cx < 0 || cy < 0 || cx >= g->w || cy >= g->h) return 1;
    return g->cells[cy * GM82_MP_GRID_CELLS + cx];
}

int gm82_mp_grid_path(gm82_mp_grid_world *w, int id,
                      double xstart, double ystart, double xgoal, double ygoal,
                      double *ox, double *oy, int max_pts, int allowdiag) {
    if (!ox || !oy || max_pts < 2) return 0;
    if (id < 0 || id >= GM82_MP_GRID_MAX || !w->grids[id].used) return 0;
    gm82_mp_grid *g = &w->grids[id];
    int sx = (int)((xstart - g->left) / g->cell_w);
    int sy = (int)((ystart - g->top) / g->cell_h);
    int gx = (int)((xgoal - g->left) / g->cell_w);
    int gy = (int)((ygoal - g->top) / g->cell_h);
    if (sx < 0 || sy < 0 || gx < 0 || gy < 0 || sx >= g->w || sy >= g->h || gx >= g->w || gy >= g->h)
        return 0;
    if (cell_at(g, gx, gy)) return 0;

    int total = g->w * g->h;
    int *came = (int *)malloc((size_t)total * sizeof(int));
    int *q = (int *)malloc((size_t)total * sizeof(int));
    uint8_t *seen = (uint8_t *)calloc((size_t)total, 1);
    if (!came || !q || !seen) { free(came); free(q); free(seen); return 0; }
    for (int i = 0; i < total; i++) came[i] = -1;

    int qh = 0, qt = 0;
    int start = sy * g->w + sx;
    int goal = gy * g->w + gx;
    q[qt++] = start;
    seen[start] = 1;

    int dirs[8][2] = {{1,0},{-1,0},{0,1},{0,-1},{1,1},{1,-1},{-1,1},{-1,-1}};
    int nd = allowdiag ? 8 : 4;
    int found = 0;
    while (qh < qt) {
        int cur = q[qh++];
        if (cur == goal) { found = 1; break; }
        int cx = cur % g->w, cy = cur / g->w;
        for (int d = 0; d < nd; d++) {
            int nx = cx + dirs[d][0], ny = cy + dirs[d][1];
            if (nx < 0 || ny < 0 || nx >= g->w || ny >= g->h) continue;
            if (cell_at(g, nx, ny)) continue;
            int ni = ny * g->w + nx;
            if (seen[ni]) continue;
            seen[ni] = 1;
            came[ni] = cur;
            q[qt++] = ni;
        }
    }

    int npts = 0;
    if (found) {
        int path[4096];
        int len = 0;
        for (int c = goal; c >= 0 && len < 4096; c = came[c]) path[len++] = c;
        /* reverse */
        for (int i = len - 1; i >= 0 && npts < max_pts; i--) {
            int cx = path[i] % g->w, cy = path[i] / g->w;
            ox[npts] = g->left + (cx + 0.5) * g->cell_w;
            oy[npts] = g->top + (cy + 0.5) * g->cell_h;
            npts++;
        }
    }
    free(came); free(q); free(seen);
    return npts;
}
