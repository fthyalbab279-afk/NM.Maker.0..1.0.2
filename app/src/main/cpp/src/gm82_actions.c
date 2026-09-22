#define _POSIX_C_SOURCE 200809L
#include "gm82_actions.h"
#include "gm82_gml_builtins.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <zlib.h>

#define GM82_ACTION_MAX_ARGS 8

static int32_t rd_i32(const uint8_t *p) {
    return (int32_t)(p[0]|(p[1]<<8)|(p[2]<<16)|(p[3]<<24));
}

static uint8_t *inflate_at(const uint8_t *src, size_t n, size_t *ol) {
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

void gm82_action_table_free(gm82_action_table *t) {
    if (!t) return;
    for (int i = 0; i < t->count; i++) free(t->objects[i].items);
    free(t->objects);
    memset(t, 0, sizeof(*t));
}

static void add_action(gm82_object_actions *oa, int etype, int enumb, const char *name,
                       int aid, int kind, const int32_t *args, int argc) {
    if (oa->count >= oa->capacity) {
        int nc = oa->capacity ? oa->capacity * 2 : 8;
        gm82_action_ref *n = (gm82_action_ref *)realloc(oa->items, (size_t)nc * sizeof(*n));
        if (!n) return;
        oa->items = n; oa->capacity = nc;
    }
    gm82_action_ref *a = &oa->items[oa->count++];
    memset(a, 0, sizeof(*a));
    a->event_type = etype;
    a->event_numb = enumb;
    if (argc > 0) a->action_id = args[0];
    else a->action_id = aid;
    if (argc > 1) a->kind = args[1];
    else a->kind = kind;
    if (name) strncpy(a->name, name, sizeof(a->name)-1);
}

int gm82_actions_scan_gmk(const uint8_t *data, size_t size, gm82_action_table *out) {
    memset(out, 0, sizeof(*out));
    if (!data || size < 12) return -1;
    gm82_object_actions tmp[128];
    int nobj = 0;
    for (size_t i = 12; i + 2 < size; i++) {
        if (!(data[i]==0x78 && (data[i+1]==0x9c||data[i+1]==0xda||data[i+1]==0x01||data[i+1]==0x5e)))
            continue;
        size_t ol = 0;
        uint8_t *d = inflate_at(data + i, size - i, &ol);
        i += 16;
        if (!d || ol < 40) { free(d); continue; }
        if (rd_i32(d) != 1) { free(d); continue; }
        int32_t slen = rd_i32(d + 4);
        if (slen < 4 || slen > 48 || 8 + (size_t)slen + 12 > ol) { free(d); continue; }
        char name[64];
        memcpy(name, d + 8, (size_t)slen); name[slen] = 0;
        int ok = 1;
        for (int k = 0; k < slen; k++) if (name[k] < 32 || name[k] > 126) ok = 0;
        if (!ok || (strncmp(name, "obj", 3) != 0 && strncmp(name, "object", 6) != 0)) { free(d); continue; }
        if (nobj >= 128) { free(d); break; }
        gm82_object_actions *oa = &tmp[nobj++];
        memset(oa, 0, sizeof(*oa));
        strncpy(oa->object_name, name, sizeof(oa->object_name)-1);
        for (size_t o = 0; o + 8 < ol; o++) {
            int32_t n = rd_i32(d + o);
            if (n < 8 || n > 48 || o + 4 + (size_t)n > ol) continue;
            const uint8_t *s = d + o + 4;
            if (memcmp(s, "action_", 7) != 0) continue;
            int good = 1;
            for (int k = 0; k < n; k++) if (s[k] < 32 || s[k] > 126) { good = 0; break; }
            if (!good) continue;
            char aname[64];
            memcpy(aname, s, (size_t)n); aname[n] = 0;
            int etype = -1, aid = 0, kind = 0;
            if (o >= 24) {
                kind = rd_i32(d + o - 20);
                for (int b = 24; b <= 48 && (int)o - b >= 0; b += 4) {
                    if (rd_i32(d + o - b) == 400) {
                        if ((int)o - b >= 4) etype = rd_i32(d + o - b - 4);
                        break;
                    }
                }
            }
            int32_t args[GM82_ACTION_MAX_ARGS];
            int argc = 0;
            size_t ao = o + 4 + (size_t)n;
            while (argc < GM82_ACTION_MAX_ARGS && ao + 4 <= ol) {
                int32_t v = rd_i32(d + ao);
                if (v >= 8 && v <= 48 && ao + 4 + (size_t)v <= ol) {
                    const uint8_t *p = d + ao + 4;
                    if (memcmp(p, "action_", 7) == 0) break;
                }
                if (v == 400) break;
                args[argc++] = v;
                ao += 4;
                if (argc >= 4) break;
            }
            add_action(oa, etype, 0, aname, aid, kind, args, argc);
            o += 4 + (size_t)n;
        }
        free(d);
    }

    /* Second pass: harvest action_* from ANY inflate (shooter packs actions outside obj blobs) */
    if (nobj == 0 || 1) {
        gm82_object_actions global_bucket;
        memset(&global_bucket, 0, sizeof(global_bucket));
        strncpy(global_bucket.object_name, "_global_actions", sizeof(global_bucket.object_name)-1);
        for (size_t i = 12; i + 2 < size; i++) {
            if (!(data[i]==0x78 && (data[i+1]==0x9c||data[i+1]==0xda||data[i+1]==0x01||data[i+1]==0x5e)))
                continue;
            size_t ol = 0;
            uint8_t *d = inflate_at(data + i, size - i, &ol);
            i += 16;
            if (!d || ol < 16) { free(d); continue; }
            for (size_t o = 0; o + 8 < ol; o++) {
                int32_t n = rd_i32(d + o);
                if (n < 8 || n > 48 || o + 4 + (size_t)n > ol) continue;
                const uint8_t *s = d + o + 4;
                if (memcmp(s, "action_", 7) != 0) continue;
                int good = 1;
                for (int k = 0; k < n; k++) if (s[k] < 32 || s[k] > 126) { good = 0; break; }
                if (!good) continue;
                char aname[64];
                memcpy(aname, s, (size_t)n); aname[n] = 0;
                /* skip if already captured under an object with same name */
                int dup = 0;
                for (int oi = 0; oi < nobj && !dup; oi++)
                    for (int ai = 0; ai < tmp[oi].count; ai++)
                        if (strcmp(tmp[oi].items[ai].name, aname) == 0) { dup = 1; break; }
                if (dup) { o += 4 + (size_t)n; continue; }
                add_action(&global_bucket, -1, 0, aname, 0, 0, NULL, 0);
                o += 4 + (size_t)n;
            }
            free(d);
        }
        if (global_bucket.count > 0 && nobj < 128) {
            tmp[nobj++] = global_bucket;
        } else {
            free(global_bucket.items);
        }
    }

    out->count = nobj;
    if (nobj > 0) {
        out->objects = (gm82_object_actions *)malloc((size_t)nobj * sizeof(*out->objects));
        if (out->objects) {
            memcpy(out->objects, tmp, (size_t)nobj * sizeof(*tmp));
            for (int i = 0; i < nobj; i++) tmp[i].items = NULL;
        } else out->count = 0;
    }
    return out->count;
}

bool gm82_action_execute_named(gm82_runtime *rt, gm82_instance *self, const char *name) {
    if (!rt || !self || !name) return false;
    gm82_gml_set_runtime(rt);
    gm82_gml_set_self(self);
    if (strcmp(name, "action_kill_object") == 0 || strcmp(name, "action_destroy") == 0) {
        gml_instance_destroy();
        return true;
    }
    if (strcmp(name, "action_sprite_set") == 0) return true;
    if (strcmp(name, "action_move") == 0) return true;
    if (strcmp(name, "action_set_hspeed") == 0) return true;
    if (strcmp(name, "action_set_vspeed") == 0) return true;
    if (strcmp(name, "action_set_gravity") == 0) return true;
    if (strcmp(name, "action_set_health") == 0) return true;
    if (strcmp(name, "action_sound") == 0) return true; /* no audio backend yet */
    if (strcmp(name, "action_end_sound") == 0) return true;
    if (strcmp(name, "action_restart_game") == 0) { rt->running = 0; return true; }
    if (strcmp(name, "action_end_game") == 0) { rt->running = 0; return true; }
    if (strcmp(name, "action_bounce") == 0) {
        if (self->vspeed*self->vspeed >= self->hspeed*self->hspeed)
            self->vspeed = -self->vspeed;
        else
            self->hspeed = -self->hspeed;
        return true;
    }
    if (strcmp(name, "action_set_cursor") == 0) return true;
    if (strcmp(name, "action_change_object") == 0) return true;
    return false;
}

static bool execute_ref(gm82_runtime *rt, gm82_instance *self, const gm82_action_ref *ar) {
    if (!ar || !ar->name[0]) return false;
    gm82_gml_set_runtime(rt);
    gm82_gml_set_self(self);
    if (strcmp(ar->name, "action_sprite_set") == 0) {
        self->sprite_index = ar->action_id >= 0 ? ar->action_id : (ar->kind >= 0 ? ar->kind : 1);
        return true;
    }
    if (strcmp(ar->name, "action_change_object") == 0) {
        int target_oi = ar->action_id >= 0 ? ar->action_id : ar->kind;
        if (rt->objects && target_oi >= 0 && target_oi < rt->objects->count) {
            self->object_index = target_oi;
            self->sprite_index = rt->objects->items[target_oi].sprite_index;
            self->solid = rt->objects->items[target_oi].solid;
        }
        return true;
    }
    if (strcmp(ar->name, "action_set_hspeed") == 0) {
        self->hspeed = (double)ar->action_id;
        return true;
    }
    if (strcmp(ar->name, "action_set_vspeed") == 0) {
        self->vspeed = (double)ar->action_id;
        return true;
    }
    if (strcmp(ar->name, "action_kill_object") == 0) {
        gml_instance_destroy();
        return true;
    }
    if (strcmp(ar->name, "action_set_cursor") == 0) return true;
    if (strcmp(ar->name, "action_bounce") == 0) {
        if (ar->action_id == 0) self->hspeed = -self->hspeed;
        else self->vspeed = -self->vspeed;
        return true;
    }
    if (strcmp(ar->name, "action_set_score") == 0) {
        if (ar->kind != 0) gml_set_score(gml_get_score() + ar->action_id);
        else gml_set_score((double)ar->action_id);
        return true;
    }
    if (strcmp(ar->name, "action_set_lives") == 0) {
        if (ar->kind != 0) gml_set_lives(gml_get_lives() + ar->action_id);
        else gml_set_lives((double)ar->action_id);
        return true;
    }
    if (strcmp(ar->name, "action_set_health") == 0) {
        if (ar->kind != 0) gml_set_health(gml_get_health() + ar->action_id);
        else gml_set_health((double)ar->action_id);
        return true;
    }
    if (strcmp(ar->name, "action_set_gravity") == 0) {
        self->gravity = (double)ar->action_id;
        if (ar->kind != 0) self->gravity_direction = (double)ar->kind;
        return true;
    }
    if (strcmp(ar->name, "action_set_friction") == 0) {
        self->friction = (double)ar->action_id;
        return true;
    }
    if (strcmp(ar->name, "action_move_to") == 0) {
        self->x = (double)ar->action_id;
        self->y = (double)ar->kind;
        return true;
    }
    if (strcmp(ar->name, "action_snap") == 0) {
        int sx = ar->action_id > 0 ? ar->action_id : 16;
        int sy = ar->kind > 0 ? ar->kind : 16;
        self->x = (double)(((int)self->x / sx) * sx);
        self->y = (double)(((int)self->y / sy) * sy);
        return true;
    }
    if (strcmp(ar->name, "action_reverse_ydir") == 0) {
        self->vspeed = -self->vspeed;
        return true;
    }
    if (strcmp(ar->name, "action_speed_horizontal") == 0) {
        self->hspeed = (double)ar->action_id;
        return true;
    }
    if (strcmp(ar->name, "action_speed_vertical") == 0) {
        self->vspeed = (double)ar->action_id;
        return true;
    }
    if (strcmp(ar->name, "action_sound") == 0 || strcmp(ar->name, "action_play_sound") == 0 ||
        strcmp(ar->name, "action_snd_play") == 0) {
        /* Needs sound runtime global – no-op if unbound */
        return true;
    }
    if (strcmp(ar->name, "action_reverse_xdir") == 0) {
        self->hspeed = -self->hspeed;
        return true;
    }
    if (strcmp(ar->name, "action_move") == 0) {
        /* arg0 often direction-related; arg1 speed – best-effort */
        if (ar->kind != 0) self->speed = (double)ar->kind;
        if (ar->action_id != 0) {
            self->direction = (double)ar->action_id;
        }
        return true;
    }
    if (strcmp(ar->name, "action_next_room") == 0 || strcmp(ar->name, "action_if_next_room") == 0) {
        if (rt->rooms && rt->current_room + 1 < rt->rooms->count)
            gm82_runtime_goto_room(rt, rt->current_room + 1);
        return true;
    }
    if (strcmp(ar->name, "action_set_alarm") == 0) {
        int idx = ar->action_id;
        if (idx < 0) idx = 0;
        if (idx > 11) idx = 11;
        self->alarms[idx] = ar->kind > 0 ? ar->kind : 30;
        return true;
    }
    if (strcmp(ar->name, "action_create_object") == 0) {
        int oi = ar->action_id;
        gm82_runtime_instance_create(rt, oi, self->x, self->y);
        return true;
    }
    return false;
}

void gm82_actions_fire_create(gm82_runtime *rt, gm82_instance *self, const gm82_action_table *table) {
    if (!rt || !self || !table || !rt->objects) return;
    if (self->object_index < 0 || self->object_index >= rt->objects->count) return;
    const char *oname = rt->objects->items[self->object_index].name;
    for (int i = 0; i < table->count; i++) {
        if (strcmp(table->objects[i].object_name, oname) != 0) continue;
        for (int a = 0; a < table->objects[i].count; a++) {
            gm82_action_ref *ar = &table->objects[i].items[a];
            /* Create event only (type 0). Skip unknown (-1) room transitions on spawn. */
            if (ar->event_type != 0) continue;
            if (strcmp(ar->name, "action_next_room") == 0 ||
                strcmp(ar->name, "action_if_next_room") == 0 ||
                strcmp(ar->name, "action_previous_room") == 0)
                continue;
            execute_ref(rt, self, ar);
        }
        break;
    }
}

void gm82_actions_fire_step(gm82_runtime *rt, gm82_instance *self, const gm82_action_table *table) {
    if (!rt || !self || !table || !rt->objects) return;
    if (self->object_index < 0 || self->object_index >= rt->objects->count) return;
    const char *oname = rt->objects->items[self->object_index].name;
    for (int i = 0; i < table->count; i++) {
        if (strcmp(table->objects[i].object_name, oname) != 0) continue;
        for (int a = 0; a < table->objects[i].count; a++) {
            gm82_action_ref *ar = &table->objects[i].items[a];
            /* Step event type 3; also allow unknown for move/hspeed style */
            if (ar->event_type != 3 && ar->event_type != -1) continue;
            if (strcmp(ar->name, "action_set_hspeed") == 0 ||
                strcmp(ar->name, "action_reverse_xdir") == 0 ||
                strcmp(ar->name, "action_move") == 0) {
                /* reuse create executor path via fire_create single - call execute by name pack */
                /* Direct: */
                gm82_gml_set_runtime(rt);
                gm82_gml_set_self(self);
                if (strcmp(ar->name, "action_set_hspeed") == 0)
                    self->hspeed = (double)ar->action_id;
                else if (strcmp(ar->name, "action_reverse_xdir") == 0)
                    self->hspeed = -self->hspeed;
            }
        }
        break;
    }
}
