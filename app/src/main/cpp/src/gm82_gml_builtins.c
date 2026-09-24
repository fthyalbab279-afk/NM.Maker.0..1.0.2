#define _POSIX_C_SOURCE 200809L
#include <stdio.h>
#include <time.h>
#include "gm82_gml_builtins.h"
#include "gm82_path.h"
#include "gm82_timeline.h"
#include "gm82_gml_eval.h"
#include "gm82_particles.h"
#include "gm82_mp_grid.h"
#include "gm82_script.h"
#include "gm82_gml_eval.h"

static double g_score = 0;
static double g_lives = 3;
static double g_health = 100;
static int g_game_ended = 0;
static double g_timer_ms = 0;
static double g_delta = 1.0/30.0;
static uint8_t *g_draw_buf = NULL;
static int32_t g_draw_w = 0, g_draw_h = 0;
static uint32_t g_draw_color = 0xFFFFFF;
static double g_draw_alpha = 1.0;
#include <math.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

static gm82_runtime *g_rt = NULL;
static gm82_instance *g_self = NULL;

void gm82_gml_set_runtime(gm82_runtime *rt) { g_rt = rt; }
gm82_runtime *gm82_gml_get_runtime(void) { return g_rt; }
void gm82_gml_set_self(gm82_instance *self) { g_self = self; }
gm82_instance *gm82_gml_get_self(void) { return g_self; }

double gml_instance_create(double x, double y, double object_index) {
    if (!g_rt) return -1;
    gm82_instance *inst = gm82_runtime_instance_create(g_rt, (int32_t)object_index, x, y);
    return inst ? (double)inst->id : -1;
}

void gml_instance_destroy(void) {
    if (!g_rt || !g_self) return;
    gm82_runtime_instance_destroy(g_rt, g_self);
}

double gml_instance_number(double object_index) {
    if (!g_rt) return 0;
    int32_t oi = (int32_t)object_index;
    int n = 0;
    for (int i = 0; i < g_rt->instance_count; i++) {
        if (!g_rt->instances[i].alive) continue;
        if (oi < 0 || g_rt->instances[i].object_index == oi) n++;
    }
    return (double)n;
}

double gml_instance_exists(double id_or_object) {
    if (!g_rt) return 0;
    int32_t v = (int32_t)id_or_object;
    for (int i = 0; i < g_rt->instance_count; i++) {
        if (!g_rt->instances[i].alive) continue;
        if (g_rt->instances[i].id == v || g_rt->instances[i].object_index == v)
            return 1;
    }
    return 0;
}

void gml_motion_set(double dir, double spd) {
    if (!g_self) return;
    g_self->direction = dir;
    g_self->speed = spd;
    double rad = dir * M_PI / 180.0;
    g_self->hspeed = cos(rad) * spd;
    g_self->vspeed = -sin(rad) * spd;
}

void gml_motion_add(double dir, double spd) {
    if (!g_self) return;
    double rad = dir * M_PI / 180.0;
    g_self->hspeed += cos(rad) * spd;
    g_self->vspeed += -sin(rad) * spd;
    g_self->speed = sqrt(g_self->hspeed * g_self->hspeed + g_self->vspeed * g_self->vspeed);
    if (g_self->speed > 0.0001)
        g_self->direction = atan2(-g_self->vspeed, g_self->hspeed) * 180.0 / M_PI;
}

void gml_move_towards_point(double tx, double ty, double sp) {
    if (!g_self) return;
    double dx = tx - g_self->x;
    double dy = ty - g_self->y;
    double dist = sqrt(dx * dx + dy * dy);
    if (dist < 0.0001) { g_self->hspeed = 0; g_self->vspeed = 0; g_self->speed = 0; return; }
    g_self->hspeed = dx / dist * sp;
    g_self->vspeed = dy / dist * sp;
    g_self->speed = sp;
    g_self->direction = atan2(-dy, dx) * 180.0 / M_PI;
}

double gml_point_distance(double x1, double y1, double x2, double y2) {
    double dx = x2 - x1, dy = y2 - y1;
    return sqrt(dx * dx + dy * dy);
}

double gml_point_direction(double x1, double y1, double x2, double y2) {
    return atan2(-(y2 - y1), (x2 - x1)) * 180.0 / M_PI;
}

static bool sprite_size(gm82_runtime *rt, int32_t sprite_index, int32_t *w, int32_t *h) {
    *w = 16; *h = 16;
    if (!rt || !rt->sprites || sprite_index < 0 || sprite_index >= rt->sprites->count)
        return false;
    *w = rt->sprites->frames[sprite_index].width;
    *h = rt->sprites->frames[sprite_index].height;
    return true;
}

static bool aabb_overlap(double ax, double ay, int aw, int ah,
                         double bx, double by, int bw, int bh) {
    return ax < bx + bw && ax + aw > bx && ay < by + bh && ay + ah > by;
}

double gml_place_meeting(double x, double y, double object_index) {
    if (!g_rt || !g_self) return 0;
    int32_t oi = (int32_t)object_index;
    int32_t sw, sh;
    sprite_size(g_rt, g_self->sprite_index, &sw, &sh);
    for (int i = 0; i < g_rt->instance_count; i++) {
        gm82_instance *o = &g_rt->instances[i];
        if (!o->alive || o == g_self) continue;
        if (oi >= 0 && o->object_index != oi) continue;
        int32_t ow, oh;
        sprite_size(g_rt, o->sprite_index, &ow, &oh);
        if (aabb_overlap(x, y, sw, sh, o->x, o->y, ow, oh)) return 1;
    }
    /* also solid tiles when object_index < 0 (all) */
    if (oi < 0 && g_rt->rooms && g_rt->current_room >= 0 && g_rt->current_room < g_rt->rooms->count) {
        const gm82_decoded_room *room = &g_rt->rooms->items[g_rt->current_room];
        for (int ti = 0; ti < room->tile_count; ti++) {
            const gm82_decoded_tile *tile = &room->tiles[ti];
            if (aabb_overlap(x, y, sw, sh, tile->x, tile->y, tile->width, tile->height))
                return 1;
        }
    }
    return 0;
}

double gml_position_meeting(double x, double y, double object_index) {
    if (!g_rt) return 0;
    int32_t oi = (int32_t)object_index;
    for (int i = 0; i < g_rt->instance_count; i++) {
        gm82_instance *o = &g_rt->instances[i];
        if (!o->alive) continue;
        if (oi >= 0 && o->object_index != oi) continue;
        int32_t ow, oh;
        sprite_size(g_rt, o->sprite_index, &ow, &oh);
        if (x >= o->x && x < o->x + ow && y >= o->y && y < o->y + oh) return 1;
    }
    return 0;
}

double gml_instance_place(double x, double y, double object_index) {
    if (!g_rt || !g_self) return -4; /* noone */
    int32_t oi = (int32_t)object_index;
    int32_t sw, sh;
    sprite_size(g_rt, g_self->sprite_index, &sw, &sh);
    for (int i = 0; i < g_rt->instance_count; i++) {
        gm82_instance *o = &g_rt->instances[i];
        if (!o->alive || o == g_self) continue;
        if (oi >= 0 && o->object_index != oi) continue;
        int32_t ow, oh;
        sprite_size(g_rt, o->sprite_index, &ow, &oh);
        if (aabb_overlap(x, y, sw, sh, o->x, o->y, ow, oh))
            return (double)o->id;
    }
    return -4;
}

void gm82_draw_set_target(uint8_t *rgba, int32_t w, int32_t h) {
    g_draw_buf = rgba; g_draw_w = w; g_draw_h = h;
}
double gml_draw_clear(double color) {
    if (!g_draw_buf || g_draw_w <= 0 || g_draw_h <= 0) return 0;
    uint32_t c = (uint32_t)color;
    uint8_t r = (c >> 16) & 255, g = (c >> 8) & 255, b = c & 255;
    size_t n = (size_t)g_draw_w * (size_t)g_draw_h;
    for (size_t i = 0; i < n; i++) {
        g_draw_buf[i*4]=r; g_draw_buf[i*4+1]=g; g_draw_buf[i*4+2]=b; g_draw_buf[i*4+3]=255;
    }
    return 1;
}
void gml_draw_sprite(double sprite, double x, double y) {
    if (!g_draw_buf || !g_rt || !g_rt->sprites) return;
    int si = (int)sprite;
    if (si < 0 || si >= g_rt->sprites->count) return;
    const gm82_decoded_frame *fr = &g_rt->sprites->frames[si];
    if (!fr->rgba) return;
    int dx0 = (int)x, dy0 = (int)y;
    for (int sy = 0; sy < fr->height; sy++) {
        int dy = dy0 + sy;
        if (dy < 0 || dy >= g_draw_h) continue;
        for (int sx = 0; sx < fr->width; sx++) {
            int dx = dx0 + sx;
            if (dx < 0 || dx >= g_draw_w) continue;
            const uint8_t *s = fr->rgba + ((size_t)sy * (size_t)fr->width + (size_t)sx) * 4;
            if (s[3] == 0) continue;
            uint8_t *d = g_draw_buf + ((size_t)dy * (size_t)g_draw_w + (size_t)dx) * 4;
            d[0]=s[0]; d[1]=s[1]; d[2]=s[2]; d[3]=255;
        }
    }
}

void gml_draw_sprite_ext(double sprite, double subimg, double x, double y,
                         double xscale, double yscale, double rot, double color, double alpha) {
    (void)sprite; (void)subimg; (void)x; (void)y;
    (void)xscale; (void)yscale; (void)rot; (void)color; (void)alpha;
}

