#define _POSIX_C_SOURCE 200809L
#include "gm82_object_room_decode.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <zlib.h>

static int32_t rd_i32(const uint8_t *p) {
    return (int32_t)(p[0]|(p[1]<<8)|(p[2]<<16)|(p[3]<<24));
}

static uint8_t *inflate_at(const uint8_t *src, size_t n, size_t *out_len) {
    *out_len = 0;
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
    *out_len = strm.total_out; inflateEnd(&strm); return dst;
}

void gm82_decoded_object_list_free(gm82_decoded_object_list *L) {
    if (!L) return;
    if (L->items) {
        for (int i = 0; i < L->count; i++) {
            for (int e = 0; e < L->items[i].event_count && e < GM82_OBJ_EVENT_MAX; e++)
                free(L->items[i].events[e].code_snippet);
        }
    }
    free(L->items);
    memset(L, 0, sizeof(*L));
}
void gm82_decoded_room_list_free(gm82_decoded_room_list *L) {
    if (!L) return;
    for (int i = 0; i < L->count; i++) {
        free(L->items[i].instances);
        free(L->items[i].tiles);
    }
    free(L->items); memset(L, 0, sizeof(*L));
}

/* Object ver 430 (observed): sprite_index, solid, visible, depth, ... */
int gm82_decode_objects_from_gmk(const uint8_t *data, size_t size, gm82_decoded_object_list *out) {
    memset(out, 0, sizeof(*out));
    if (!data || size < 12) return -1;
    gm82_decoded_object *tmp = (gm82_decoded_object *)calloc(256, sizeof(gm82_decoded_object));
    if (!tmp) return -1;
    int n = 0;

    for (size_t i = 12; i + 2 < size; i++) {
        if (!(data[i]==0x78 && (data[i+1]==0x9c||data[i+1]==0xda||data[i+1]==0x01||data[i+1]==0x5e)))
            continue;
        size_t ol = 0;
        uint8_t *d = inflate_at(data + i, size - i, &ol);
        i += 16;
        if (!d || ol < 28) { free(d); continue; }
        if (rd_i32(d) != 1) { free(d); continue; }
        int32_t slen = rd_i32(d + 4);
        if (slen < 4 || slen > 48 || 8 + (size_t)slen + 12 > ol) { free(d); continue; }
        const uint8_t *ns = d + 8;
        int ok = 1;
        for (int k = 0; k < slen; k++) if (ns[k] < 32 || ns[k] > 126) ok = 0;
        if (!ok) { free(d); continue; }
        char name[64]; memcpy(name, ns, (size_t)slen); name[slen] = 0;
        /* Accept obj_*, objName (no underscore), object* */
        if (strncmp(name, "obj", 3) != 0 && strncmp(name, "object", 6) != 0) { free(d); continue; }
        size_t off = 8 + (size_t)slen + 8;
        int32_t ver = rd_i32(d + off); off += 4;
        if (ver != 430 && ver != 400 && ver != 800) { free(d); continue; }
        if (n >= 256) { free(d); break; }
        gm82_decoded_object *o = &tmp[n++];
        memset(o, 0, sizeof(*o));
        strncpy(o->name, name, sizeof(o->name)-1);
        o->sprite_index = rd_i32(d + off);
        o->solid = rd_i32(d + off + 4);
        o->visible = rd_i32(d + off + 8);
        o->depth = rd_i32(d + off + 12);
        o->persistent = (off + 16 + 4 <= ol) ? rd_i32(d + off + 16) : 0;
        o->event_count = 0;
        /* GM8: after object header, for main event type 0..11:
         *   list of (event_kind, actionlist) ending with event_kind=-1
         * ver 430 layout: fields then event stream at off+32 */
        if ((ver == 430 || ver == 400) && off + 32 <= ol) {
            size_t p = off + 32;
            /* GM8 always has 12 main event categories (Create..Trigger) */
            for (int main = 0; main < 12 && p + 4 <= ol; main++) {
                int safety = 0;
                while (p + 4 <= ol && safety++ < 48) {
                    int32_t eid = rd_i32(d + p); p += 4;
                    if (eid == -1) break; /* end of this main type */
                    if (p + 8 > ol) break;
                    int32_t aver = rd_i32(d + p);
                    if (aver != 400 && aver != 440) {
                        /* not an action list – back up and end this main */
                        p -= 4;
                        break;
                    }
                    p += 4;
                    int32_t acount = rd_i32(d + p); p += 4;
                    if (acount < 0 || acount > 64) {
                        break;
                    }

                    gm82_decoded_event *ev = NULL;
                    if (o->event_count < GM82_OBJ_EVENT_MAX) {
                        ev = &o->events[o->event_count];
                        ev->main_type = main;
                        ev->event_numb = eid;
                        ev->action_count = acount;
                        ev->action_name0[0] = 0;
                        ev->action_id0 = -1;
                        ev->action_arg0 = -1;
                        ev->code_snippet = NULL;
                        o->event_count++;
                    }

                    /* Skip/scan action bodies; resync to next -1 at end of this event's list.
                     * Each action has variable layout; scan for action_* and final -1. */
                    size_t window_end = p + 1200;
                    if (window_end > ol) window_end = ol;
                    size_t q = p;
                    int found_term = 0;
                    int actions_seen = 0;
                    while (q + 4 <= window_end && actions_seen < acount + 4) {
                        int32_t nlen = rd_i32(d + q);
                        if (nlen >= 8 && nlen < 48 && q + 4 + (size_t)nlen <= ol) {
                            const uint8_t *s = d + q + 4;
                            if (s[0]=='a' && s[1]=='c' && s[2]=='t' && s[3]=='i' &&
                                s[4]=='o' && s[5]=='n' && s[6]=='_') {
                                actions_seen++;
                                if (ev && !ev->action_name0[0]) {
                                    int L = nlen < 47 ? nlen : 47;
                                    memcpy(ev->action_name0, s, (size_t)L);
                                    ev->action_name0[L] = 0;
                                    if (q >= 8) {
                                        int32_t maybe_id = rd_i32(d + q - 8);
                                        if (maybe_id > 0 && maybe_id < 10000)
                                            ev->action_id0 = maybe_id;
                                    }
                                    size_t r = q + 4 + (size_t)nlen;
                                    for (int t = 0; t < 8 && r + 4 <= ol; t++, r += 4) {
                                        int32_t v = rd_i32(d + r);
                                        if (v >= 0 && v < 512) {
                                            ev->action_arg0 = v;
                                            break;
                                        }
                                    }
                                }
                            }
                        }
                        if (rd_i32(d + q) == -1) {
                            p = q; /* leave -1 for outer loop / next main */
                            found_term = 1;
                            break;
                        }
                        q += 4;
                    }
                    if (!found_term) {
                        /* advance past window; try continue main types */
                        p = window_end;
                        break;
                    }
                }
            }
        }

        /* Second pass: find execute-code GML strings in object blob */
        if (o->event_count > 0) {
            for (size_t q = 0; q + 8 < ol; q++) {
                int32_t nlen = rd_i32(d + q);
                if (nlen < 40 || nlen > 2000 || q + 4 + (size_t)nlen > ol) continue;
                const uint8_t *cs = d + q + 4;
                int ok = 1;
                for (int k = 0; k < nlen; k++) {
                    uint8_t ch = cs[k];
                    if (ch == 0) { ok = 0; break; }
                    if (ch < 9 || (ch > 13 && ch < 32 && ch != 9)) { ok = 0; break; }
                }
                if (!ok) continue;
                /* require GML markers */
                int has = 0;
                for (int k = 0; k + 8 < nlen; k++) {
                    if (cs[k]=='k' && cs[k+1]=='e' && cs[k+2]=='y' && cs[k+3]=='b' &&
                        cs[k+4]=='o' && cs[k+5]=='a' && cs[k+6]=='r' && cs[k+7]=='d') {
                        has = 1; break;
                    }
                    if (cs[k]=='p' && cs[k+1]=='l' && cs[k+2]=='a' && cs[k+3]=='c' &&
                        cs[k+4]=='e' && cs[k+5]=='_' && cs[k+6]=='f') {
                        has = 1; break;
                    }
                    if (cs[k]=='s' && cs[k+1]=='p' && cs[k+2]=='r' && cs[k+3]=='i' &&
                        cs[k+4]=='t' && cs[k+5]=='e' && cs[k+6]=='_' && cs[k+7]=='i') {
                        has = 1; break;
                    }
                    if (cs[k]=='v' && cs[k+1]=='s' && cs[k+2]=='p' && cs[k+3]=='e' &&
                        cs[k+4]=='e' && cs[k+5]=='d') {
                        has = 1; break;
                    }
                    if (cs[k]=='h' && cs[k+1]=='s' && cs[k+2]=='p' && cs[k+3]=='e' &&
                        cs[k+4]=='e' && cs[k+5]=='d') {
                        has = 1; break;
                    }
                }
                if (!has) continue;
                /* attach to first event lacking snippet, prefer step (3) if present */
                gm82_decoded_event *target = &o->events[0];
                for (int ei = 0; ei < o->event_count; ei++) {
                    if (o->events[ei].main_type == 3) { target = &o->events[ei]; break; }
                }
                if (!target->code_snippet || (int)strlen(target->code_snippet) < 20) {
                    free(target->code_snippet);
                    target->code_snippet = (char *)malloc((size_t)nlen + 1);
                    if (target->code_snippet) {
                        memcpy(target->code_snippet, cs, (size_t)nlen);
                        target->code_snippet[nlen] = 0;
                    }
                }
            }
        }
        free(d);
    }
    out->count = n;
    if (n > 0) {
        out->items = (gm82_decoded_object *)malloc((size_t)n * sizeof(*out->items));
        if (out->items) memcpy(out->items, tmp, (size_t)n * sizeof(*tmp));
        else out->count = 0;
    }
    free(tmp);
    return out->count;
}

