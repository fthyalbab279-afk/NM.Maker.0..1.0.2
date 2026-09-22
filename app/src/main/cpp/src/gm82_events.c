#ifndef _GNU_SOURCE
#define _GNU_SOURCE
#endif
#include "gm82_events.h"
#include "gm82_gml_builtins.h"
#include "gm82_input.h"
#include "gm82_actions.h"
#include "gm82_gml_eval.h"
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <zlib.h>

/* ---- Default behaviors (name-based until action lists fully parsed) ---- */

static void beh_player_create(gm82_runtime *rt, gm82_instance *self) {
    (void)rt;
    self->image_speed = 0.2;
}

static void beh_player_step(gm82_runtime *rt, gm82_instance *self) {
    gm82_gml_set_runtime(rt);
    gm82_gml_set_self(self);

    /* horizontal input (vk_left=37 vk_right=39 or A/D) */
    {
        double h = 0;
        if (gml_keyboard_check(37) || gml_keyboard_check(65)) h -= 3.0;
        if (gml_keyboard_check(39) || gml_keyboard_check(68)) h += 3.0;
        gml_set_hspeed(h);
        /* jump */
        if ((gml_keyboard_check(38) || gml_keyboard_check(32)) && gml_get_vspeed() == 0)
            gml_set_vspeed(-8.0);
    }

    /* gravity */
    gml_set_vspeed(gml_get_vspeed() + 0.5);

    /* collide with any solid below */
    if (gml_place_meeting(gml_get_x(), gml_get_y() + 1, -1)) {
        /* check only solids manually */
        int32_t sw = 16, sh = 16;
        if (rt->sprites && self->sprite_index >= 0 && self->sprite_index < rt->sprites->count) {
            sw = rt->sprites->frames[self->sprite_index].width;
            sh = rt->sprites->frames[self->sprite_index].height;
        }
        for (int i = 0; i < rt->instance_count; i++) {
            gm82_instance *o = &rt->instances[i];
            if (!o->alive || !o->solid || o == self) continue;
            int32_t ow = 16, oh = 16;
            if (rt->sprites && o->sprite_index >= 0 && o->sprite_index < rt->sprites->count) {
                ow = rt->sprites->frames[o->sprite_index].width;
                oh = rt->sprites->frames[o->sprite_index].height;
            }
            if (self->x < o->x + ow && self->x + sw > o->x &&
                self->y + 1 < o->y + oh && self->y + sh + 1 > o->y) {
                if (self->vspeed > 0) {
                    self->y = o->y - sh;
                    self->vspeed = 0;
                }
            }
        }
    }

    /* keep inside room */
    if (self->y > rt->room_height) {
        self->y = 0;
        self->vspeed = 0;
    }
    if (self->x < 0) self->x = 0;
    if (self->x > rt->room_width - 16) self->x = (double)(rt->room_width - 16);
}

static void beh_enemy_step(gm82_runtime *rt, gm82_instance *self) {
    gm82_gml_set_runtime(rt);
    gm82_gml_set_self(self);
    /* simple patrol */
    if (self->hspeed == 0) self->hspeed = 1.0;
    if (self->x < 0 || self->x > rt->room_width - 16)
        self->hspeed = -self->hspeed;
    /* gravity light */
    self->vspeed += 0.3;
    if (gml_place_meeting(self->x, self->y + 1, -1)) {
        for (int i = 0; i < rt->instance_count; i++) {
            gm82_instance *o = &rt->instances[i];
            if (!o->alive || !o->solid || o == self) continue;
            int32_t sh = 16, oh = 16;
            if (rt->sprites && self->sprite_index >= 0 && self->sprite_index < rt->sprites->count)
                sh = rt->sprites->frames[self->sprite_index].height;
            if (rt->sprites && o->sprite_index >= 0 && o->sprite_index < rt->sprites->count)
                oh = rt->sprites->frames[o->sprite_index].height;
            if (self->vspeed > 0 && self->y + sh >= o->y && self->y < o->y + oh &&
                self->x + 8 > o->x && self->x < o->x + 16) {
                self->y = o->y - sh;
                self->vspeed = 0;
            }
        }
    }
}

static void beh_solid_create(gm82_runtime *rt, gm82_instance *self) {
    (void)rt;
    self->solid = 1;
}

