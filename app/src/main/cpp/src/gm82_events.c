#define _POSIX_C_SOURCE 200809L
#include "gm82_events.h"
#include "gm82_gml_builtins.h"
#include "gm82_input.h"
#include "gm82_actions.h"
#include <string.h>
#include <stdio.h>

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

void gm82_events_fire_create_all(gm82_runtime *rt) {
    if (!rt) return;
    gm82_gml_set_runtime(rt);
    for (int i = 0; i < rt->instance_count; i++) {
        gm82_instance *inst = &rt->instances[i];
        if (!inst->alive) continue;
        const gm82_behavior *b = gm82_events_find_behavior(rt, inst->object_index);
        if (b && b->on_create) {
            gm82_gml_set_self(inst);
            b->on_create(rt, inst);
        }
        if (rt->actions)
            gm82_actions_fire_create(rt, inst, rt->actions);
    }
}

void gm82_events_fire_step_all(gm82_runtime *rt) {
    if (!rt) return;
    gm82_gml_set_runtime(rt);
    for (int i = 0; i < rt->instance_count; i++) {
        gm82_instance *inst = &rt->instances[i];
        if (!inst->alive) continue;
        const gm82_behavior *b = gm82_events_find_behavior(rt, inst->object_index);
        if (b && b->on_step) {
            gm82_gml_set_self(inst);
            b->on_step(rt, inst);
        }
        if (rt->actions) {
            gm82_actions_fire_event(rt, inst, GM82_EV_STEP, GM82_STEP_NORMAL, rt->actions);
        }
    }
}

void gm82_events_fire_event_all(gm82_runtime *rt, int32_t event_type, int32_t event_numb) {
    if (!rt) return;
    gm82_gml_set_runtime(rt);
    for (int i = 0; i < rt->instance_count; i++) {
        gm82_instance *inst = &rt->instances[i];
        if (!inst->alive) continue;
        gm82_gml_set_self(inst);
        if (rt->actions) {
            gm82_actions_fire_event(rt, inst, event_type, event_numb, rt->actions);
        }
    }
}

void gm82_events_fire_collision(gm82_runtime *rt, gm82_instance *inst_a, gm82_instance *inst_b) {
    if (!rt || !inst_a || !inst_b || !inst_a->alive || !inst_b->alive) return;
    gm82_gml_set_runtime(rt);
    if (rt->actions) {
        gm82_gml_set_self(inst_a);
        gm82_gml_set_other(inst_b);
        gm82_actions_fire_event(rt, inst_a, GM82_EV_COLLISION, inst_b->object_index, rt->actions);
        gm82_gml_set_self(inst_b);
        gm82_gml_set_other(inst_a);
        gm82_actions_fire_event(rt, inst_b, GM82_EV_COLLISION, inst_a->object_index, rt->actions);
    }
}
