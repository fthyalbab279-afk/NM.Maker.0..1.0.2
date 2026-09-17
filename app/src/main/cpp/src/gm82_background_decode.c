#define _POSIX_C_SOURCE 200809L
#include "gm82_background_decode.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <stdint.h>
#include <zlib.h>
#include <stdbool.h>

void gm82_decoded_background_list_init(gm82_decoded_background_list *L) {
    memset(L, 0, sizeof(*L));
}

void gm82_decoded_background_list_free(gm82_decoded_background_list *L) {
    if (!L) return;
    for (int i = 0; i < L->count; i++) free(L->items[i].rgba);
    free(L->items);
    memset(L, 0, sizeof(*L));
}

static int32_t rd_i32(const uint8_t *p) {
    return (int32_t)(p[0] | (p[1]<<8) | (p[2]<<16) | (p[3]<<24));
}

static uint8_t *inflate_at(const uint8_t *src, size_t src_len, size_t *out_len) {
    *out_len = 0;
    z_stream strm; memset(&strm, 0, sizeof(strm));
    if (inflateInit2(&strm, 15) != Z_OK) return NULL;
    size_t cap = src_len * 8 + 256;
    uint8_t *dst = (uint8_t *)malloc(cap);
    if (!dst) { inflateEnd(&strm); return NULL; }
    strm.next_in = (Bytef *)src; strm.avail_in = (uInt)src_len;
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

static bool list_push(gm82_decoded_background_list *L, const char *name,
                      int32_t w, int32_t h, const uint8_t *bgra, size_t len) {
    if ((size_t)(w*h*4) != len) return false;
    /* Deduplication: check if background with same name is already loaded */
    for (int i = 0; i < L->count; i++) {
        if (name && strcmp(L->items[i].name, name) == 0) {
            return true; /* already stored */
        }
    }
    if (L->count >= L->capacity) {
        int nc = L->capacity ? L->capacity * 2 : 16;
        void *n = realloc(L->items, (size_t)nc * sizeof(*L->items));
        if (!n) return false;
        L->items = (gm82_decoded_background *)n;
        L->capacity = nc;
    }
    gm82_decoded_background *b = &L->items[L->count];
    memset(b, 0, sizeof(*b));
    strncpy(b->name, name ? name : "?", sizeof(b->name)-1);
    b->width = w; b->height = h; b->rgba_size = len;
    b->rgba = (uint8_t *)malloc(len);
    if (!b->rgba) return false;
    for (int32_t i = 0; i < w*h; i++) {
        b->rgba[i*4+0] = bgra[i*4+2];
        b->rgba[i*4+1] = bgra[i*4+1];
        b->rgba[i*4+2] = bgra[i*4+0];
        b->rgba[i*4+3] = bgra[i*4+3];
    }
    L->count++;
    return true;
}

static void scan_bg_blob(gm82_decoded_background_list *L, const uint8_t *d, size_t len) {
    if (len < 40) return;
    int32_t exists = rd_i32(d);
    if (exists != 1) return;
    int32_t slen = rd_i32(d + 4);
    if (slen < 1 || slen > 64 || 8 + (size_t)slen + 28 > len) return;
    const uint8_t *ns = d + 8;
    for (int k = 0; k < slen; k++) if (ns[k] < 32 || ns[k] > 126) return;
    char name[64];
    memcpy(name, ns, (size_t)slen); name[slen] = 0;
    size_t off = 8 + (size_t)slen + 8; /* skip double last_changed */
    if (off + 4 > len) return;
    int32_t ver = rd_i32(d + off);
    if (ver != 710 && ver != 800 && ver != 543 && ver != 400) return;
    off += 4;
    
    /* Background format:
     * is_tile(i32), tw(i32), th(i32), tox(i32), toy(i32), tsx(i32), tsy(i32) [28 bytes]
     * sub_ver(i32=800), bw(i32), bh(i32), dlen(i32), BGRA pixels...
     */
    if (off + 28 + 16 <= len) {
        size_t bp = off + 28;
        int32_t bver = rd_i32(d + bp);
        int32_t bw = rd_i32(d + bp + 4);
        int32_t bh = rd_i32(d + bp + 8);
        int32_t bdlen = rd_i32(d + bp + 12);
        if (bver == 800 && bw >= 1 && bh >= 1 && bw <= 8192 && bh <= 8192 &&
            (size_t)bdlen == (size_t)(bw * bh * 4) && bp + 16 + (size_t)bdlen <= len) {
            list_push(L, name, bw, bh, d + bp + 16, (size_t)bdlen);
            return;
        }
    }

    /* Fallback search */
    int best_j = -1, best_w = 0, best_h = 0, best_dlen = 0;
    for (size_t j = off; j + 16 < len; j += 4) {
        int32_t v = rd_i32(d + j);
        if (v != 800) continue;
        int32_t w = rd_i32(d + j + 4);
        int32_t h = rd_i32(d + j + 8);
        int32_t dlen = rd_i32(d + j + 12);
        if (w < 8 || h < 8 || w > 8192 || h > 8192) continue;
        if (dlen != w * h * 4) continue;
        if (j + 16 + (size_t)dlen > len) continue;
        if (best_j < 0 || w * h > best_w * best_h) {
            best_j = (int)j; best_w = w; best_h = h; best_dlen = dlen;
        }
    }
    if (best_j >= 0) {
        list_push(L, name, best_w, best_h, d + best_j + 16, (size_t)best_dlen);
    }
}

int gm82_decode_backgrounds_from_gmk(const uint8_t *data, size_t size,
                                     gm82_decoded_background_list *out) {
    gm82_decoded_background_list_init(out);
    if (!data || size < 12) return -1;
    if (rd_i32(data) != 1234321) return -2;
    for (size_t i = 12; i + 2 < size; i++) {
        if (data[i] == 0x78 && (data[i+1] == 0x9c || data[i+1] == 0xda ||
                                data[i+1] == 0x01 || data[i+1] == 0x5e)) {
            size_t ol = 0;
            uint8_t *inf = inflate_at(data + i, size - i, &ol);
            if (inf && ol > 40) {
                scan_bg_blob(out, inf, ol);
                free(inf);
            }
            i += 16;
        }
    }
    return out->count;
}

int gm82_decode_backgrounds_from_file(const char *path,
                                      gm82_decoded_background_list *out) {
    gm82_decoded_background_list_init(out);
    FILE *f = fopen(path, "rb");
    if (!f) return -1;
    fseek(f, 0, SEEK_END); long sz = ftell(f); fseek(f, 0, SEEK_SET);
    if (sz < 12) { fclose(f); return -2; }
    uint8_t *buf = (uint8_t *)malloc((size_t)sz);
    if (!buf) { fclose(f); return -3; }
    if (fread(buf, 1, (size_t)sz, f) != (size_t)sz) { free(buf); fclose(f); return -4; }
    fclose(f);
    int n = gm82_decode_backgrounds_from_gmk(buf, (size_t)sz, out);
    free(buf);
    return n;
}
