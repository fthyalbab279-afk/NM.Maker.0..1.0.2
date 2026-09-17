#define _POSIX_C_SOURCE 200809L
#include "gm82_sprite_decode.h"
/*
 * Phase 1.2 – Real sprite frame pixel decoder for GMK 800.
 *
 * Proven working on mario_bros.gmk (extracted bloque_invisible 16x16,
 * mario_pierde 20x32, mini_mario_trans 31x50, castillo, ...).
 *
 * Pattern found in real data:
 *   [exists=1][strlen][name...][...meta...][800][width][height][w*h*4][BGRA pixels]
 *
 * NOT claimed to cover every GMK variant (zelda/plataformas still need more work).
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <zlib.h>
#include <stdbool.h>

void gm82_decoded_sprite_list_init(gm82_decoded_sprite_list *L) {
    memset(L, 0, sizeof(*L));
}

void gm82_decoded_sprite_list_free(gm82_decoded_sprite_list *L) {
    for (int i = 0; i < L->count; i++) free(L->frames[i].rgba);
    free(L->frames);
    memset(L, 0, sizeof(*L));
}

static bool list_push(gm82_decoded_sprite_list *L, const char *name,
                      int32_t w, int32_t h, const uint8_t *bgra, size_t bgra_len) {
    if ((size_t)(w * h * 4) != bgra_len) return false;
    if (L->count >= L->capacity) {
        int nc = L->capacity ? L->capacity * 2 : 16;
        gm82_decoded_frame *nf = (gm82_decoded_frame *)realloc(L->frames, (size_t)nc * sizeof(*nf));
        if (!nf) return false;
        L->frames = nf;
        L->capacity = nc;
    }
    gm82_decoded_frame *f = &L->frames[L->count];
    memset(f, 0, sizeof(*f));
    strncpy(f->name, name ? name : "?", sizeof(f->name) - 1);
    f->width = w;
    f->height = h;
    f->rgba_size = bgra_len;
    f->rgba = (uint8_t *)malloc(bgra_len);
    if (!f->rgba) return false;
    /* BGRA → RGBA */
    for (int32_t i = 0; i < w * h; i++) {
        f->rgba[i*4+0] = bgra[i*4+2]; /* R */
        f->rgba[i*4+1] = bgra[i*4+1]; /* G */
        f->rgba[i*4+2] = bgra[i*4+0]; /* B */
        f->rgba[i*4+3] = bgra[i*4+3]; /* A */
    }
    L->count++;
    return true;
}

static uint8_t *inflate_at(const uint8_t *src, size_t src_len, size_t *out_len) {
    *out_len = 0;
    z_stream strm; memset(&strm, 0, sizeof(strm));
    if (inflateInit2(&strm, 15) != Z_OK) return NULL;
    size_t cap = src_len * 8 + 256;
    uint8_t *dst = (uint8_t *)malloc(cap);
    if (!dst) { inflateEnd(&strm); return NULL; }
    strm.next_in = (Bytef *)src;
    strm.avail_in = (uInt)src_len;
    strm.next_out = dst;
    strm.avail_out = (uInt)cap;
    int ret;
    while ((ret = inflate(&strm, Z_NO_FLUSH)) == Z_OK) {
        if (strm.avail_out == 0) {
            size_t used = cap; cap *= 2;
            uint8_t *nd = (uint8_t *)realloc(dst, cap);
            if (!nd) { free(dst); inflateEnd(&strm); return NULL; }
            dst = nd;
            strm.next_out = dst + used;
            strm.avail_out = (uInt)(cap - used);
        }
    }
    if (ret != Z_STREAM_END) { free(dst); inflateEnd(&strm); return NULL; }
    *out_len = strm.total_out;
    inflateEnd(&strm);
    return dst;
}

static int32_t rd_i32(const uint8_t *p) {
    return (int32_t)(p[0] | (p[1]<<8) | (p[2]<<16) | (p[3]<<24));
}

/* Extract BGRA frames from one inflated blob.
 * Primary: ver=800 or ver=710 followed by w/h/dlen/pixels.
 * Secondary: plausible w/h/dlen when preceded by exists/version flag.
 * This increases coverage for zelda / plataformas style GMKs. */
static void scan_blob(gm82_decoded_sprite_list *L, const uint8_t *d, size_t len) {
    /* Byte-step scan: after variable-length names the 800 marker is often
       not 4-byte aligned. Required for zelda / plataformas style resources. */
    for (size_t j = 0; j + 20 < len; j++) {
        int32_t ver = rd_i32(d + j);
        if (ver != 800 && ver != 710) continue;

        int32_t w = rd_i32(d + j + 4);
        int32_t h = rd_i32(d + j + 8);
        int32_t dlen = rd_i32(d + j + 12);
        size_t pix_off = j + 16;

        if (w < 1 || h < 1 || w > 2048 || h > 2048) continue;
        if (dlen != w * h * 4) continue;
        if (pix_off + (size_t)dlen > len) continue;

        char name[64] = "?";
        for (size_t back = j; back >= 4; back--) {
            int32_t slen = rd_i32(d + back - 4);
            if (slen >= 1 && slen <= 62 && back + (size_t)slen <= j) {
                const uint8_t *s = d + back;
                int ok = 1;
                for (int k = 0; k < slen; k++) {
                    if (s[k] < 32 || s[k] > 126) { ok = 0; break; }
                }
                if (ok) {
                    memcpy(name, s, (size_t)slen);
                    name[slen] = 0;
                    break;
                }
            }
            if (back < 5) break;
        }
        list_push(L, name, w, h, d + pix_off, (size_t)dlen);
        j = pix_off + (size_t)dlen - 1; /* skip past frame */
    }
}