/* Room ver 541: header + best-effort instance scan */
int gm82_decode_rooms_from_gmk(const uint8_t *data, size_t size, gm82_decoded_room_list *out) {
    memset(out, 0, sizeof(*out));
    if (!data || size < 12) return -1;
    gm82_decoded_room tmp[32];
    int n = 0;

    for (size_t i = 12; i + 2 < size; i++) {
        if (!(data[i]==0x78 && (data[i+1]==0x9c||data[i+1]==0xda||data[i+1]==0x01||data[i+1]==0x5e)))
            continue;
        size_t ol = 0;
        uint8_t *d = inflate_at(data + i, size - i, &ol);
        i += 16;
        if (!d || ol < 40) { free(d); continue; }
        if (rd_i32(d) != 1) { free(d); continue; }
        int32_t slen = rd_i32(d + 4);
        if (slen < 1 || slen > 48 || 8 + (size_t)slen + 20 > ol) { free(d); continue; }
        const uint8_t *ns = d + 8;
        int ok = 1;
        for (int k = 0; k < slen; k++) if (ns[k] < 32 || ns[k] > 126) ok = 0;
        if (!ok) { free(d); continue; }
        char name[64]; memcpy(name, ns, (size_t)slen); name[slen] = 0;
        /* room*, r001, r_menu*, level* – zelda uses r001 style */
        int name_ok = (strncmp(name, "room", 4) == 0) ||
                      (name[0] == 'r' && (name[1] == '_' || (name[1] >= '0' && name[1] <= '9'))) ||
                      (strncmp(name, "level", 5) == 0);
        if (!name_ok) { free(d); continue; }
        size_t off = 8 + (size_t)slen + 8;
        int32_t ver = rd_i32(d + off); off += 4;
        if (ver != 541 && ver != 800 && ver != 520) { free(d); continue; }
        /* caption string */
        int32_t clen = rd_i32(d + off); off += 4;
        if (clen < 0 || off + (size_t)clen > ol) { free(d); continue; }
        off += (size_t)clen;
        if (off + 20 > ol) { free(d); continue; }
        int32_t width = rd_i32(d + off); off += 4;
        int32_t height = rd_i32(d + off); off += 4;
        off += 8; /* snap */
        off += 4; /* isometric */
        int32_t speed = rd_i32(d + off); off += 4;
        if (width < 16 || height < 16 || width > 10000 || height > 10000) { free(d); continue; }

        if (n >= 32) { free(d); break; }
        gm82_decoded_room *r = &tmp[n++];
        memset(r, 0, sizeof(*r));
        strncpy(r->name, name, sizeof(r->name)-1);
        r->width = width; r->height = height; r->speed = speed;

        /* Instance harvest: GM format x,y,object,id,[creationCode string],locked.
           Primary: aligned scan. Observed stride ~24 bytes when creation code empty. */
        gm82_decoded_instance ibuf[512];
        int ic = 0;
        for (size_t j = off; j + 20 <= ol && ic < 512; j += 4) {
            int32_t x = rd_i32(d + j);
            int32_t y = rd_i32(d + j + 4);
            int32_t obj = rd_i32(d + j + 8);
            int32_t id = rd_i32(d + j + 12);
            if (x < 0 || y < 0 || x > width || y > height) continue;
            if (obj < 0 || obj > 200) continue;
            if (id < 100001 || id > 2000000) continue;
            int dup = 0;
            for (int k = 0; k < ic; k++)
                if (ibuf[k].id == id) { dup = 1; break; }
            if (dup) continue;
            ibuf[ic].x = x; ibuf[ic].y = y;
            ibuf[ic].object_index = obj; ibuf[ic].id = id;
            ic++;
            /* skip ahead if next field looks like empty creation code + locked */
            if (j + 24 <= ol) {
                int32_t maybe_clen = rd_i32(d + j + 16);
                if (maybe_clen == 0)
                    j += 20; /* will +=4 in loop → stride 24 */
            }
        }
        r->instance_count = ic;
        if (ic > 0) {
            r->instances = (gm82_decoded_instance *)malloc((size_t)ic * sizeof(*r->instances));
            if (r->instances) memcpy(r->instances, ibuf, (size_t)ic * sizeof(*ibuf));
            else r->instance_count = 0;
        }
        /* Tiles: after continuous instance block (stride 24 from first hit) */
        r->tiles = NULL; r->tile_count = 0;
        if (ic > 0) {
            /* find first instance offset again roughly – scan for tile count after instances */
            size_t scan = off;
            int found_first = -1;
            for (size_t j = off; j + 24 <= ol; j += 4) {
                int32_t x = rd_i32(d + j), y = rd_i32(d + j + 4);
                int32_t obj = rd_i32(d + j + 8), id = rd_i32(d + j + 12);
                if (id >= 100001 && id <= 2000000 && x >= 0 && y >= 0 && x <= width && y <= height && obj >= 0 && obj <= 200) {
                    found_first = (int)j; break;
                }
            }
            if (found_first >= 0) {
                size_t to = (size_t)found_first;
                int ninst = 0;
                while (to + 24 <= ol && ninst < ic + 5) {
                    int32_t id = rd_i32(d + to + 12);
                    if (id < 100001 || id > 2000000) break;
                    to += 24; ninst++;
                }
                if (to + 4 <= ol) {
                    int32_t tc = rd_i32(d + to);
                    if (tc > 0 && tc < 5000 && to + 4 + (size_t)tc * 40 <= ol) {
                        gm82_decoded_tile *tb = (gm82_decoded_tile *)malloc((size_t)tc * sizeof(*tb));
                        int nt = 0;
                        if (tb) {
                            size_t p = to + 4;
                            for (int ti = 0; ti < tc; ti++) {
                                int32_t tx = rd_i32(d + p);
                                int32_t ty = rd_i32(d + p + 4);
                                int32_t bg = rd_i32(d + p + 8);
                                int32_t xo = rd_i32(d + p + 12);
                                int32_t yo = rd_i32(d + p + 16);
                                int32_t tw = rd_i32(d + p + 20);
                                int32_t th = rd_i32(d + p + 24);
                                int32_t depth = rd_i32(d + p + 28);
                                int32_t tid = rd_i32(d + p + 32);
                                /* locked at +36 */
                                p += 40;
                                if (tx < -100 || ty < -100 || tx > width + 100 || ty > height + 100) continue;
                                if (tw < 1 || th < 1 || tw > 512 || th > 512) continue;
                                if (bg < 0 || bg > 64) continue;
                                tb[nt].x = tx; tb[nt].y = ty;
                                tb[nt].background_index = bg;
                                tb[nt].xo = xo; tb[nt].yo = yo;
                                tb[nt].width = tw; tb[nt].height = th;
                                tb[nt].depth = depth; tb[nt].id = tid;
                                nt++;
                            }
                            if (nt > 0) { r->tiles = tb; r->tile_count = nt; }
                            else free(tb);
                        }
                    }
                }
            }
        }
        free(d);
    }
    out->count = n;
    if (n > 0) {
        out->items = (gm82_decoded_room *)malloc((size_t)n * sizeof(*out->items));
        if (out->items) {
            memcpy(out->items, tmp, (size_t)n * sizeof(*tmp));
            /* ownership of instances pointers transferred */
            for (int i = 0; i < n; i++) { tmp[i].instances = NULL; tmp[i].tiles = NULL; }
        } else out->count = 0;
    }
    return out->count;
}