double gml_get_x(void) { return g_self ? g_self->x : 0; }
double gml_get_y(void) { return g_self ? g_self->y : 0; }
double gml_get_bbox_left(void) { return g_self ? g_self->x : 0; }
double gml_get_bbox_right(void) {
    if (!g_self) return 0;
    int32_t sw = 16;
    if (g_rt && g_rt->sprites && g_self->sprite_index >= 0 && g_self->sprite_index < g_rt->sprites->count)
        sw = g_rt->sprites->frames[g_self->sprite_index].width;
    return g_self->x + sw - 1;
}
double gml_get_bbox_top(void) { return g_self ? g_self->y : 0; }
double gml_get_bbox_bottom(void) {
    if (!g_self) return 0;
    int32_t sh = 16;
    if (g_rt && g_rt->sprites && g_self->sprite_index >= 0 && g_self->sprite_index < g_rt->sprites->count)
        sh = g_rt->sprites->frames[g_self->sprite_index].height;
    return g_self->y + sh - 1;
}
void gml_set_x(double v) { if (g_self) g_self->x = v; }
void gml_set_y(double v) { if (g_self) g_self->y = v; }
double gml_get_hspeed(void) { return g_self ? g_self->hspeed : 0; }
double gml_get_vspeed(void) { return g_self ? g_self->vspeed : 0; }
void gml_set_hspeed(double v) { if (g_self) g_self->hspeed = v; }
void gml_set_vspeed(double v) { if (g_self) g_self->vspeed = v; }
double gml_get_direction(void) { return g_self ? g_self->direction : 0; }
void gml_set_direction(double v) {
    if (!g_self) return;
    g_self->direction = v;
    double rad = v * M_PI / 180.0;
    g_self->hspeed = cos(rad) * g_self->speed;
    g_self->vspeed = -sin(rad) * g_self->speed;
}
double gml_get_speed(void) { return g_self ? g_self->speed : 0; }
void gml_set_speed(double v) {
    if (!g_self) return;
    g_self->speed = v;
    double rad = g_self->direction * M_PI / 180.0;
    g_self->hspeed = cos(rad) * v;
    g_self->vspeed = -sin(rad) * v;
}
double gml_get_sprite_index(void) { return g_self ? (double)g_self->sprite_index : -1; }
void gml_set_sprite_index(double v) { if (g_self) g_self->sprite_index = (int32_t)v; }
double gml_get_image_index(void) { return g_self ? (double)g_self->image_index : 0; }
void gml_set_image_index(double v) { if (g_self) g_self->image_index = (int32_t)v; }
double gml_get_solid(void) { return g_self ? (double)g_self->solid : 0; }
double gml_get_visible(void) { return g_self ? (double)g_self->visible : 0; }

double gml_room_width(void) { return g_rt ? (double)g_rt->room_width : 0; }
double gml_room_height(void) { return g_rt ? (double)g_rt->room_height : 0; }
double gml_room_speed(void) { return g_rt ? (double)g_rt->room_speed : 30; }

double gml_abs(double v) { return fabs(v); }
double gml_sign(double v) { return (v > 0) - (v < 0); }
double gml_clamp(double v, double lo, double hi) {
    if (v < lo) return lo;
    if (v > hi) return hi;
    return v;
}
double gml_lerp(double a, double b, double t) { return a + (b - a) * t; }
double gml_irandom(double n) {
    if (n <= 0) return 0;
    return (double)(rand() % ((int)n + 1));
}
double gml_random(double n) {
    return ((double)rand() / (double)RAND_MAX) * n;
}

double gml_room_goto(double room_index) {
    if (!g_rt) return 0;
    return gm82_runtime_goto_room(g_rt, (int)room_index) ? 1.0 : 0.0;
}
double gml_room(void) {
    return g_rt ? (double)g_rt->current_room : 0;
}


double gml_room_restart(void) {
    if (!g_rt) return 0;
    return gm82_runtime_goto_room(g_rt, g_rt->current_room) ? 1.0 : 0.0;
}
double gml_room_previous(void) {
    if (!g_rt || g_rt->current_room <= 0) return 0;
    return gm82_runtime_goto_room(g_rt, g_rt->current_room - 1) ? 1.0 : 0.0;
}
double gml_room_next(void) {
    if (!g_rt || !g_rt->rooms) return 0;
    if (g_rt->current_room + 1 >= g_rt->rooms->count) return 0;
    return gm82_runtime_goto_room(g_rt, g_rt->current_room + 1) ? 1.0 : 0.0;
}
double gml_game_end(void) {
    g_game_ended = 1;
    if (!g_rt) return 0;
    g_rt->running = 0;
    return 1;
}

double gml_get_score(void) { return g_score; }
void   gml_set_score(double v) { g_score = v; }
double gml_get_lives(void) { return g_lives; }
void   gml_set_lives(double v) { g_lives = v; }
double gml_get_health(void) { return g_health; }
void   gml_set_health(double v) { g_health = v; }

double gml_instance_nearest(double x, double y, double object_index) {
    if (!g_rt) return -4;
    int32_t oi = (int32_t)object_index;
    double best = 1e300;
    int32_t best_id = -4;
    for (int i = 0; i < g_rt->instance_count; i++) {
        gm82_instance *o = &g_rt->instances[i];
        if (!o->alive) continue;
        if (oi >= 0 && o->object_index != oi) continue;
        if (o == g_self) continue;
        double dx = o->x - x, dy = o->y - y;
        double d = dx*dx + dy*dy;
        if (d < best) { best = d; best_id = o->id; }
    }
    return (double)best_id;
}

double gml_instance_find(double object_index, double n) {
    if (!g_rt) return -4;
    int32_t oi = (int32_t)object_index;
    int want = (int)n;
    int seen = 0;
    for (int i = 0; i < g_rt->instance_count; i++) {
        gm82_instance *o = &g_rt->instances[i];
        if (!o->alive) continue;
        if (oi >= 0 && o->object_index != oi) continue;
        if (seen == want) return (double)o->id;
        seen++;
    }
    return -4;
}

double gml_distance_to_object(double object_index) {
    if (!g_rt || !g_self) return 1000000;
    int32_t oi = (int32_t)object_index;
    double best = 1e300;
    for (int i = 0; i < g_rt->instance_count; i++) {
        gm82_instance *o = &g_rt->instances[i];
        if (!o->alive || o == g_self) continue;
        if (oi >= 0 && o->object_index != oi) continue;
        double dx = o->x - g_self->x, dy = o->y - g_self->y;
        double d = sqrt(dx*dx + dy*dy);
        if (d < best) best = d;
    }
    return best > 1e299 ? 1000000 : best;
}

double gml_point_in_rectangle(double px, double py, double x1, double y1, double x2, double y2) {
    if (x1 > x2) { double t=x1; x1=x2; x2=t; }
    if (y1 > y2) { double t=y1; y1=y2; y2=t; }
    return (px >= x1 && px <= x2 && py >= y1 && py <= y2) ? 1.0 : 0.0;
}

double gml_collision_rectangle(double x1, double y1, double x2, double y2, double obj, double prec, double notme) {
    (void)prec;
    if (!g_rt) return -4;
    if (x1 > x2) { double t=x1; x1=x2; x2=t; }
    if (y1 > y2) { double t=y1; y1=y2; y2=t; }
    int32_t oi = (int32_t)obj;
    for (int i = 0; i < g_rt->instance_count; i++) {
        gm82_instance *o = &g_rt->instances[i];
        if (!o->alive) continue;
        if (notme && o == g_self) continue;
        if (oi >= 0 && o->object_index != oi) continue;
        int32_t ow, oh;
        sprite_size(g_rt, o->sprite_index, &ow, &oh);
        if (aabb_overlap(x1, y1, x2-x1, y2-y1, o->x, o->y, ow, oh))
            return (double)o->id;
    }
    return -4;
}

double gml_collision_circle(double xc, double yc, double rad, double obj, double prec, double notme) {
    (void)prec;
    if (!g_rt) return -4;
    int32_t oi = (int32_t)obj;
    double rad_sq = rad * rad;
    for (int i = 0; i < g_rt->instance_count; i++) {
        gm82_instance *o = &g_rt->instances[i];
        if (!o->alive) continue;
        if (notme && o == g_self) continue;
        if (oi >= 0 && o->object_index != oi) continue;
        int32_t ow, oh;
        sprite_size(g_rt, o->sprite_index, &ow, &oh);

        double closest_x = xc;
        if (closest_x < o->x) closest_x = o->x;
        else if (closest_x > o->x + ow) closest_x = o->x + ow;

        double closest_y = yc;
        if (closest_y < o->y) closest_y = o->y;
        else if (closest_y > o->y + oh) closest_y = o->y + oh;

        double dx = xc - closest_x;
        double dy = yc - closest_y;
        if (dx * dx + dy * dy <= rad_sq)
            return (double)o->id;
    }
    return -4;
}

double gml_collision_point(double x, double y, double obj, double prec, double notme) {
    return gml_collision_rectangle(x, y, x+1, y+1, obj, prec, notme);
}

static bool line_aabb_overlap(double x1, double y1, double x2, double y2, double bx, double by, double bw, double bh) {
    double xmin = bx, xmax = bx + bw;
    double ymin = by, ymax = by + bh;
    double dx = x2 - x1, dy = y2 - y1;

    double p[4] = { -dx, dx, -dy, dy };
    double q[4] = { x1 - xmin, xmax - x1, y1 - ymin, ymax - y1 };
    double u1 = 0.0, u2 = 1.0;

    for (int i = 0; i < 4; i++) {
        if (p[i] == 0.0) {
            if (q[i] < 0.0) return false;
        } else {
            double t = q[i] / p[i];
            if (p[i] < 0.0) {
                if (t > u1) u1 = t;
            } else {
                if (t < u2) u2 = t;
            }
        }
    }
    return u1 <= u2;
}

double gml_collision_line(double x1, double y1, double x2, double y2, double obj, double prec, double notme) {
    (void)prec;
    if (!g_rt) return -4;
    int32_t oi = (int32_t)obj;
    for (int i = 0; i < g_rt->instance_count; i++) {
        gm82_instance *o = &g_rt->instances[i];
        if (!o->alive) continue;
        if (notme && o == g_self) continue;
        if (oi >= 0 && o->object_index != oi) continue;
        int32_t ow, oh;
        sprite_size(g_rt, o->sprite_index, &ow, &oh);
        if (line_aabb_overlap(x1, y1, x2, y2, o->x, o->y, ow, oh))
            return (double)o->id;
    }
    return -4;
}

