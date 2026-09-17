#define _POSIX_C_SOURCE 200809L
#include "gm82_runtime.h"
#include "gm82_gml_builtins.h"
#include "gm82_events.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <math.h>

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

void gm82_runtime_init(gm82_runtime *rt) {
    memset(rt, 0, sizeof(*rt));
    rt->next_id = 100001;
    rt->room_speed = 30;
}

void gm82_runtime_bind_assets(gm82_runtime *rt,
    const gm82_decoded_object_list *objects,
    const gm82_decoded_sprite_list *sprites,
    const gm82_decoded_background_list *backgrounds,
    const gm82_decoded_room_list *rooms,
    const gm82_action_table *actions) {
    rt->objects = objects;
    rt->sprites = sprites;
    rt->backgrounds = backgrounds;
    rt->rooms = rooms;
    rt->actions = actions;
    rt->sprite_groups = NULL;
}

void gm82_runtime_bind_sprite_groups(gm82_runtime *rt, const gm82_sprite_group_list *groups) {
    rt->sprite_groups = groups;
}

void gm82_runtime_bind_scripts(gm82_runtime *rt, const struct gm82_script_list *scripts) {
    if (rt) rt->scripts = scripts;
}

static void apply_object_defaults(gm82_runtime *rt, gm82_instance *inst, int32_t object_index) {
    inst->object_index = object_index;
    inst->sprite_index = -1;
    inst->solid = 0;
    inst->visible = 1;
    inst->depth = 0;
    inst->persistent = 0;
    inst->image_index = 0;
    inst->image_speed = 1.0;
    if (rt->objects && object_index >= 0 && object_index < rt->objects->count) {
        const gm82_decoded_object *o = &rt->objects->items[object_index];
        inst->sprite_index = o->sprite_index;
        inst->solid = o->solid;
        inst->visible = o->visible ? 1 : 1; /* default visible if flag weird */
        inst->depth = o->depth;
    }
}

gm82_instance *gm82_runtime_instance_create(gm82_runtime *rt, int32_t object_index, double x, double y) {
    if (!rt || rt->instance_count >= GM82_MAX_INSTANCES) return NULL;
    gm82_instance *inst = &rt->instances[rt->instance_count++];
    memset(inst, 0, sizeof(*inst));
    inst->id = rt->next_id++;
    inst->x = x; inst->y = y;
    inst->alive = 1;
    for (int i = 0; i < 12; i++) inst->alarms[i] = -1;
    apply_object_defaults(rt, inst, object_index);
    /* Create event: minimal – nothing yet (no GML). Hook point for later. */
    return inst;
}

void gm82_runtime_instance_destroy(gm82_runtime *rt, gm82_instance *inst) {
    (void)rt;
    if (inst) inst->alive = 0;
}

bool gm82_runtime_goto_room(gm82_runtime *rt, int room_index) {
    if (!rt || !rt->rooms || room_index < 0 || room_index >= rt->rooms->count)
        return false;

    const gm82_decoded_room *room = &rt->rooms->items[room_index];
    rt->current_room = room_index;
    rt->room_width = room->width;
    rt->room_height = room->height;
    rt->room_speed = room->speed > 0 ? room->speed : 30;
    rt->view_enabled = 1;
    rt->view_x = 0; rt->view_y = 0;
    rt->view_w = room->width > 640 ? 320.0 : (double)room->width;
    rt->view_h = room->height > 480 ? 240.0 : (double)room->height;
    if (rt->view_w > room->width) rt->view_w = room->width;
    if (rt->view_h > room->height) rt->view_h = room->height;
    rt->view_follow_object = 0;

    /* Fire Room End for all alive instances before leaving current room */
    if (rt->running) {
        gm82_events_fire_event_all(rt, GM82_EV_OTHER, 5); /* 5 = Room End in GM */
    }

    /* Destroy non-persistent */
    int w = 0;
    for (int i = 0; i < rt->instance_count; i++) {
        if (rt->instances[i].alive && rt->instances[i].persistent) {
            if (w != i) rt->instances[w] = rt->instances[i];
            w++;
        }
    }
    rt->instance_count = w;

    /* Spawn room instances + Create */
    for (int i = 0; i < room->instance_count; i++) {
        const gm82_decoded_instance *src = &room->instances[i];
        gm82_instance *inst = gm82_runtime_instance_create(rt, src->object_index, (double)src->x, (double)src->y);
        if (inst && src->id > 0) inst->id = src->id;
    }
    rt->running = 1;
    rt->frame = 0;
    gm82_events_fire_create_all(rt);
    /* Fire Room Start (type 7, numb 4 in GM) */
    gm82_events_fire_event_all(rt, GM82_EV_OTHER, 4);
    return true;
}