static const gm82_behavior g_behaviors[] = {
    { "obj_minimario", beh_player_create, beh_player_step },
    { "obj_mario",     beh_player_create, beh_player_step },
    { "obj_enemigo",   NULL,              beh_enemy_step },
    { "obj_bloque",    beh_solid_create,  NULL },
    { "obj_castillo",  beh_solid_create,  NULL },
    { NULL, NULL, NULL }
};

void gm82_events_register_defaults(void) {
    /* static table – nothing to do at runtime */
}

const gm82_behavior *gm82_events_find_behavior(gm82_runtime *rt, int32_t object_index) {
    if (!rt || !rt->objects || object_index < 0 || object_index >= rt->objects->count)
        return NULL;
    const char *name = rt->objects->items[object_index].name;
    for (int i = 0; g_behaviors[i].object_name_prefix; i++) {
        if (strncmp(name, g_behaviors[i].object_name_prefix,
                    strlen(g_behaviors[i].object_name_prefix)) == 0)
            return &g_behaviors[i];
    }
    return NULL;
}


#define GM82_GML_BIND_MAX 16
typedef struct {
    char prefix[64];
    const char *code;
} gm82_gml_bind;

static gm82_gml_bind g_gml_step[GM82_GML_BIND_MAX];
static gm82_gml_bind g_gml_create[GM82_GML_BIND_MAX];
static int g_gml_step_n;
static int g_gml_create_n;

void gm82_events_clear_gml_bindings(void) {
    g_gml_step_n = 0;
    g_gml_create_n = 0;
    memset(g_gml_step, 0, sizeof(g_gml_step));
    memset(g_gml_create, 0, sizeof(g_gml_create));
}

bool gm82_events_bind_gml_step(const char *object_name_prefix, const char *code) {
    if (!object_name_prefix || !code || g_gml_step_n >= GM82_GML_BIND_MAX) return false;
    gm82_gml_bind *b = &g_gml_step[g_gml_step_n++];
    strncpy(b->prefix, object_name_prefix, sizeof(b->prefix)-1);
    b->code = code;
    return true;
}

bool gm82_events_bind_gml_create(const char *object_name_prefix, const char *code) {
    if (!object_name_prefix || !code || g_gml_create_n >= GM82_GML_BIND_MAX) return false;
    gm82_gml_bind *b = &g_gml_create[g_gml_create_n++];
    strncpy(b->prefix, object_name_prefix, sizeof(b->prefix)-1);
    b->code = code;
    return true;
}

static const char *find_gml(gm82_gml_bind *arr, int n, gm82_runtime *rt, int32_t object_index) {
    if (!rt || !rt->objects || object_index < 0 || object_index >= rt->objects->count) return NULL;
    const char *name = rt->objects->items[object_index].name;
    for (int i = 0; i < n; i++) {
        size_t L = strlen(arr[i].prefix);
        if (L > 0 && strncmp(name, arr[i].prefix, L) == 0) return arr[i].code;
    }
    return NULL;
}


static void fire_object_create_actions(gm82_runtime *rt, gm82_instance *inst) {
    if (!rt || !inst || !rt->objects) return;
    int32_t oi = inst->object_index;
    if (oi < 0 || oi >= rt->objects->count) return;
    const gm82_decoded_object *obj = &rt->objects->items[oi];
    for (int e = 0; e < obj->event_count && e < GM82_OBJ_EVENT_MAX; e++) {
        const gm82_decoded_event *ev = &obj->events[e];
        if (ev->main_type != 0) continue; /* Create only */
        if (ev->action_name0[0]) {
            gm82_action_execute_named(rt, inst, ev->action_name0);
            if (strcmp(ev->action_name0, "action_sprite_set") == 0 && ev->action_arg0 >= 0)
                inst->sprite_index = ev->action_arg0;
            else if (strcmp(ev->action_name0, "action_sprite_set") == 0 &&
                     rt->objects && inst->object_index >= 0 &&
                     inst->object_index < rt->objects->count)
                inst->sprite_index = rt->objects->items[inst->object_index].sprite_index;
            if (strcmp(ev->action_name0, "action_set_hspeed") == 0 && ev->action_arg0 != -1)
                inst->hspeed = (double)ev->action_arg0;
            if (strcmp(ev->action_name0, "action_set_vspeed") == 0 && ev->action_arg0 != -1)
                inst->vspeed = (double)ev->action_arg0;
            if (strcmp(ev->action_name0, "action_kill_object") == 0)
                gm82_action_execute_named(rt, inst, "action_kill_object");
        }
    }
}