double gml_place_free(double x, double y) {
    /* GM8: true iff no collision with a *solid object* at (x,y). Tiles are not objects. */
    if (!g_rt || !g_self) return 1;
    int32_t sw, sh;
    sprite_size(g_rt, g_self->sprite_index, &sw, &sh);
    for (int i = 0; i < g_rt->instance_count; i++) {
        gm82_instance *o = &g_rt->instances[i];
        if (!o->alive || o == g_self || !o->solid) continue;
        int32_t ow, oh;
        sprite_size(g_rt, o->sprite_index, &ow, &oh);
        if (aabb_overlap(x, y, sw, sh, o->x, o->y, ow, oh)) return 0;
    }
    return 1;
}

double gml_place_empty(double x, double y) {
    if (!g_rt || !g_self) return 1;
    int32_t sw, sh;
    sprite_size(g_rt, g_self->sprite_index, &sw, &sh);
    for (int i = 0; i < g_rt->instance_count; i++) {
        gm82_instance *o = &g_rt->instances[i];
        if (!o->alive || o == g_self) continue;
        int32_t ow, oh;
        sprite_size(g_rt, o->sprite_index, &ow, &oh);
        if (aabb_overlap(x, y, sw, sh, o->x, o->y, ow, oh)) return 0;
    }
    return 1;
}

double gml_move_contact_solid(double dir, double maxdist) {
    if (!g_rt || !g_self) return 0;
    if (maxdist < 0) maxdist = 1000;
    double rad = dir * 3.141592653589793 / 180.0;
    double dx = cos(rad), dy = -sin(rad);
    double step = 1.0;
    double moved = 0;
    while (moved < maxdist) {
        double nx = g_self->x + dx * step;
        double ny = g_self->y + dy * step;
        if (!gml_place_free(nx, ny)) break;
        g_self->x = nx;
        g_self->y = ny;
        moved += step;
    }
    return moved;
}

double gml_sprite_get_width(double sprite) {
    if (!g_rt || !g_rt->sprites) return 0;
    int si = (int)sprite;
    if (si < 0 || si >= g_rt->sprites->count) return 0;
    return (double)g_rt->sprites->frames[si].width;
}
double gml_sprite_get_height(double sprite) {
    if (!g_rt || !g_rt->sprites) return 0;
    int si = (int)sprite;
    if (si < 0 || si >= g_rt->sprites->count) return 0;
    return (double)g_rt->sprites->frames[si].height;
}
double gml_sprite_get_number(double sprite) {
    (void)sprite;
    /* multi-frame not fully tracked yet – report 1 if exists */
    return gml_sprite_exists(sprite);
}
double gml_sprite_exists(double sprite) {
    if (!g_rt || !g_rt->sprites) return 0;
    int si = (int)sprite;
    return (si >= 0 && si < g_rt->sprites->count && g_rt->sprites->frames[si].rgba) ? 1.0 : 0.0;
}
double gml_object_exists(double object_index) {
    if (!g_rt || !g_rt->objects) return 0;
    int oi = (int)object_index;
    return (oi >= 0 && oi < g_rt->objects->count) ? 1.0 : 0.0;
}
double gml_object_get_sprite(double object_index) {
    if (!g_rt || !g_rt->objects) return -1;
    int oi = (int)object_index;
    if (oi < 0 || oi >= g_rt->objects->count) return -1;
    return (double)g_rt->objects->items[oi].sprite_index;
}
double gml_object_get_solid(double object_index) {
    if (!g_rt || !g_rt->objects) return 0;
    int oi = (int)object_index;
    if (oi < 0 || oi >= g_rt->objects->count) return 0;
    return g_rt->objects->items[oi].solid ? 1.0 : 0.0;
}

void gm82_gml_set_frame_time(double seconds) {
    if (seconds > 0) g_delta = seconds;
    g_timer_ms += g_delta * 1000000.0; /* microseconds like GM */
}
double gml_get_timer(void) { return g_timer_ms; }
double gml_delta_time(void) { return g_delta * 1000000.0; }

void gml_draw_set_color(double color) { g_draw_color = (uint32_t)color; }
void gml_draw_set_alpha(double alpha) { g_draw_alpha = alpha; }
double gml_draw_get_alpha(void) { return g_draw_alpha; }
double gml_draw_get_color(void) { return (double)g_draw_color; }

static void put_px(int x, int y) {
    if (!g_draw_buf || x < 0 || y < 0 || x >= g_draw_w || y >= g_draw_h) return;
    uint8_t *d = g_draw_buf + ((size_t)y * (size_t)g_draw_w + (size_t)x) * 4;
    double a = g_draw_alpha;
    if (a < 0) a = 0;
    if (a > 1) a = 1;
    uint8_t sr = (g_draw_color >> 16) & 255;
    uint8_t sg = (g_draw_color >> 8) & 255;
    uint8_t sb = g_draw_color & 255;
    d[0] = (uint8_t)(d[0] * (1.0 - a) + sr * a);
    d[1] = (uint8_t)(d[1] * (1.0 - a) + sg * a);
    d[2] = (uint8_t)(d[2] * (1.0 - a) + sb * a);
    d[3] = 255;
}

void gml_draw_rectangle(double x1, double y1, double x2, double y2, double outline) {
    int a = (int)x1, b = (int)y1, c = (int)x2, d = (int)y2;
    if (a > c) { int t=a; a=c; c=t; }
    if (b > d) { int t=b; b=d; d=t; }
    if (outline) {
        for (int x = a; x <= c; x++) { put_px(x, b); put_px(x, d); }
        for (int y = b; y <= d; y++) { put_px(a, y); put_px(c, y); }
    } else {
        for (int y = b; y <= d; y++)
            for (int x = a; x <= c; x++) put_px(x, y);
    }
}

void gml_draw_circle(double x, double y, double r, double outline) {
    int cx = (int)x, cy = (int)y, rad = (int)r;
    if (rad < 0) rad = -rad;
    for (int dy = -rad; dy <= rad; dy++) {
        for (int dx = -rad; dx <= rad; dx++) {
            int dist2 = dx*dx + dy*dy;
            if (outline) {
                if (dist2 <= rad*rad && dist2 >= (rad-1)*(rad-1)) put_px(cx+dx, cy+dy);
            } else {
                if (dist2 <= rad*rad) put_px(cx+dx, cy+dy);
            }
        }
    }
}

void gml_draw_line(double x1, double y1, double x2, double y2) {
    int a = (int)x1, b = (int)y1, c = (int)x2, d = (int)y2;
    int dx = abs(c - a), sx = a < c ? 1 : -1;
    int dy = -abs(d - b), sy = b < d ? 1 : -1;
    int err = dx + dy;
    for (;;) {
        put_px(a, b);
        if (a == c && b == d) break;
        int e2 = 2 * err;
        if (e2 >= dy) { err += dy; a += sx; }
        if (e2 <= dx) { err += dx; b += sy; }
    }
}

#define GM82_DEBUG_MAX 64
static char g_debug_logs[GM82_DEBUG_MAX][128];
static int g_debug_count = 0;

void gm82_debug_log(const char *msg) {
    if (!msg) return;
    int i = g_debug_count % GM82_DEBUG_MAX;
    strncpy(g_debug_logs[i], msg, 127);
    g_debug_logs[i][127] = 0;
    g_debug_count++;
}

int gm82_debug_log_count(void) { return g_debug_count; }

const char *gm82_debug_log_get(int index) {
    if (index < 0 || index >= g_debug_count) return "";
    int start = (g_debug_count > GM82_DEBUG_MAX) ? (g_debug_count - GM82_DEBUG_MAX) : 0;
    int i = (start + index) % GM82_DEBUG_MAX;
    if (index >= GM82_DEBUG_MAX) return "";
    /* map linear index into ring */
    int real = (g_debug_count <= GM82_DEBUG_MAX) ? index : ((g_debug_count - GM82_DEBUG_MAX + index) % GM82_DEBUG_MAX);
    (void)i;
    real = (g_debug_count <= GM82_DEBUG_MAX) ? index : ((g_debug_count + index - GM82_DEBUG_MAX) % GM82_DEBUG_MAX);
    return g_debug_logs[real];
}

double gml_string_length(const char *s) {
    return s ? (double)strlen(s) : 0;
}

double gml_real(const char *s) {
    if (!s) return 0;
    return strtod(s, NULL);
}

double gml_string_pos(const char *sub, const char *str) {
    if (!sub || !str) return 0;
    const char *p = strstr(str, sub);
    return p ? (double)(p - str + 1) : 0;
}

double gml_string_char_at(const char *str, double index) {
    if (!str) return 0;
    int idx = (int)index - 1; /* GML 1-based indexing */
    int len = (int)strlen(str);
    if (idx < 0 || idx >= len) return 0;
    return (double)(unsigned char)str[idx];
}

double gml_string_digits(const char *str, char *out, size_t out_sz) {
    if (!out || out_sz == 0) return 0;
    out[0] = 0;
    if (!str) return 0;
    size_t k = 0;
    for (size_t i = 0; str[i] && k + 1 < out_sz; i++) {
        if (isdigit((unsigned char)str[i])) out[k++] = str[i];
    }
    out[k] = 0;
    return (double)k;
}

double gml_string_lower(const char *str, char *out, size_t out_sz) {
    if (!out || out_sz == 0) return 0;
    out[0] = 0;
    if (!str) return 0;
    size_t k = 0;
    for (size_t i = 0; str[i] && k + 1 < out_sz; i++) {
        out[k++] = (char)tolower((unsigned char)str[i]);
    }
    out[k] = 0;
    return (double)k;
}

double gml_string_upper(const char *str, char *out, size_t out_sz) {
    if (!out || out_sz == 0) return 0;
    out[0] = 0;
    if (!str) return 0;
    size_t k = 0;
    for (size_t i = 0; str[i] && k + 1 < out_sz; i++) {
        out[k++] = (char)toupper((unsigned char)str[i]);
    }
    out[k] = 0;
    return (double)k;
}

double gml_array_length_1d(double array_id) {
    (void)array_id;
    return 1.0;
}

double gml_array_height_2d(double array_id) {
    (void)array_id;
    return 1.0;
}