int gm82_decode_sprites_from_gmk(const uint8_t *data, size_t size,
                                 gm82_decoded_sprite_list *out) {
    gm82_decoded_sprite_list_init(out);
    if (!data || size < 12) return -1;
    int32_t magic = rd_i32(data);
    int32_t ver = rd_i32(data + 4);
    if (magic != 1234321 || ver < 500 || ver > 810) return -2;

    for (size_t i = 12; i + 2 < size; i++) {
        if (data[i] == 0x78 && (data[i+1] == 0x9c || data[i+1] == 0xda ||
                                data[i+1] == 0x01 || data[i+1] == 0x5e)) {
            size_t out_len = 0;
            uint8_t *inf = inflate_at(data + i, size - i, &out_len);
            if (inf && out_len > 32) {
                scan_blob(out, inf, out_len);
                free(inf);
            }
            i += 16; /* skip ahead */
        }
    }
    return out->count;
}

int gm82_decode_sprites_from_file(const char *path, gm82_decoded_sprite_list *out) {
    gm82_decoded_sprite_list_init(out);
    FILE *f = fopen(path, "rb");
    if (!f) return -1;
    fseek(f, 0, SEEK_END);
    long sz = ftell(f);
    fseek(f, 0, SEEK_SET);
    if (sz < 12 || sz > 200*1024*1024) { fclose(f); return -2; }
    uint8_t *buf = (uint8_t *)malloc((size_t)sz);
    if (!buf) { fclose(f); return -3; }
    if (fread(buf, 1, (size_t)sz, f) != (size_t)sz) { free(buf); fclose(f); return -4; }
    fclose(f);
    int n = gm82_decode_sprites_from_gmk(buf, (size_t)sz, out);
    free(buf);
    return n;
}

#ifdef GM82_SPRITE_MAIN
int main(int argc, char **argv) {
    const char *path = argc > 1 ? argv[1] :
        "/home/workdir/artifacts/apk_extract/assets/www/samples/mario_bros.gmk";
    gm82_decoded_sprite_list L;
    int n = gm82_decode_sprites_from_file(path, &L);
    printf("decoded_frames=%d\n", n);
    for (int i = 0; i < L.count && i < 20; i++) {
        printf("  [%d] %s %dx%d rgba=%zu\n", i, L.frames[i].name,
               L.frames[i].width, L.frames[i].height, L.frames[i].rgba_size);
    }
    /* write first frame as PPM for visual proof */
    if (L.count > 0) {
        gm82_decoded_frame *f = &L.frames[0];
        char fn[256];
        snprintf(fn, sizeof(fn), "/tmp/decoded_%s.ppm", f->name);
        FILE *o = fopen(fn, "wb");
        if (o) {
            fprintf(o, "P6\n%d %d\n255\n", f->width, f->height);
            for (int i = 0; i < f->width * f->height; i++) {
                fputc(f->rgba[i*4+0], o);
                fputc(f->rgba[i*4+1], o);
                fputc(f->rgba[i*4+2], o);
            }
            fclose(o);
            printf("wrote %s\n", fn);
        }
    }
    gm82_decoded_sprite_list_free(&L);
    return n > 0 ? 0 : 1;
}
#endif


void gm82_sprite_group_list_free(gm82_sprite_group_list *G) {
    if (!G) return;
    free(G->items);
    memset(G, 0, sizeof(*G));
}

void gm82_sprite_groups_build(const gm82_decoded_sprite_list *frames, gm82_sprite_group_list *out) {
    memset(out, 0, sizeof(*out));
    if (!frames || frames->count <= 0) return;
    gm82_sprite_group tmp[512];
    int n = 0;
    int i = 0;
    while (i < frames->count && n < 512) {
        const char *name = frames->frames[i].name;
        int start = i;
        while (i < frames->count && strcmp(frames->frames[i].name, name) == 0) i++;
        gm82_sprite_group *g = &tmp[n++];
        memset(g, 0, sizeof(*g));
        strncpy(g->name, name, sizeof(g->name)-1);
        g->frame_start = start;
        g->frame_count = i - start;
        g->width = frames->frames[start].width;
        g->height = frames->frames[start].height;
    }
    out->count = n;
    if (n > 0) {
        out->items = (gm82_sprite_group *)malloc((size_t)n * sizeof(*out->items));
        if (out->items) memcpy(out->items, tmp, (size_t)n * sizeof(*tmp));
        else out->count = 0;
    }
}

int gm82_sprite_group_index_for_frame(const gm82_sprite_group_list *G, int frame_index) {
    if (!G) return -1;
    for (int i = 0; i < G->count; i++) {
        int s = G->items[i].frame_start;
        int e = s + G->items[i].frame_count;
        if (frame_index >= s && frame_index < e) return i;
    }
    return -1;
}

int gm82_sprite_resolve_frame(const gm82_sprite_group_list *G, int sprite_index, int image_index) {
    if (!G || sprite_index < 0) return sprite_index;
    /* If sprite_index is a frame index, find its group and apply image_index */
    int gi = gm82_sprite_group_index_for_frame(G, sprite_index);
    if (gi < 0) return sprite_index;
    const gm82_sprite_group *g = &G->items[gi];
    if (g->frame_count <= 0) return sprite_index;
    int idx = image_index % g->frame_count;
    if (idx < 0) idx += g->frame_count;
    return g->frame_start + idx;
}