void gm82_events_fire_collision(gm82_runtime *rt, gm82_instance *inst, gm82_instance *other) {
    if (!rt || !inst || !rt->objects) return;
    int32_t oi = inst->object_index;
    if (oi < 0 || oi >= rt->objects->count) return;
    const gm82_decoded_object *obj = &rt->objects->items[oi];
    (void)other;
    for (int e = 0; e < obj->event_count && e < GM82_OBJ_EVENT_MAX; e++) {
        const gm82_decoded_event *ev = &obj->events[e];
        if (ev->main_type != 4) continue; /* Collision main event */
        if (!ev->action_name0[0]) continue;
        gm82_action_execute_named(rt, inst, ev->action_name0);
    }
}

void gm82_events_fire_create_all(gm82_runtime *rt) {
    if (!rt) return;
    gm82_gml_set_runtime(rt);
    for (int i = 0; i < rt->instance_count; i++) {
        gm82_instance *inst = &rt->instances[i];
        if (!inst->alive) continue;
        const char *gcode = find_gml(g_gml_create, g_gml_create_n, rt, inst->object_index);
        if (gcode) {
            gm82_gml_set_self(inst);
            gm82_gml_eval_block(rt, inst, gcode);
        } else {
            const gm82_behavior *b = gm82_events_find_behavior(rt, inst->object_index);
            if (b && b->on_create) {
                gm82_gml_set_self(inst);
                b->on_create(rt, inst);
            }
        }
        fire_object_create_actions(rt, inst);
        if (rt->actions)
            gm82_actions_fire_create(rt, inst, rt->actions);
    }
}


static void fire_object_step_actions(gm82_runtime *rt, gm82_instance *inst) {
    if (!rt || !inst || !rt->objects) return;
    int32_t oi = inst->object_index;
    if (oi < 0 || oi >= rt->objects->count) return;
    const gm82_decoded_object *obj = &rt->objects->items[oi];
    for (int e = 0; e < obj->event_count && e < GM82_OBJ_EVENT_MAX; e++) {
        const gm82_decoded_event *ev = &obj->events[e];
        /* Step (3) actions; also run any decoded GML snippet each step
         * when it looks like movement code (keyboard/place_free). */
        if (ev->main_type == 3 && ev->action_name0[0]) {
            gm82_action_execute_named(rt, inst, ev->action_name0);
            if (strcmp(ev->action_name0, "action_set_hspeed") == 0 && ev->action_arg0 != -1)
                inst->hspeed = (double)ev->action_arg0;
            if (strcmp(ev->action_name0, "action_set_vspeed") == 0 && ev->action_arg0 != -1)
                inst->vspeed = (double)ev->action_arg0;
        }
        if (ev->code_snippet && ev->code_snippet[0] && (ev->main_type == 3 ||
            strstr(ev->code_snippet, "keyboard") || strstr(ev->code_snippet, "place_free"))) {
            gm82_gml_eval_block(rt, inst, ev->code_snippet);
        }
    }
}

void gm82_events_fire_step_all(gm82_runtime *rt) {
    if (!rt) return;
    gm82_gml_set_runtime(rt);
    for (int i = 0; i < rt->instance_count; i++) {
        gm82_instance *inst = &rt->instances[i];
        if (!inst->alive) continue;
        const char *gcode = find_gml(g_gml_step, g_gml_step_n, rt, inst->object_index);
        if (gcode) {
            gm82_gml_set_self(inst);
            gm82_gml_eval_block(rt, inst, gcode);
        } else {
            const gm82_behavior *b = gm82_events_find_behavior(rt, inst->object_index);
            if (b && b->on_step) {
                gm82_gml_set_self(inst);
                b->on_step(rt, inst);
            }
        }
        fire_object_step_actions(rt, inst);
        if (rt->actions)
            gm82_actions_fire_step(rt, inst, rt->actions);
    }
}