double gml_instance_change(double object_index, double perform_events) {
    (void)perform_events;
    if (!g_rt || !g_self) return 0;
    int oi = (int)object_index;
    if (!g_rt->objects || oi < 0 || oi >= g_rt->objects->count) return 0;
    g_self->object_index = oi;
    g_self->sprite_index = g_rt->objects->items[oi].sprite_index;
    g_self->solid = g_rt->objects->items[oi].solid;
    return 1;
}

double gml_instance_copy(double perform_events) {
    (void)perform_events;
    if (!g_rt || !g_self) return -4;
    gm82_instance *n = gm82_runtime_instance_create(g_rt, g_self->object_index, g_self->x, g_self->y);
    if (!n) return -4;
    n->hspeed = g_self->hspeed;
    n->vspeed = g_self->vspeed;
    n->direction = g_self->direction;
    n->speed = g_self->speed;
    n->image_index = g_self->image_index;
    n->image_speed = g_self->image_speed;
    n->gravity = g_self->gravity;
    n->gravity_direction = g_self->gravity_direction;
    n->friction = g_self->friction;
    return (double)n->id;
}

double gml_instance_deactivate_all(double notme) {
    if (!g_rt) return 0;
    for (int i = 0; i < g_rt->instance_count; i++) {
        if (notme && &g_rt->instances[i] == g_self) continue;
        g_rt->instances[i].alive = 0; /* soft deactivate = destroy for now */
    }
    return 1;
}

double gml_instance_activate_all(void) {
    /* full activate not tracked separately from alive yet */
    return 0;
}

static gm82_path_list *g_paths = NULL;
void gm82_path_bind(gm82_path_list *paths) { g_paths = paths; }

double gml_path_start(double path_index, double speed, double end_action, double absolute) {
    (void)end_action; (void)absolute;
    if (!g_self || !g_paths) return 0;
    int pi = (int)path_index;
    if (pi < 0 || pi >= g_paths->count) return 0;
    g_self->path_index = pi;
    g_self->path_position = 0;
    g_self->path_speed = speed;
    if (g_paths->items[pi].count > 0) {
        g_self->x = g_paths->items[pi].points[0].x;
        g_self->y = g_paths->items[pi].points[0].y;
    }
    return 1;
}

double gml_path_end(void) {
    if (!g_self) return 0;
    g_self->path_index = -1;
    g_self->path_speed = 0;
    return 1;
}

double gml_path_get_number(void) {
    return g_paths ? (double)g_paths->count : 0;
}

void gm82_path_step_instance(gm82_instance *inst) {
    if (!inst || !g_paths || inst->path_index < 0) return;
    if (inst->path_index >= g_paths->count) return;
    const gm82_path *path = &g_paths->items[inst->path_index];
    gm82_path_advance(path, &inst->x, &inst->y, &inst->path_position, inst->path_speed);
}

static gm82_timeline_list *g_timelines = NULL;
void gm82_timeline_bind(gm82_timeline_list *tls) { g_timelines = tls; }

double gml_timeline_start(double timeline_index, double position, double step, double direction) {
    (void)direction;
    if (!g_self || !g_timelines) return 0;
    int ti = (int)timeline_index;
    if (ti < 0 || ti >= g_timelines->count) return 0;
    g_self->timeline_index = ti;
    g_self->timeline_position = position;
    g_self->timeline_speed = step != 0 ? step : 1.0;
    g_self->timeline_running = 1;
    return 1;
}

double gml_timeline_stop(void) {
    if (!g_self) return 0;
    g_self->timeline_running = 0;
    g_self->timeline_index = -1;
    return 1;
}

void gm82_timeline_step_instance(gm82_runtime *rt, gm82_instance *inst) {
    if (!rt || !inst || !g_timelines || !inst->timeline_running) return;
    if (inst->timeline_index < 0 || inst->timeline_index >= g_timelines->count) return;
    gm82_timeline *tl = &g_timelines->items[inst->timeline_index];
    double prev = inst->timeline_position;
    inst->timeline_position += inst->timeline_speed;
    for (int m = 0; m < tl->count; m++) {
        double s = (double)tl->moments[m].step;
        if (s > prev && s <= inst->timeline_position) {
            if (tl->moments[m].code[0])
                gm82_gml_eval_stmt(rt, inst, tl->moments[m].code);
        }
    }
}

/* Tiny 8x8 bitmap font: each glyph = 8 bytes (rows), bit7 = leftmost pixel.
   Covers digits, uppercase, and a few symbols. Unknown -> box. */
static const unsigned char FONT8[96][8] = {
    /* 32 space */ {0,0,0,0,0,0,0,0},
    /* 33 ! */ {0x18,0x18,0x18,0x18,0x18,0x00,0x18,0x00},
    /* 34-47 minimal */ {0},
    {0},{0},{0},{0},{0},{0},{0},{0},{0},{0},{0},{0},{0},{0},
    /* 48 0 */ {0x3C,0x66,0x6E,0x76,0x66,0x66,0x3C,0x00},
    /* 49 1 */ {0x18,0x18,0x38,0x18,0x18,0x18,0x7E,0x00},
    /* 50 2 */ {0x3C,0x66,0x06,0x0C,0x30,0x60,0x7E,0x00},
    /* 51 3 */ {0x3C,0x66,0x06,0x1C,0x06,0x66,0x3C,0x00},
    /* 52 4 */ {0x0C,0x1C,0x3C,0x6C,0x7E,0x0C,0x0C,0x00},
    /* 53 5 */ {0x7E,0x60,0x7C,0x06,0x06,0x66,0x3C,0x00},
    /* 54 6 */ {0x3C,0x60,0x60,0x7C,0x66,0x66,0x3C,0x00},
    /* 55 7 */ {0x7E,0x06,0x0C,0x18,0x30,0x30,0x30,0x00},
    /* 56 8 */ {0x3C,0x66,0x66,0x3C,0x66,0x66,0x3C,0x00},
    /* 57 9 */ {0x3C,0x66,0x66,0x3E,0x06,0x0C,0x38,0x00},
    /* 58-64 */ {0},{0},{0},{0},{0},{0},{0},
    /* 65 A */ {0x18,0x3C,0x66,0x66,0x7E,0x66,0x66,0x00},
    /* 66 B */ {0x7C,0x66,0x66,0x7C,0x66,0x66,0x7C,0x00},
    /* 67 C */ {0x3C,0x66,0x60,0x60,0x60,0x66,0x3C,0x00},
    /* 68 D */ {0x78,0x6C,0x66,0x66,0x66,0x6C,0x78,0x00},
    /* 69 E */ {0x7E,0x60,0x60,0x7C,0x60,0x60,0x7E,0x00},
    /* 70 F */ {0x7E,0x60,0x60,0x7C,0x60,0x60,0x60,0x00},
    /* 71 G */ {0x3C,0x66,0x60,0x6E,0x66,0x66,0x3C,0x00},
    /* 72 H */ {0x66,0x66,0x66,0x7E,0x66,0x66,0x66,0x00},
    /* 73 I */ {0x3C,0x18,0x18,0x18,0x18,0x18,0x3C,0x00},
    /* 74 J */ {0x1E,0x0C,0x0C,0x0C,0x0C,0x6C,0x38,0x00},
    /* 75 K */ {0x66,0x6C,0x78,0x70,0x78,0x6C,0x66,0x00},
    /* 76 L */ {0x60,0x60,0x60,0x60,0x60,0x60,0x7E,0x00},
    /* 77 M */ {0x63,0x77,0x7F,0x6B,0x63,0x63,0x63,0x00},
    /* 78 N */ {0x66,0x76,0x7E,0x7E,0x6E,0x66,0x66,0x00},
    /* 79 O */ {0x3C,0x66,0x66,0x66,0x66,0x66,0x3C,0x00},
    /* 80 P */ {0x7C,0x66,0x66,0x7C,0x60,0x60,0x60,0x00},
    /* 81 Q */ {0x3C,0x66,0x66,0x66,0x66,0x3C,0x0E,0x00},
    /* 82 R */ {0x7C,0x66,0x66,0x7C,0x78,0x6C,0x66,0x00},
    /* 83 S */ {0x3C,0x66,0x60,0x3C,0x06,0x66,0x3C,0x00},
    /* 84 T */ {0x7E,0x18,0x18,0x18,0x18,0x18,0x18,0x00},
    /* 85 U */ {0x66,0x66,0x66,0x66,0x66,0x66,0x3C,0x00},
    /* 86 V */ {0x66,0x66,0x66,0x66,0x66,0x3C,0x18,0x00},
    /* 87 W */ {0x63,0x63,0x63,0x6B,0x7F,0x77,0x63,0x00},
    /* 88 X */ {0x66,0x66,0x3C,0x18,0x3C,0x66,0x66,0x00},
    /* 89 Y */ {0x66,0x66,0x66,0x3C,0x18,0x18,0x18,0x00},
    /* 90 Z */ {0x7E,0x06,0x0C,0x18,0x30,0x60,0x7E,0x00},
};

static void draw_glyph(int x, int y, char ch) {
    if (!g_draw_buf) return;
    unsigned char c = (unsigned char)ch;
    if (c >= 'a' && c <= 'z') c = (unsigned char)(c - 'a' + 'A');
    int idx = (int)c - 32;
    if (idx < 0 || idx >= 96) idx = 0;
    const unsigned char *g = FONT8[idx];
    /* if empty glyph for letter range, use a filled box */
    int empty = 1;
    for (int r = 0; r < 8; r++) if (g[r]) empty = 0;
    for (int row = 0; row < 8; row++) {
        unsigned char bits = empty && c > 32 ? 0x7E : g[row];
        if (empty && c > 32 && (row==0||row==7)) bits = 0x7E;
        if (empty && c > 32 && row>0 && row<7) bits = 0x42;
        for (int col = 0; col < 8; col++) {
            if (bits & (0x80 >> col)) put_px(x + col, y + row);
        }
    }
}

void gml_draw_text(double x, double y, const char *str) {
    if (!str || !g_draw_buf) return;
    int cx = (int)x, cy = (int)y;
    for (const char *p = str; *p; p++) {
        if (*p == '\n') { cy += 10; cx = (int)x; continue; }
        draw_glyph(cx, cy, *p);
        cx += 8;
    }
}

