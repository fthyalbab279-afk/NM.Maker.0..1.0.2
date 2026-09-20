#define _POSIX_C_SOURCE 200809L
#include "gm82_script.h"
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <zlib.h>

void gm82_script_list_init(gm82_script_list *L) {
    memset(L, 0, sizeof(*L));
}

int gm82_script_add(gm82_script_list *L, const char *name, const char *code) {
    if (!L || L->count >= GM82_SCRIPT_MAX) return -1;
    gm82_script *s = &L->items[L->count];
    memset(s, 0, sizeof(*s));
    if (name) strncpy(s->name, name, sizeof(s->name)-1);
    if (code) strncpy(s->code, code, sizeof(s->code)-1);
    return L->count++;
}

int gm82_script_find(const gm82_script_list *L, const char *name) {
    if (!L || !name) return -1;
    for (int i = 0; i < L->count; i++)
        if (strcmp(L->items[i].name, name) == 0) return i;
    return -1;
}

static int32_t rd_i32(const uint8_t *p) {
    return (int32_t)(p[0]|p[1]<<8|p[2]<<16|p[3]<<24);
}

static int is_audio_filename(const char *s, int len) {
    if (len < 4) return 0;
    const char *exts[] = { ".wav", ".mid", ".mp3", ".ogg", ".wma", ".aiff" };
    for (int i = 0; i < 6; i++) {
        int elen = (int)strlen(exts[i]);
        if (len >= elen) {
            const char *end = s + len - elen;
            int match = 1;
            for (int k = 0; k < elen; k++) {
                char c1 = end[k];
                char c2 = exts[i][k];
                if (c1 >= 'A' && c1 <= 'Z') c1 = (char)(c1 - 'A' + 'a');
                if (c1 != c2) { match = 0; break; }
            }
            if (match) return 1;
        }
    }
    return 0;
}

static uint8_t *inflate_at(const uint8_t *src, size_t n, size_t *ol) {
    *ol = 0;
    z_stream strm; memset(&strm, 0, sizeof(strm));
    if (inflateInit2(&strm, 15) != Z_OK) return NULL;
    size_t cap = n * 8 + 256;
    uint8_t *dst = malloc(cap);
    if (!dst) { inflateEnd(&strm); return NULL; }
    strm.next_in = (Bytef*)src; strm.avail_in = (uInt)n;
    strm.next_out = dst; strm.avail_out = (uInt)cap;
    int ret;
    while ((ret = inflate(&strm, Z_NO_FLUSH)) == Z_OK) {
        if (strm.avail_out == 0) {
            size_t used = cap; cap *= 2;
            uint8_t *nd = realloc(dst, cap);
            if (!nd) { free(dst); inflateEnd(&strm); return NULL; }
            dst = nd; strm.next_out = dst + used; strm.avail_out = (uInt)(cap - used);
        }
    }
    if (ret != Z_STREAM_END) { free(dst); inflateEnd(&strm); return NULL; }
    *ol = strm.total_out; inflateEnd(&strm); return dst;
}