static int32_t ev_rd_i32(const uint8_t *p) {
    return (int32_t)(p[0]|p[1]<<8|p[2]<<16|p[3]<<24);
}

static uint8_t *ev_inflate(const uint8_t *src, size_t n, size_t *ol) {
    *ol = 0;
    z_stream strm; memset(&strm, 0, sizeof(strm));
    if (inflateInit2(&strm, 15) != Z_OK) return NULL;
    size_t cap = n * 8 + 256;
    uint8_t *dst = (uint8_t *)malloc(cap);
    if (!dst) { inflateEnd(&strm); return NULL; }
    strm.next_in = (Bytef *)src; strm.avail_in = (uInt)n;
    strm.next_out = dst; strm.avail_out = (uInt)cap;
    int ret;
    while ((ret = inflate(&strm, Z_NO_FLUSH)) == Z_OK) {
        if (strm.avail_out == 0) {
            size_t used = cap; cap *= 2;
            uint8_t *nd = (uint8_t *)realloc(dst, cap);
            if (!nd) { free(dst); inflateEnd(&strm); return NULL; }
            dst = nd; strm.next_out = dst + used; strm.avail_out = (uInt)(cap - used);
        }
    }
    if (ret != Z_STREAM_END) { free(dst); inflateEnd(&strm); return NULL; }
    *ol = strm.total_out; inflateEnd(&strm); return dst;
}

/* stored copies so pointers stay valid after scan */
static char g_autobind_code_store[16][1024];
static int g_autobind_store_n;

int gm82_events_autobind_gml_from_gmk(const uint8_t *data, size_t size) {
    if (!data || size < 12) return -1;
    int bound = 0;
    g_autobind_store_n = 0;
    for (size_t i = 12; i + 2 < size; i++) {
        if (!(data[i]==0x78 && (data[i+1]==0x9c||data[i+1]==0xda||data[i+1]==0x01||data[i+1]==0x5e)))
            continue;
        size_t ol = 0;
        uint8_t *d = ev_inflate(data + i, size - i, &ol);
        i += 16;
        if (!d || ol < 40) { free(d); continue; }
        if (ev_rd_i32(d) != 1) { free(d); continue; }
        int32_t slen = ev_rd_i32(d + 4);
        if (slen < 3 || slen > 48 || 8 + (size_t)slen > ol) { free(d); continue; }
        char name[64];
        memcpy(name, d + 8, (size_t)slen); name[slen] = 0;
        int ok = 1;
        for (int k = 0; k < slen; k++) if (name[k] < 32 || name[k] > 126) ok = 0;
        if (!ok || strncmp(name, "obj", 3) != 0) { free(d); continue; }

        /* pick longest GML-like string in blob */
        const char *best = NULL; int best_n = 0;
        for (size_t j = 0; j + 8 < ol; j++) {
            int32_t n = ev_rd_i32(d + j);
            if (n < 16 || n > 1000 || j + 4 + (size_t)n > ol) continue;
            const uint8_t *s = d + j + 4;
            int good = 1;
            for (int k = 0; k < n; k++) {
                unsigned char c = s[k];
                if (!(c >= 32 && c < 127) && c != 9 && c != 10 && c != 13) { good = 0; break; }
            }
            if (!good) continue;
            int score = 0;
            if (memmem(s, (size_t)n, "keyboard_", 9)) score += 2;
            if (memmem(s, (size_t)n, "if ", 3)) score += 1;
            if (memmem(s, (size_t)n, "x", 1) && memmem(s, (size_t)n, "=", 1)) score += 1;
            if (memmem(s, (size_t)n, "image_", 6)) score += 1;
            if (score < 2) continue;
            if (n > best_n) { best_n = n; best = (const char *)s; }
        }
        if (best && g_autobind_store_n < 16) {
            int ncopy = best_n < 1023 ? best_n : 1023;
            memcpy(g_autobind_code_store[g_autobind_store_n], best, (size_t)ncopy);
            g_autobind_code_store[g_autobind_store_n][ncopy] = 0;
            if (gm82_events_bind_gml_create(name, g_autobind_code_store[g_autobind_store_n])) {
                g_autobind_store_n++;
                bound++;
            }
        }
        free(d);
    }
    return bound;
}