void gml_draw_text_color(double x, double y, const char *str,
                         double c1, double c2, double c3, double c4) {
    (void)c2; (void)c3; (void)c4;
    uint32_t old = g_draw_color;
    g_draw_color = (uint32_t)c1;
    gml_draw_text(x, y, str);
    g_draw_color = old;
}

#define GM82_SURFACE_MAX 32
typedef struct {
    int used;
    int32_t w, h;
    uint8_t *rgba;
} gm82_surface;

static gm82_surface g_surfaces[GM82_SURFACE_MAX];
static int g_surf_target = -1;
static uint8_t *g_draw_buf_saved = NULL;
static int32_t g_draw_w_saved = 0, g_draw_h_saved = 0;

double gml_surface_create(double w, double h) {
    int iw = (int)w, ih = (int)h;
    if (iw < 1 || ih < 1 || iw > 4096 || ih > 4096) return -1;
    for (int i = 0; i < GM82_SURFACE_MAX; i++) {
        if (g_surfaces[i].used) continue;
        size_t n = (size_t)iw * (size_t)ih * 4;
        uint8_t *p = (uint8_t *)calloc(n, 1);
        if (!p) return -1;
        g_surfaces[i].used = 1;
        g_surfaces[i].w = iw;
        g_surfaces[i].h = ih;
        g_surfaces[i].rgba = p;
        return (double)i;
    }
    return -1;
}

double gml_surface_free(double id) {
    int i = (int)id;
    if (i < 0 || i >= GM82_SURFACE_MAX || !g_surfaces[i].used) return 0;
    if (g_surf_target == i) gml_surface_reset_target();
    free(g_surfaces[i].rgba);
    memset(&g_surfaces[i], 0, sizeof(g_surfaces[i]));
    return 1;
}

double gml_surface_exists(double id) {
    int i = (int)id;
    return (i >= 0 && i < GM82_SURFACE_MAX && g_surfaces[i].used) ? 1.0 : 0.0;
}

double gml_surface_get_width(double id) {
    int i = (int)id;
    if (i < 0 || i >= GM82_SURFACE_MAX || !g_surfaces[i].used) return 0;
    return (double)g_surfaces[i].w;
}

double gml_surface_get_height(double id) {
    int i = (int)id;
    if (i < 0 || i >= GM82_SURFACE_MAX || !g_surfaces[i].used) return 0;
    return (double)g_surfaces[i].h;
}

double gml_surface_set_target(double id) {
    int i = (int)id;
    if (i < 0 || i >= GM82_SURFACE_MAX || !g_surfaces[i].used) return 0;
    if (g_surf_target < 0) {
        g_draw_buf_saved = g_draw_buf;
        g_draw_w_saved = g_draw_w;
        g_draw_h_saved = g_draw_h;
    }
    g_surf_target = i;
    g_draw_buf = g_surfaces[i].rgba;
    g_draw_w = g_surfaces[i].w;
    g_draw_h = g_surfaces[i].h;
    return 1;
}

double gml_surface_reset_target(void) {
    if (g_surf_target < 0) return 0;
    g_draw_buf = g_draw_buf_saved;
    g_draw_w = g_draw_w_saved;
    g_draw_h = g_draw_h_saved;
    g_surf_target = -1;
    return 1;
}

double gml_draw_surface(double id, double x, double y) {
    int i = (int)id;
    if (i < 0 || i >= GM82_SURFACE_MAX || !g_surfaces[i].used || !g_draw_buf) return 0;
    int dx0 = (int)x, dy0 = (int)y;
    int sw = g_surfaces[i].w, sh = g_surfaces[i].h;
    const uint8_t *src = g_surfaces[i].rgba;
    for (int sy = 0; sy < sh; sy++) {
        int dy = dy0 + sy;
        if (dy < 0 || dy >= g_draw_h) continue;
        for (int sx = 0; sx < sw; sx++) {
            int dx = dx0 + sx;
            if (dx < 0 || dx >= g_draw_w) continue;
            const uint8_t *s = src + ((size_t)sy * (size_t)sw + (size_t)sx) * 4;
            if (s[3] == 0) continue;
            uint8_t *d = g_draw_buf + ((size_t)dy * (size_t)g_draw_w + (size_t)dx) * 4;
            d[0]=s[0]; d[1]=s[1]; d[2]=s[2]; d[3]=255;
        }
    }
    return 1;
}

static gm82_particle_world *g_particles = NULL;
void gm82_particles_bind(gm82_particle_world *w) { g_particles = w; }

double gml_part_system_create(void) {
    if (!g_particles) return -1;
    return (double)gm82_part_system_create(g_particles);
}
double gml_part_system_destroy(double sys) {
    if (!g_particles) return 0;
    gm82_part_system_destroy(g_particles, (int)sys);
    return 1;
}
double gml_part_type_create(void) {
    if (!g_particles) return -1;
    return (double)gm82_part_type_create(g_particles);
}
double gml_part_particles_create(double sys, double x, double y, double type, double number) {
    if (!g_particles) return 0;
    gm82_part_particles_create(g_particles, (int)sys, x, y, (int)type, (int)number);
    return 1;
}
double gml_part_system_update(double sys) {
    if (!g_particles) return 0;
    gm82_part_system_update(g_particles, (int)sys);
    return 1;
}

#define GM82_DS_LIST_MAX 32
#define GM82_DS_LIST_CAP 256
typedef struct {
    int used;
    int size;
    double data[GM82_DS_LIST_CAP];
} gm82_ds_list;

static gm82_ds_list g_ds_lists[GM82_DS_LIST_MAX];

double gml_ds_list_create(void) {
    for (int i = 0; i < GM82_DS_LIST_MAX; i++) {
        if (g_ds_lists[i].used) continue;
        memset(&g_ds_lists[i], 0, sizeof(g_ds_lists[i]));
        g_ds_lists[i].used = 1;
        return (double)i;
    }
    return -1;
}

double gml_ds_list_destroy(double id) {
    int i = (int)id;
    if (i < 0 || i >= GM82_DS_LIST_MAX || !g_ds_lists[i].used) return 0;
    memset(&g_ds_lists[i], 0, sizeof(g_ds_lists[i]));
    return 1;
}

double gml_ds_list_add(double id, double value) {
    int i = (int)id;
    if (i < 0 || i >= GM82_DS_LIST_MAX || !g_ds_lists[i].used) return 0;
    if (g_ds_lists[i].size >= GM82_DS_LIST_CAP) return 0;
    g_ds_lists[i].data[g_ds_lists[i].size++] = value;
    return 1;
}

double gml_ds_list_find_value(double id, double pos) {
    int i = (int)id, p = (int)pos;
    if (i < 0 || i >= GM82_DS_LIST_MAX || !g_ds_lists[i].used) return 0;
    if (p < 0 || p >= g_ds_lists[i].size) return 0;
    return g_ds_lists[i].data[p];
}

double gml_ds_list_size(double id) {
    int i = (int)id;
    if (i < 0 || i >= GM82_DS_LIST_MAX || !g_ds_lists[i].used) return 0;
    return (double)g_ds_lists[i].size;
}

double gml_ds_list_clear(double id) {
    int i = (int)id;
    if (i < 0 || i >= GM82_DS_LIST_MAX || !g_ds_lists[i].used) return 0;
    g_ds_lists[i].size = 0;
    return 1;
}

double gml_ds_list_delete(double id, double pos) {
    int i = (int)id, p = (int)pos;
    if (i < 0 || i >= GM82_DS_LIST_MAX || !g_ds_lists[i].used) return 0;
    if (p < 0 || p >= g_ds_lists[i].size) return 0;
    for (int j = p; j < g_ds_lists[i].size - 1; j++)
        g_ds_lists[i].data[j] = g_ds_lists[i].data[j+1];
    g_ds_lists[i].size--;
    return 1;
}

double gml_ds_list_find_index(double id, double value) {
    int i = (int)id;
    if (i < 0 || i >= GM82_DS_LIST_MAX || !g_ds_lists[i].used) return -1;
    for (int j = 0; j < g_ds_lists[i].size; j++)
        if (g_ds_lists[i].data[j] == value) return (double)j;
    return -1;
}

double gml_ds_list_empty(double id) {
    return gml_ds_list_size(id) == 0 ? 1.0 : 0.0;
}

#define GM82_DS_MAP_MAX 16
#define GM82_DS_MAP_CAP 128
typedef struct {
    int used, size;
    double keys[GM82_DS_MAP_CAP];
    double vals[GM82_DS_MAP_CAP];
} gm82_ds_map;
static gm82_ds_map g_ds_maps[GM82_DS_MAP_MAX];