static void update_speed_from_dir(gm82_instance *inst) {
    double rad = inst->direction * M_PI / 180.0;
    inst->hspeed = cos(rad) * inst->speed;
    inst->vspeed = -sin(rad) * inst->speed; /* GM: direction 90 = up */
}

void gm82_runtime_step(gm82_runtime *rt) {
    if (!rt || !rt->running) return;
    rt->frame++;
    {
        double sec = (rt->room_speed > 0) ? (1.0 / (double)rt->room_speed) : (1.0/30.0);
        gm82_gml_set_frame_time(sec);
    }

    /* Object Step events (behaviors / future GML) */
    gm82_events_fire_step_all(rt);

    /* Alarms */
    for (int i = 0; i < rt->instance_count; i++) {
        gm82_instance *inst = &rt->instances[i];
        if (!inst->alive) continue;
        for (int a = 0; a < 12; a++) {
            if (inst->alarms[a] > 0) {
                inst->alarms[a]--;
                if (inst->alarms[a] == 0) {
                    gm82_alarm_fire_scripts(rt, inst, a);
                    inst->alarms[a] = -1;
                }
            }
        }
    }

    /* Begin Step / Step: apply velocity */
    for (int i = 0; i < rt->instance_count; i++) {
        gm82_instance *inst = &rt->instances[i];
        if (!inst->alive) continue;

        if (inst->speed != 0.0)
            update_speed_from_dir(inst);

        if (inst->path_index >= 0 && inst->path_speed != 0.0) {
            gm82_path_step_instance(inst);
        }
        if (inst->timeline_running) {
            gm82_timeline_step_instance(rt, inst);
        }

        if (inst->gravity != 0.0) {
            double rad = inst->gravity_direction * 3.141592653589793 / 180.0;
            inst->hspeed += cos(rad) * inst->gravity;
            inst->vspeed -= sin(rad) * inst->gravity;
        }
        if (inst->friction != 0.0) {
            double f = inst->friction;
            if (inst->hspeed > 0) { inst->hspeed -= f; if (inst->hspeed < 0) inst->hspeed = 0; }
            else if (inst->hspeed < 0) { inst->hspeed += f; if (inst->hspeed > 0) inst->hspeed = 0; }
            if (inst->vspeed > 0) { inst->vspeed -= f; if (inst->vspeed < 0) inst->vspeed = 0; }
            else if (inst->vspeed < 0) { inst->vspeed += f; if (inst->vspeed > 0) inst->vspeed = 0; }
        }

        inst->x += inst->hspeed;
        inst->y += inst->vspeed;

        /* Solid collision against other solid instances (AABB using sprite size) */
        if (rt->sprites) {
            int32_t sw = 16, sh = 16;
            if (inst->sprite_index >= 0 && inst->sprite_index < rt->sprites->count) {
                sw = rt->sprites->frames[inst->sprite_index].width;
                sh = rt->sprites->frames[inst->sprite_index].height;
            }
            for (int j = 0; j < rt->instance_count; j++) {
                if (i == j) continue;
                gm82_instance *other = &rt->instances[j];
                if (!other->alive || !other->solid) continue;
                int32_t ow = 16, oh = 16;
                if (other->sprite_index >= 0 && other->sprite_index < rt->sprites->count) {
                    ow = rt->sprites->frames[other->sprite_index].width;
                    oh = rt->sprites->frames[other->sprite_index].height;
                }
                double ax1 = inst->x, ay1 = inst->y, ax2 = inst->x + sw, ay2 = inst->y + sh;
                double bx1 = other->x, by1 = other->y, bx2 = other->x + ow, by2 = other->y + oh;
                if (ax1 < bx2 && ax2 > bx1 && ay1 < by2 && ay2 > by1) {
                    /* push back on Y if falling */
                    if (inst->vspeed > 0 && (inst->y - inst->vspeed + sh) <= by1 + 2) {
                        inst->y = other->y - sh;
                        inst->vspeed = 0;
                    } else if (inst->vspeed < 0 && (inst->y - inst->vspeed) >= by2 - 2) {
                        inst->y = by2;
                        inst->vspeed = 0;
                    } else if (inst->hspeed > 0) {
                        inst->x = bx1 - sw;
                        inst->hspeed = 0;
                    } else if (inst->hspeed < 0) {
                        inst->x = bx2;
                        inst->hspeed = 0;
                    }
                }
            }
        }

        /* Tile collision (treat tiles as solid platforms) */
        if (rt->rooms && rt->current_room >= 0 && rt->current_room < rt->rooms->count) {
            const gm82_decoded_room *room = &rt->rooms->items[rt->current_room];
            int32_t sw = 16, sh = 16;
            if (rt->sprites && inst->sprite_index >= 0 && inst->sprite_index < rt->sprites->count) {
                sw = rt->sprites->frames[inst->sprite_index].width;
                sh = rt->sprites->frames[inst->sprite_index].height;
            }
            for (int ti = 0; ti < room->tile_count; ti++) {
                const gm82_decoded_tile *tile = &room->tiles[ti];
                double ax1 = inst->x, ay1 = inst->y, ax2 = inst->x + sw, ay2 = inst->y + sh;
                double bx1 = tile->x, by1 = tile->y, bx2 = tile->x + tile->width, by2 = tile->y + tile->height;
                if (ax1 < bx2 && ax2 > bx1 && ay1 < by2 && ay2 > by1) {
                    if (inst->vspeed > 0 && (inst->y - inst->vspeed + sh) <= by1 + 1) {
                        inst->y = by1 - sh;
                        inst->vspeed = 0;
                    } else if (inst->hspeed > 0) {
                        inst->x = bx1 - sw;
                        inst->hspeed = 0;
                    } else if (inst->hspeed < 0) {
                        inst->x = bx2;
                        inst->hspeed = 0;
                    }
                }
            }
        }

        /* image_index advance (animation) */
        if (inst->image_speed != 0.0) {
            /* accumulate using direction field low bits? keep simple: add rounded speed each frame */
            static double accum[512];
            int slot = (inst->id > 0) ? (inst->id % 512) : 0;
            accum[slot] += inst->image_speed;
            while (accum[slot] >= 1.0) {
                inst->image_index++;
                accum[slot] -= 1.0;
            }
            while (accum[slot] <= -1.0) {
                inst->image_index--;
                accum[slot] += 1.0;
            }
            if (rt->sprite_groups && inst->sprite_index >= 0) {
                int gi = gm82_sprite_group_index_for_frame(rt->sprite_groups, inst->sprite_index);
                if (gi >= 0) {
                    int fc = rt->sprite_groups->items[gi].frame_count;
                    if (fc > 0) {
                        int rel = inst->image_index % fc;
                        if (rel < 0) rel += fc;
                        inst->image_index = rel;
                    }
                }
            }
        }
    }

    /* Collision events dispatch (Phase 2 Parity) */
    for (int i = 0; i < rt->instance_count; i++) {
        gm82_instance *inst = &rt->instances[i];
        if (!inst->alive) continue;
        int32_t sw = 16, sh = 16;
        if (rt->sprites && inst->sprite_index >= 0 && inst->sprite_index < rt->sprites->count) {
            sw = rt->sprites->frames[inst->sprite_index].width;
            sh = rt->sprites->frames[inst->sprite_index].height;
        }
        for (int j = i + 1; j < rt->instance_count; j++) {
            gm82_instance *other = &rt->instances[j];
            if (!other->alive) continue;
            int32_t ow = 16, oh = 16;
            if (rt->sprites && other->sprite_index >= 0 && other->sprite_index < rt->sprites->count) {
                ow = rt->sprites->frames[other->sprite_index].width;
                oh = rt->sprites->frames[other->sprite_index].height;
            }
            if (inst->x < other->x + ow && inst->x + sw > other->x &&
                inst->y < other->y + oh && inst->y + sh > other->y) {
                gm82_events_fire_collision(rt, inst, other);
                if (inst->alive && other->alive) {
                    gm82_events_fire_collision(rt, other, inst);
                }
            }
        }
    }

    /* Camera follow */
    if (rt->view_enabled && rt->view_follow_object >= 0) {
        for (int i = 0; i < rt->instance_count; i++) {
            gm82_instance *tgt = &rt->instances[i];
            if (!tgt->alive || tgt->object_index != rt->view_follow_object) continue;
            double margin = 64;
            if (tgt->x < rt->view_x + margin) rt->view_x = tgt->x - margin;
            if (tgt->x > rt->view_x + rt->view_w - margin) rt->view_x = tgt->x - rt->view_w + margin;
            if (tgt->y < rt->view_y + margin) rt->view_y = tgt->y - margin;
            if (tgt->y > rt->view_y + rt->view_h - margin) rt->view_y = tgt->y - rt->view_h + margin;
            if (rt->view_x < 0) rt->view_x = 0;
            if (rt->view_y < 0) rt->view_y = 0;
            if (rt->view_x + rt->view_w > rt->room_width) rt->view_x = rt->room_width - rt->view_w;
            if (rt->view_y + rt->view_h > rt->room_height) rt->view_y = rt->room_height - rt->view_h;
            if (rt->view_x < 0) rt->view_x = 0;
            if (rt->view_y < 0) rt->view_y = 0;
            break;
        }
    }

    /* Compact dead instances periodically */
    if ((rt->frame & 31) == 0) {
        int w = 0;
        for (int i = 0; i < rt->instance_count; i++) {
            if (rt->instances[i].alive) {
                if (w != i) rt->instances[w] = rt->instances[i];
                w++;
            }
        }
        rt->instance_count = w;
    }
}