int gm82_decode_scripts_from_gmk(const uint8_t *data, size_t size, gm82_script_list *out) {
    gm82_script_list_init(out);
    if (!data || size < 12) return -1;
    for (size_t i = 12; i + 2 < size; i++) {
        if (!(data[i]==0x78 && (data[i+1]==0x9c||data[i+1]==0xda||data[i+1]==0x01||data[i+1]==0x5e)))
            continue;
        size_t ol = 0;
        uint8_t *d = inflate_at(data + i, size - i, &ol);
        i += 16;
        if (!d || ol < 30) { free(d); continue; }
        if (rd_i32(d) != 1 && rd_i32(d) != 800) { free(d); continue; }
        int32_t slen = rd_i32(d + 4);
        if (slen < 2 || slen > 48 || 8+(size_t)slen+8 > ol) { free(d); continue; }
        char name[64]; memcpy(name, d+8, (size_t)slen); name[slen]=0;
        int ok=1; for(int k=0;k<slen;k++) if(name[k]<32||name[k]>126) ok=0;
        if (!ok) { free(d); continue; }

        /* Search for long GML-like string in this script resource blob */
        const char *best_code = NULL;
        int best_len = 0;
        for (size_t j = 8 + (size_t)slen; j + 4 < ol; j++) {
            int32_t n = rd_i32(d + j);
            if (n >= 10 && n < GM82_SCRIPT_CODE_MAX - 1 && j + 4 + (size_t)n <= ol) {
                const uint8_t *s = d + j + 4;
                int printable = 1;
                for (int k = 0; k < n; k++) {
                    unsigned char c = s[k];
                    if (!(c >= 32 && c < 127) && c != 9 && c != 10 && c != 13) { printable = 0; break; }
                }
                if (printable && !is_audio_filename((const char *)s, n) && n > best_len) {
                    best_len = n;
                    best_code = (const char *)s;
                }
            }
        }
        /* Filter out audio file names like .wav, .mid, .mp3, .ogg, .wma */
        int is_audio = 0;
        if (best_code && best_len >= 4) {
            const char *exts[] = { ".wav", ".mid", ".mp3", ".ogg", ".wma", ".midi", NULL };
            for (int e = 0; exts[e]; e++) {
                int elen = (int)strlen(exts[e]);
                if (best_len >= elen) {
                    if (strncasecmp(best_code + best_len - elen, exts[e], (size_t)elen) == 0) {
                        is_audio = 1;
                        break;
                    }
                }
            }
        }

        if (best_code && best_len >= 10 && !is_audio) {
            char code[GM82_SCRIPT_CODE_MAX];
            int ncopy = best_len < GM82_SCRIPT_CODE_MAX - 1 ? best_len : GM82_SCRIPT_CODE_MAX - 1;
            memcpy(code, best_code, (size_t)ncopy);
            code[ncopy] = 0;
            gm82_script_add(out, name, code);
        }
        free(d);
    }

    /* Fallback: if no standard script headers found, harvest GML fragments as scripts */
    if (out->count == 0) {
        gm82_gml_fragment_list frags;
        gm82_harvest_gml_fragments_from_gmk(data, size, &frags);
        for (int f = 0; f < frags.count && out->count < GM82_SCRIPT_MAX; f++) {
            char sname[64];
            snprintf(sname, sizeof(sname), "scr_gml_%d", f);
            gm82_script_add(out, sname, frags.items[f].code);
        }
    }
    return out->count;
}

void gm82_gml_fragment_list_init(gm82_gml_fragment_list *L) {
    memset(L, 0, sizeof(*L));
}

static int looks_like_gml(const char *t, int n) {
    /* require at least one strong token */
    static const char *keys[] = {
        "instance_", "motion_", "keyboard_", "sprite_index", "hspeed", "vspeed",
        "room_goto", "place_free", "place_meeting", "image_index", "gravity",
        "if ", "x =", "y =", "x=", "y=", "argument0", NULL
    };
    for (int k = 0; keys[k]; k++) {
        const char *p = keys[k];
        int plen = (int)strlen(p);
        for (int i = 0; i + plen <= n; i++) {
            int m = 1;
            for (int j = 0; j < plen; j++) {
                char a = t[i+j], b = p[j];
                if (a >= 'A' && a <= 'Z') a = (char)(a - 'A' + 'a');
                if (b >= 'A' && b <= 'Z') b = (char)(b - 'A' + 'a');
                if (a != b) { m = 0; break; }
            }
            if (m) return 1;
        }
    }
    return 0;
}

int gm82_harvest_gml_fragments_from_gmk(const uint8_t *data, size_t size,
                                          gm82_gml_fragment_list *out) {
    gm82_gml_fragment_list_init(out);
    if (!data || size < 12) return -1;
    for (size_t i = 12; i + 2 < size; i++) {
        if (!(data[i]==0x78 && (data[i+1]==0x9c||data[i+1]==0xda||
                                data[i+1]==0x01||data[i+1]==0x5e)))
            continue;
        size_t ol = 0;
        uint8_t *d = inflate_at(data + i, size - i, &ol);
        i += 16;
        if (!d || ol < 16) { free(d); continue; }
        for (size_t j = 0; j + 8 < ol; j++) {
            int32_t n = rd_i32(d + j);
            if (n < 12 || n > GM82_GML_FRAG_CODE - 1) continue;
            if (j + 4 + (size_t)n > ol) continue;
            const uint8_t *s = d + j + 4;
            int good = 1;
            for (int k = 0; k < n; k++) {
                unsigned char c = s[k];
                if (!(c >= 32 && c < 127) && c != 9 && c != 10 && c != 13) {
                    good = 0; break;
                }
            }
            if (!good) continue;
            if (!looks_like_gml((const char *)s, n)) continue;
            /* dedupe by prefix */
            int dup = 0;
            for (int u = 0; u < out->count; u++) {
                if (out->items[u].length == n &&
                    memcmp(out->items[u].code, s, (size_t)(n < 32 ? n : 32)) == 0) {
                    dup = 1; break;
                }
            }
            if (dup) continue;
            if (out->count >= GM82_GML_FRAG_MAX) { free(d); return out->count; }
            gm82_gml_fragment *f = &out->items[out->count++];
            memcpy(f->code, s, (size_t)n);
            f->code[n] = 0;
            f->length = n;
            j += 4 + (size_t)n - 1;
        }
        free(d);
    }
    return out->count;
}