double gml_ds_map_create(void) {
    for (int i = 0; i < GM82_DS_MAP_MAX; i++) {
        if (g_ds_maps[i].used) continue;
        memset(&g_ds_maps[i], 0, sizeof(g_ds_maps[i]));
        g_ds_maps[i].used = 1;
        return (double)i;
    }
    return -1;
}
double gml_ds_map_destroy(double id) {
    int i = (int)id;
    if (i < 0 || i >= GM82_DS_MAP_MAX || !g_ds_maps[i].used) return 0;
    memset(&g_ds_maps[i], 0, sizeof(g_ds_maps[i]));
    return 1;
}
double gml_ds_map_add(double id, double key, double value) {
    int i = (int)id;
    if (i < 0 || i >= GM82_DS_MAP_MAX || !g_ds_maps[i].used) return 0;
    for (int j = 0; j < g_ds_maps[i].size; j++)
        if (g_ds_maps[i].keys[j] == key) { g_ds_maps[i].vals[j] = value; return 1; }
    if (g_ds_maps[i].size >= GM82_DS_MAP_CAP) return 0;
    g_ds_maps[i].keys[g_ds_maps[i].size] = key;
    g_ds_maps[i].vals[g_ds_maps[i].size] = value;
    g_ds_maps[i].size++;
    return 1;
}
double gml_ds_map_find_value(double id, double key) {
    int i = (int)id;
    if (i < 0 || i >= GM82_DS_MAP_MAX || !g_ds_maps[i].used) return 0;
    for (int j = 0; j < g_ds_maps[i].size; j++)
        if (g_ds_maps[i].keys[j] == key) return g_ds_maps[i].vals[j];
    return 0;
}
double gml_ds_map_exists(double id, double key) {
    int i = (int)id;
    if (i < 0 || i >= GM82_DS_MAP_MAX || !g_ds_maps[i].used) return 0;
    for (int j = 0; j < g_ds_maps[i].size; j++)
        if (g_ds_maps[i].keys[j] == key) return 1;
    return 0;
}
double gml_ds_map_size(double id) {
    int i = (int)id;
    if (i < 0 || i >= GM82_DS_MAP_MAX || !g_ds_maps[i].used) return 0;
    return (double)g_ds_maps[i].size;
}
double gml_ds_map_clear(double id) {
    int i = (int)id;
    if (i < 0 || i >= GM82_DS_MAP_MAX || !g_ds_maps[i].used) return 0;
    g_ds_maps[i].size = 0;
    return 1;
}
double gml_ds_map_delete(double id, double key) {
    int i = (int)id;
    if (i < 0 || i >= GM82_DS_MAP_MAX || !g_ds_maps[i].used) return 0;
    for (int j = 0; j < g_ds_maps[i].size; j++) {
        if (g_ds_maps[i].keys[j] != key) continue;
        for (int k = j; k < g_ds_maps[i].size - 1; k++) {
            g_ds_maps[i].keys[k] = g_ds_maps[i].keys[k+1];
            g_ds_maps[i].vals[k] = g_ds_maps[i].vals[k+1];
        }
        g_ds_maps[i].size--;
        return 1;
    }
    return 0;
}

#define GM82_DS_STACK_MAX 16
#define GM82_DS_STACK_CAP 128
typedef struct {
    int used, size;
    double data[GM82_DS_STACK_CAP];
} gm82_ds_stack;
static gm82_ds_stack g_ds_stacks[GM82_DS_STACK_MAX];

double gml_ds_stack_create(void) {
    for (int i = 0; i < GM82_DS_STACK_MAX; i++) {
        if (g_ds_stacks[i].used) continue;
        memset(&g_ds_stacks[i], 0, sizeof(g_ds_stacks[i]));
        g_ds_stacks[i].used = 1;
        return (double)i;
    }
    return -1;
}
double gml_ds_stack_destroy(double id) {
    int i = (int)id;
    if (i < 0 || i >= GM82_DS_STACK_MAX || !g_ds_stacks[i].used) return 0;
    memset(&g_ds_stacks[i], 0, sizeof(g_ds_stacks[i]));
    return 1;
}
double gml_ds_stack_push(double id, double value) {
    int i = (int)id;
    if (i < 0 || i >= GM82_DS_STACK_MAX || !g_ds_stacks[i].used) return 0;
    if (g_ds_stacks[i].size >= GM82_DS_STACK_CAP) return 0;
    g_ds_stacks[i].data[g_ds_stacks[i].size++] = value;
    return 1;
}
double gml_ds_stack_pop(double id) {
    int i = (int)id;
    if (i < 0 || i >= GM82_DS_STACK_MAX || !g_ds_stacks[i].used || g_ds_stacks[i].size <= 0) return 0;
    return g_ds_stacks[i].data[--g_ds_stacks[i].size];
}
double gml_ds_stack_top(double id) {
    int i = (int)id;
    if (i < 0 || i >= GM82_DS_STACK_MAX || !g_ds_stacks[i].used || g_ds_stacks[i].size <= 0) return 0;
    return g_ds_stacks[i].data[g_ds_stacks[i].size - 1];
}
double gml_ds_stack_size(double id) {
    int i = (int)id;
    if (i < 0 || i >= GM82_DS_STACK_MAX || !g_ds_stacks[i].used) return 0;
    return (double)g_ds_stacks[i].size;
}
double gml_ds_stack_empty(double id) {
    return gml_ds_stack_size(id) == 0 ? 1.0 : 0.0;
}

#define GM82_DS_QUEUE_MAX 16
#define GM82_DS_QUEUE_CAP 128
typedef struct {
    int used, head, tail, size;
    double data[GM82_DS_QUEUE_CAP];
} gm82_ds_queue;
static gm82_ds_queue g_ds_queues[GM82_DS_QUEUE_MAX];

double gml_ds_queue_create(void) {
    for (int i = 0; i < GM82_DS_QUEUE_MAX; i++) {
        if (g_ds_queues[i].used) continue;
        memset(&g_ds_queues[i], 0, sizeof(g_ds_queues[i]));
        g_ds_queues[i].used = 1;
        return (double)i;
    }
    return -1;
}
double gml_ds_queue_destroy(double id) {
    int i = (int)id;
    if (i < 0 || i >= GM82_DS_QUEUE_MAX || !g_ds_queues[i].used) return 0;
    memset(&g_ds_queues[i], 0, sizeof(g_ds_queues[i]));
    return 1;
}
double gml_ds_queue_enqueue(double id, double value) {
    int i = (int)id;
    if (i < 0 || i >= GM82_DS_QUEUE_MAX || !g_ds_queues[i].used) return 0;
    if (g_ds_queues[i].size >= GM82_DS_QUEUE_CAP) return 0;
    g_ds_queues[i].data[g_ds_queues[i].tail] = value;
    g_ds_queues[i].tail = (g_ds_queues[i].tail + 1) % GM82_DS_QUEUE_CAP;
    g_ds_queues[i].size++;
    return 1;
}
double gml_ds_queue_dequeue(double id) {
    int i = (int)id;
    if (i < 0 || i >= GM82_DS_QUEUE_MAX || !g_ds_queues[i].used || g_ds_queues[i].size <= 0) return 0;
    double v = g_ds_queues[i].data[g_ds_queues[i].head];
    g_ds_queues[i].head = (g_ds_queues[i].head + 1) % GM82_DS_QUEUE_CAP;
    g_ds_queues[i].size--;
    return v;
}
double gml_ds_queue_head(double id) {
    int i = (int)id;
    if (i < 0 || i >= GM82_DS_QUEUE_MAX || !g_ds_queues[i].used || g_ds_queues[i].size <= 0) return 0;
    return g_ds_queues[i].data[g_ds_queues[i].head];
}
double gml_ds_queue_tail(double id) {
    int i = (int)id;
    if (i < 0 || i >= GM82_DS_QUEUE_MAX || !g_ds_queues[i].used || g_ds_queues[i].size <= 0) return 0;
    int t = g_ds_queues[i].tail - 1;
    if (t < 0) t = GM82_DS_QUEUE_CAP - 1;
    return g_ds_queues[i].data[t];
}
double gml_ds_queue_size(double id) {
    int i = (int)id;
    if (i < 0 || i >= GM82_DS_QUEUE_MAX || !g_ds_queues[i].used) return 0;
    return (double)g_ds_queues[i].size;
}
double gml_ds_queue_empty(double id) {
    return gml_ds_queue_size(id) == 0 ? 1.0 : 0.0;
}
double gml_ds_queue_clear(double id) {
    int i = (int)id;
    if (i < 0 || i >= GM82_DS_QUEUE_MAX || !g_ds_queues[i].used) return 0;
    g_ds_queues[i].head = g_ds_queues[i].tail = g_ds_queues[i].size = 0;
    return 1;
}

#define GM82_DS_PRIO_MAX 16
#define GM82_DS_PRIO_CAP 128
typedef struct {
    int used, size;
    double vals[GM82_DS_PRIO_CAP];
    double prios[GM82_DS_PRIO_CAP];
} gm82_ds_prio;
static gm82_ds_prio g_ds_prios[GM82_DS_PRIO_MAX];

double gml_ds_priority_create(void) {
    for (int i = 0; i < GM82_DS_PRIO_MAX; i++) {
        if (g_ds_prios[i].used) continue;
        memset(&g_ds_prios[i], 0, sizeof(g_ds_prios[i]));
        g_ds_prios[i].used = 1;
        return (double)i;
    }
    return -1;
}
double gml_ds_priority_destroy(double id) {
    int i = (int)id;
    if (i < 0 || i >= GM82_DS_PRIO_MAX || !g_ds_prios[i].used) return 0;
    memset(&g_ds_prios[i], 0, sizeof(g_ds_prios[i]));
    return 1;
}
double gml_ds_priority_add(double id, double value, double priority) {
    int i = (int)id;
    if (i < 0 || i >= GM82_DS_PRIO_MAX || !g_ds_prios[i].used) return 0;
    if (g_ds_prios[i].size >= GM82_DS_PRIO_CAP) return 0;
    g_ds_prios[i].vals[g_ds_prios[i].size] = value;
    g_ds_prios[i].prios[g_ds_prios[i].size] = priority;
    g_ds_prios[i].size++;
    return 1;
}
static int prio_max_idx(gm82_ds_prio *p) {
    if (p->size <= 0) return -1;
    int best = 0;
    for (int j = 1; j < p->size; j++)
        if (p->prios[j] > p->prios[best]) best = j;
    return best;
}
double gml_ds_priority_find_max(double id) {
    int i = (int)id;
    if (i < 0 || i >= GM82_DS_PRIO_MAX || !g_ds_prios[i].used) return 0;
    int b = prio_max_idx(&g_ds_prios[i]);
    return b >= 0 ? g_ds_prios[i].vals[b] : 0;
}
double gml_ds_priority_delete_max(double id) {
    int i = (int)id;
    if (i < 0 || i >= GM82_DS_PRIO_MAX || !g_ds_prios[i].used) return 0;
    int b = prio_max_idx(&g_ds_prios[i]);
    if (b < 0) return 0;
    double v = g_ds_prios[i].vals[b];
    for (int j = b; j < g_ds_prios[i].size - 1; j++) {
        g_ds_prios[i].vals[j] = g_ds_prios[i].vals[j+1];
        g_ds_prios[i].prios[j] = g_ds_prios[i].prios[j+1];
    }
    g_ds_prios[i].size--;
    return v;
}
double gml_ds_priority_size(double id) {
    int i = (int)id;
    if (i < 0 || i >= GM82_DS_PRIO_MAX || !g_ds_prios[i].used) return 0;
    return (double)g_ds_prios[i].size;
}
double gml_ds_priority_empty(double id) {
    return gml_ds_priority_size(id) == 0 ? 1.0 : 0.0;
}