static void blit(uint8_t *dst, int dw, int dh, const uint8_t *src, int sw, int sh, int dx, int dy) {
    for (int y = 0; y < sh; y++) {
        int yy = dy + y;
        if (yy < 0 || yy >= dh) continue;
        for (int x = 0; x < sw; x++) {
            int xx = dx + x;
            if (xx < 0 || xx >= dw) continue;
            const uint8_t *s = src + (y * sw + x) * 4;
            uint8_t sa = s[3];
            if (sa == 0) continue;
            uint8_t *d = dst + (yy * dw + xx) * 4;
            if (sa == 255) {
                d[0] = s[0]; d[1] = s[1]; d[2] = s[2]; d[3] = 255;
            } else {
                uint32_t a = sa;
                uint32_t inv = 255 - a;
                d[0] = (uint8_t)((s[0] * a + d[0] * inv) / 255);
                d[1] = (uint8_t)((s[1] * a + d[1] * inv) / 255);
                d[2] = (uint8_t)((s[2] * a + d[2] * inv) / 255);
                d[3] = 255;
            }
        }
    }
}

bool gm82_runtime_draw(gm82_runtime *rt, uint8_t *rgba, int32_t buf_w, int32_t buf_h) {
    if (!rt || !rgba || buf_w <= 0 || buf_h <= 0) return false;

    /* clear */
    for (int i = 0; i < buf_w * buf_h; i++) {
        rgba[i*4+0]=20; rgba[i*4+1]=20; rgba[i*4+2]=40; rgba[i*4+3]=255;
    }

    /* background 0 (full image – room may also use tiles from same bg) */
    if (rt->backgrounds && rt->backgrounds->count > 0 && rt->backgrounds->items[0].rgba) {
        const gm82_decoded_background *bg = &rt->backgrounds->items[0];
        /* only fill if no tiles; otherwise tiles compose the level art */
        int has_tiles = (rt->rooms && rt->current_room >= 0 && rt->current_room < rt->rooms->count
                         && rt->rooms->items[rt->current_room].tile_count > 0);
        if (!has_tiles)
            blit(rgba, buf_w, buf_h, bg->rgba, bg->width, bg->height, 0, 0);
    }

    /* tiles from current room */
    if (rt->rooms && rt->backgrounds && rt->current_room >= 0 && rt->current_room < rt->rooms->count) {
        const gm82_decoded_room *room = &rt->rooms->items[rt->current_room];
        for (int ti = 0; ti < room->tile_count; ti++) {
            const gm82_decoded_tile *tile = &room->tiles[ti];
            if (tile->background_index < 0 || tile->background_index >= rt->backgrounds->count)
                continue;
            const gm82_decoded_background *bg = &rt->backgrounds->items[tile->background_index];
            if (!bg->rgba) continue;
            int tw = tile->width, th = tile->height;
            for (int y = 0; y < th; y++) {
                int sy = tile->yo + y, dy = tile->y + y - (int)rt->view_y;
                if (sy < 0 || sy >= bg->height || dy < 0 || dy >= buf_h) continue;
                for (int x = 0; x < tw; x++) {
                    int sx = tile->xo + x, dx = tile->x + x - (int)rt->view_x;
                    if (sx < 0 || sx >= bg->width || dx < 0 || dx >= buf_w) continue;
                    const uint8_t *s = bg->rgba + ((size_t)sy * (size_t)bg->width + (size_t)sx) * 4;
                    uint8_t sa = s[3];
                    if (sa == 0) continue;
                    uint8_t *d = rgba + ((size_t)dy * (size_t)buf_w + (size_t)dx) * 4;
                    if (sa == 255) {
                        d[0] = s[0]; d[1] = s[1]; d[2] = s[2]; d[3] = 255;
                    } else {
                        uint32_t a = sa;
                        uint32_t inv = 255 - a;
                        d[0] = (uint8_t)((s[0] * a + d[0] * inv) / 255);
                        d[1] = (uint8_t)((s[1] * a + d[1] * inv) / 255);
                        d[2] = (uint8_t)((s[2] * a + d[2] * inv) / 255);
                        d[3] = 255;
                    }
                }
            }
        }
    }

    /* sort by depth (simple insertion for small counts) – draw high depth first */
    int order[GM82_MAX_INSTANCES];
    int n = 0;
    for (int i = 0; i < rt->instance_count && n < GM82_MAX_INSTANCES; i++)
        if (rt->instances[i].alive && rt->instances[i].visible)
            order[n++] = i;
    for (int i = 1; i < n; i++) {
        int key = order[i];
        int j = i - 1;
        while (j >= 0 && rt->instances[order[j]].depth < rt->instances[key].depth) {
            order[j+1] = order[j];
            j--;
        }
        order[j+1] = key;
    }

    for (int k = 0; k < n; k++) {
        gm82_instance *inst = &rt->instances[order[k]];
        /* If custom Draw event actions exist, execute them */
        if (rt->actions) {
            gm82_gml_set_self(inst);
            gm82_actions_fire_event(rt, inst, GM82_EV_DRAW, 0, rt->actions);
        }
        int si = inst->sprite_index;
        if (rt->sprite_groups)
            si = gm82_sprite_resolve_frame(rt->sprite_groups, inst->sprite_index, inst->image_index);
        if (!rt->sprites || si < 0 || si >= rt->sprites->count)
            continue;
        const gm82_decoded_frame *fr = &rt->sprites->frames[si];
        if (!fr->rgba) continue;
        {
            int dx = (int)inst->x - (int)rt->view_x;
            int dy = (int)inst->y - (int)rt->view_y;
            blit(rgba, buf_w, buf_h, fr->rgba, fr->width, fr->height, dx, dy);
        }
    }
    return true;
}

void gm82_runtime_event_user(gm82_runtime *rt, int user_event_index) {
    if (!rt || user_event_index < 0 || user_event_index > 11) return;
    /* GM82: User defined events are Other event (type 7), sub_type = 10 + user_event_index */
    gm82_events_fire_event_all(rt, GM82_EV_OTHER, 10 + user_event_index);
}