static gm82_mp_grid_world *g_mp = NULL;
static double g_mp_path_x[256], g_mp_path_y[256];
static int g_mp_path_n = 0;

void gm82_mp_grid_bind(gm82_mp_grid_world *w) { g_mp = w; }

double gml_mp_grid_create(double left, double top, double hcells, double vcells, double cellw, double cellh) {
    if (!g_mp) return -1;
    return (double)gm82_mp_grid_create(g_mp, (int)left, (int)top, (int)hcells, (int)vcells, (int)cellw, (int)cellh);
}
double gml_mp_grid_destroy(double id) {
    if (!g_mp) return 0;
    gm82_mp_grid_destroy(g_mp, (int)id);
    return 1;
}
double gml_mp_grid_clear_all(double id, double solid) {
    if (!g_mp) return 0;
    gm82_mp_grid_clear_all(g_mp, (int)id, solid != 0);
    return 1;
}
double gml_mp_grid_add_cell(double id, double cx, double cy, double solid) {
    if (!g_mp) return 0;
    gm82_mp_grid_add_cell(g_mp, (int)id, (int)cx, (int)cy, solid != 0);
    return 1;
}
double gml_mp_grid_path(double id, double xstart, double ystart, double xgoal, double ygoal, double allowdiag) {
    if (!g_mp) return 0;
    g_mp_path_n = gm82_mp_grid_path(g_mp, (int)id, xstart, ystart, xgoal, ygoal,
                                     g_mp_path_x, g_mp_path_y, 256, allowdiag != 0);
    return (double)g_mp_path_n;
}

/* buffer_type: 1=u8 2=s16 3=s32 4=f32 5=f64 6=bool */
#define GM82_BUF_MAX 16
#define GM82_BUF_CAP (64*1024)
typedef struct {
    int used;
    size_t size, pos, capacity;
    uint8_t *data;
} gm82_buffer;
static gm82_buffer g_bufs[GM82_BUF_MAX];

static size_t buf_type_size(int type) {
    switch (type) {
        case 1: return 1; case 2: return 2; case 3: return 4;
        case 4: return 4; case 5: return 8; case 6: return 1;
        default: return 1;
    }
}

double gml_buffer_sizeof(double type) { return (double)buf_type_size((int)type); }

double gml_buffer_create(double size, double type, double alignment) {
    (void)type; (void)alignment;
    size_t sz = (size_t)size;
    if (sz < 16) sz = 16;
    if (sz > GM82_BUF_CAP) sz = GM82_BUF_CAP;
    for (int i = 0; i < GM82_BUF_MAX; i++) {
        if (g_bufs[i].used) continue;
        uint8_t *p = (uint8_t *)calloc(sz, 1);
        if (!p) return -1;
        g_bufs[i].used = 1;
        g_bufs[i].data = p;
        g_bufs[i].capacity = sz;
        g_bufs[i].size = 0;
        g_bufs[i].pos = 0;
        return (double)i;
    }
    return -1;
}

double gml_buffer_delete(double id) {
    int i = (int)id;
    if (i < 0 || i >= GM82_BUF_MAX || !g_bufs[i].used) return 0;
    free(g_bufs[i].data);
    memset(&g_bufs[i], 0, sizeof(g_bufs[i]));
    return 1;
}

static int buf_write_at(gm82_buffer *b, size_t pos, int type, double value) {
    size_t ts = buf_type_size(type);
    if (pos + ts > b->capacity) return 0;
    uint8_t *p = b->data + pos;
    switch (type) {
        case 1: *p = (uint8_t)value; break;
        case 2: { int16_t v = (int16_t)value; memcpy(p, &v, 2); break; }
        case 3: { int32_t v = (int32_t)value; memcpy(p, &v, 4); break; }
        case 4: { float v = (float)value; memcpy(p, &v, 4); break; }
        case 5: { double v = value; memcpy(p, &v, 8); break; }
        case 6: *p = value != 0 ? 1 : 0; break;
        default: return 0;
    }
    if (pos + ts > b->size) b->size = pos + ts;
    return 1;
}

static double buf_read_at(gm82_buffer *b, size_t pos, int type) {
    size_t ts = buf_type_size(type);
    if (pos + ts > b->capacity) return 0;
    uint8_t *p = b->data + pos;
    switch (type) {
        case 1: return (double)(*p);
        case 2: { int16_t v; memcpy(&v, p, 2); return (double)v; }
        case 3: { int32_t v; memcpy(&v, p, 4); return (double)v; }
        case 4: { float v; memcpy(&v, p, 4); return (double)v; }
        case 5: { double v; memcpy(&v, p, 8); return v; }
        case 6: return *p ? 1.0 : 0.0;
        default: return 0;
    }
}

double gml_buffer_write(double id, double type, double value) {
    int i = (int)id;
    if (i < 0 || i >= GM82_BUF_MAX || !g_bufs[i].used) return 0;
    size_t ts = buf_type_size((int)type);
    if (!buf_write_at(&g_bufs[i], g_bufs[i].pos, (int)type, value)) return 0;
    g_bufs[i].pos += ts;
    return 1;
}

double gml_buffer_read(double id, double type) {
    int i = (int)id;
    if (i < 0 || i >= GM82_BUF_MAX || !g_bufs[i].used) return 0;
    size_t ts = buf_type_size((int)type);
    double v = buf_read_at(&g_bufs[i], g_bufs[i].pos, (int)type);
    g_bufs[i].pos += ts;
    return v;
}

double gml_buffer_poke(double id, double offset, double type, double value) {
    int i = (int)id;
    if (i < 0 || i >= GM82_BUF_MAX || !g_bufs[i].used) return 0;
    return buf_write_at(&g_bufs[i], (size_t)offset, (int)type, value) ? 1.0 : 0.0;
}

double gml_buffer_peek(double id, double offset, double type) {
    int i = (int)id;
    if (i < 0 || i >= GM82_BUF_MAX || !g_bufs[i].used) return 0;
    return buf_read_at(&g_bufs[i], (size_t)offset, (int)type);
}

double gml_buffer_seek(double id, double base, double offset) {
    int i = (int)id;
    if (i < 0 || i >= GM82_BUF_MAX || !g_bufs[i].used) return 0;
    size_t pos;
    if ((int)base == 0) pos = (size_t)offset; /* start */
    else if ((int)base == 1) pos = g_bufs[i].pos + (size_t)offset; /* relative */
    else pos = g_bufs[i].size + (size_t)offset; /* end */
    if (pos > g_bufs[i].capacity) pos = g_bufs[i].capacity;
    g_bufs[i].pos = pos;
    return 1;
}

double gml_buffer_tell(double id) {
    int i = (int)id;
    if (i < 0 || i >= GM82_BUF_MAX || !g_bufs[i].used) return 0;
    return (double)g_bufs[i].pos;
}

double gml_buffer_get_size(double id) {
    int i = (int)id;
    if (i < 0 || i >= GM82_BUF_MAX || !g_bufs[i].used) return 0;
    return (double)g_bufs[i].size;
}

#define GM82_INI_MAX_KEYS 64
typedef struct {
    char section[64];
    char key[64];
    double value;
} gm82_ini_entry;

static gm82_ini_entry g_ini[GM82_INI_MAX_KEYS];
static int g_ini_count = 0;
static int g_ini_open_flag = 0;
static char g_ini_path[256];

double gml_ini_open(const char *path) {
    if (!path) return 0;
    g_ini_count = 0;
    g_ini_open_flag = 1;
    strncpy(g_ini_path, path, sizeof(g_ini_path)-1);
    FILE *f = fopen(path, "r");
    if (!f) return 1; /* new file ok */
    char line[256], cur_sec[64] = "";
    while (fgets(line, sizeof(line), f)) {
        char *p = line;
        while (*p == ' ' || *p == '\t') p++;
        if (*p == '#' || *p == ';' || *p == '\n' || *p == '\0') continue;
        if (*p == '[') {
            char *e = strchr(p, ']');
            if (e) {
                *e = 0;
                strncpy(cur_sec, p+1, sizeof(cur_sec)-1);
            }
            continue;
        }
        char *eq = strchr(p, '=');
        if (!eq) continue;
        *eq = 0;
        char *key = p;
        char *val = eq + 1;
        while (*val == ' ') val++;
        char *nl = strchr(val, '\n'); if (nl) *nl = 0;
        char *sp = key + strlen(key) - 1;
        while (sp > key && (*sp == ' ' || *sp == '\t')) { *sp = 0; sp--; }
        if (g_ini_count >= GM82_INI_MAX_KEYS) break;
        strncpy(g_ini[g_ini_count].section, cur_sec, 63);
        strncpy(g_ini[g_ini_count].key, key, 63);
        g_ini[g_ini_count].value = strtod(val, NULL);
        g_ini_count++;
    }
    fclose(f);
    return 1;
}

double gml_ini_close(void) {
    if (!g_ini_open_flag) return 0;
    FILE *f = fopen(g_ini_path, "w");
    if (f) {
        char last_sec[64] = "\xff";
        for (int i = 0; i < g_ini_count; i++) {
            if (strcmp(g_ini[i].section, last_sec) != 0) {
                fprintf(f, "[%s]\n", g_ini[i].section);
                strncpy(last_sec, g_ini[i].section, 63);
            }
            fprintf(f, "%s=%.10g\n", g_ini[i].key, g_ini[i].value);
        }
        fclose(f);
    }
    g_ini_open_flag = 0;
    g_ini_count = 0;
    return 1;
}

static int ini_find(const char *section, const char *key) {
    const char *sec = section ? section : "";
    for (int i = 0; i < g_ini_count; i++)
        if (strcmp(g_ini[i].section, sec) == 0 && strcmp(g_ini[i].key, key) == 0)
            return i;
    return -1;
}

double gml_ini_write_real(const char *section, const char *key, double value) {
    if (!g_ini_open_flag || !key) return 0;
    int idx = ini_find(section, key);
    if (idx >= 0) { g_ini[idx].value = value; return 1; }
    if (g_ini_count >= GM82_INI_MAX_KEYS) return 0;
    strncpy(g_ini[g_ini_count].section, section ? section : "", 63);
    strncpy(g_ini[g_ini_count].key, key, 63);
    g_ini[g_ini_count].value = value;
    g_ini_count++;
    return 1;
}

double gml_ini_read_real(const char *section, const char *key, double def) {
    int idx = ini_find(section, key);
    return idx >= 0 ? g_ini[idx].value : def;
}

double gml_ini_key_exists(const char *section, const char *key) {
    return ini_find(section, key) >= 0 ? 1.0 : 0.0;
}

#define GM82_FILE_MAX 8
typedef struct {
    int used;
    FILE *fp;
    int is_write;
} gm82_text_file;
static gm82_text_file g_files[GM82_FILE_MAX];

double gml_file_text_open_read(const char *path) {
    if (!path) return -1;
    for (int i = 0; i < GM82_FILE_MAX; i++) {
        if (g_files[i].used) continue;
        FILE *f = fopen(path, "r");
        if (!f) return -1;
        g_files[i].used = 1; g_files[i].fp = f; g_files[i].is_write = 0;
        return (double)i;
    }
    return -1;
}
double gml_file_text_open_write(const char *path) {
    if (!path) return -1;
    for (int i = 0; i < GM82_FILE_MAX; i++) {
        if (g_files[i].used) continue;
        FILE *f = fopen(path, "w");
        if (!f) return -1;
        g_files[i].used = 1; g_files[i].fp = f; g_files[i].is_write = 1;
        return (double)i;
    }
    return -1;
}
double gml_file_text_open_append(const char *path) {
    if (!path) return -1;
    for (int i = 0; i < GM82_FILE_MAX; i++) {
        if (g_files[i].used) continue;
        FILE *f = fopen(path, "a");
        if (!f) return -1;
        g_files[i].used = 1; g_files[i].fp = f; g_files[i].is_write = 1;
        return (double)i;
    }
    return -1;
}
double gml_file_text_close(double id) {
    int i = (int)id;
    if (i < 0 || i >= GM82_FILE_MAX || !g_files[i].used) return 0;
    fclose(g_files[i].fp);
    memset(&g_files[i], 0, sizeof(g_files[i]));
    return 1;
}
double gml_file_text_eof(double id) {
    int i = (int)id;
    if (i < 0 || i >= GM82_FILE_MAX || !g_files[i].used) return 1;
    return feof(g_files[i].fp) ? 1.0 : 0.0;
}
double gml_file_text_write_string(double id, const char *str) {
    int i = (int)id;
    if (i < 0 || i >= GM82_FILE_MAX || !g_files[i].used || !g_files[i].is_write) return 0;
    fputs(str ? str : "", g_files[i].fp);
    return 1;
}
double gml_file_text_write_real(double id, double value) {
    int i = (int)id;
    if (i < 0 || i >= GM82_FILE_MAX || !g_files[i].used || !g_files[i].is_write) return 0;
    fprintf(g_files[i].fp, "%.10g", value);
    return 1;
}
double gml_file_text_writeln(double id) {
    int i = (int)id;
    if (i < 0 || i >= GM82_FILE_MAX || !g_files[i].used || !g_files[i].is_write) return 0;
    fputc('\n', g_files[i].fp);
    return 1;
}
double gml_file_text_read_real(double id) {
    int i = (int)id;
    if (i < 0 || i >= GM82_FILE_MAX || !g_files[i].used || g_files[i].is_write) return 0;
    double v = 0;
    if (fscanf(g_files[i].fp, "%lf", &v) != 1) return 0;
    return v;
}
double gml_file_exists(const char *path) {
    if (!path) return 0;
    FILE *f = fopen(path, "r");
    if (!f) return 0;
    fclose(f);
    return 1;
}
double gml_file_delete(const char *path) {
    if (!path) return 0;
    return remove(path) == 0 ? 1.0 : 0.0;
}

/* GM datetime is days since 1899-12-30; we use unix-ish simplified: seconds as real */
double gml_date_current_datetime(void) {
    return (double)time(NULL);
}
double gml_date_get_year(double datetime) {
    time_t t = (time_t)datetime;
    struct tm *tm = localtime(&t);
    return tm ? tm->tm_year + 1900 : 0;
}
double gml_date_get_month(double datetime) {
    time_t t = (time_t)datetime;
    struct tm *tm = localtime(&t);
    return tm ? tm->tm_mon + 1 : 0;
}
double gml_date_get_day(double datetime) {
    time_t t = (time_t)datetime;
    struct tm *tm = localtime(&t);
    return tm ? tm->tm_mday : 0;
}
double gml_date_get_hour(double datetime) {
    time_t t = (time_t)datetime;
    struct tm *tm = localtime(&t);
    return tm ? tm->tm_hour : 0;
}
double gml_date_get_minute(double datetime) {
    time_t t = (time_t)datetime;
    struct tm *tm = localtime(&t);
    return tm ? tm->tm_min : 0;
}
double gml_date_get_second(double datetime) {
    time_t t = (time_t)datetime;
    struct tm *tm = localtime(&t);
    return tm ? tm->tm_sec : 0;
}
double gml_current_time(void) { return gml_date_current_datetime(); }
double gml_current_year(void) { return gml_date_get_year(gml_current_time()); }
double gml_current_month(void) { return gml_date_get_month(gml_current_time()); }
double gml_current_day(void) { return gml_date_get_day(gml_current_time()); }

double gml_random_range(double x1, double x2) {
    double lo = x1 < x2 ? x1 : x2;
    double hi = x1 < x2 ? x2 : x1;
    return lo + gml_random(hi - lo);
}
double gml_irandom_range(double x1, double x2) {
    int lo = (int)(x1 < x2 ? x1 : x2);
    int hi = (int)(x1 < x2 ? x2 : x1);
    if (hi < lo) return lo;
    return (double)(lo + rand() % (hi - lo + 1));
}
double gml_choose(double a, double b) {
    return (rand() & 1) ? a : b;
}

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif


#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

double gml_lengthdir_x(double len, double dir) {
    return len * cos(dir * M_PI / 180.0);
}
double gml_lengthdir_y(double len, double dir) {
    return -len * sin(dir * M_PI / 180.0);
}
double gml_deg_to_rad(double deg) { return deg * M_PI / 180.0; }
double gml_rad_to_deg(double rad) { return rad * 180.0 / M_PI; }
double gml_angle_difference(double dest, double src) {
    double d = fmod(dest - src + 180.0, 360.0) - 180.0;
    if (d < -180.0) d += 360.0;
    return d;
}


static int g_win_w = 640, g_win_h = 480;
static char g_win_caption[128] = "Game Runner";

double gml_display_get_width(void) { return (double)g_win_w; }
double gml_display_get_height(void) { return (double)g_win_h; }
double gml_window_get_width(void) { return (double)g_win_w; }
double gml_window_get_height(void) { return (double)g_win_h; }

void gm82_window_set_size(int w, int h) {
    if (w > 0) g_win_w = w;
    if (h > 0) g_win_h = h;
}

double gml_window_set_caption(const char *caption) {
    if (!caption) return 0;
    strncpy(g_win_caption, caption, sizeof(g_win_caption)-1);
    return 1;
}
const char *gml_window_get_caption(void) { return g_win_caption; }

double gml_room_goto_next(void) {
    if (!g_rt) return 0;
    return gm82_runtime_goto_room(g_rt, g_rt->current_room + 1) ? 1.0 : 0.0;
}
double gml_room_goto_previous(void) {
    if (!g_rt) return 0;
    return gm82_runtime_goto_room(g_rt, g_rt->current_room - 1) ? 1.0 : 0.0;
}
double gml_game_restart(void) {
    g_game_ended = 0;
    if (!g_rt) return 0;
    if (g_rt) g_rt->running = 1;
    return gm82_runtime_goto_room(g_rt, 0) ? 1.0 : 0.0;
}
double gml_game_has_ended(void) { return g_game_ended ? 1.0 : 0.0; }

static gm82_script_list *g_scripts = NULL;
void gm82_scripts_bind(gm82_script_list *scripts) { g_scripts = scripts; }

double gml_script_exists(double script_index) {
    if (!g_scripts) return 0;
    int i = (int)script_index;
    return (i >= 0 && i < g_scripts->count) ? 1.0 : 0.0;
}

double gml_script_execute(double script_index) {
    if (!g_scripts || !g_rt) return 0;
    int i = (int)script_index;
    if (i < 0 || i >= g_scripts->count) return 0;
    const char *code = g_scripts->items[i].code;
    if (!code || !code[0]) return 0;
    return (double)gm82_gml_eval_block(g_rt, g_self, code);
}

static char g_alarm_scripts[12][128];
static int g_alarm_script_set[12];

double gml_alarm_set(double index, double steps) {
    if (!g_self) return 0;
    int i = (int)index;
    if (i < 0 || i >= 12) return 0;
    g_self->alarms[i] = (int32_t)steps;
    return 1;
}

double gml_alarm_get(double index) {
    if (!g_self) return -1;
    int i = (int)index;
    if (i < 0 || i >= 12) return -1;
    return (double)g_self->alarms[i];
}

double gml_alarm_set_script(double index, const char *code) {
    int i = (int)index;
    if (i < 0 || i >= 12) return 0;
    if (!code) { g_alarm_script_set[i] = 0; g_alarm_scripts[i][0] = 0; return 1; }
    strncpy(g_alarm_scripts[i], code, 127);
    g_alarm_scripts[i][127] = 0;
    g_alarm_script_set[i] = 1;
    return 1;
}

void gm82_alarm_fire_scripts(gm82_runtime *rt, gm82_instance *inst, int alarm_index) {
    if (!rt || !inst || alarm_index < 0 || alarm_index >= 12) return;
    if (!g_alarm_script_set[alarm_index] || !g_alarm_scripts[alarm_index][0]) return;
    gm82_gml_eval_stmt(rt, inst, g_alarm_scripts[alarm_index]);
}
