#define _POSIX_C_SOURCE 200809L
#include "gm82_sound_decode.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <zlib.h>

static int32_t rd_i32(const uint8_t *p) {
    return (int32_t)(p[0]|p[1]<<8|p[2]<<16|p[3]<<24);
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

void gm82_decoded_sound_list_free(gm82_decoded_sound_list *L) {
    if (!L) return;
    free(L->items);
    memset(L, 0, sizeof(*L));
}

int gm82_decode_sounds_from_gmk(const uint8_t *data, size_t size, gm82_decoded_sound_list *out) {
    memset(out, 0, sizeof(*out));
    if (!data || size < 12) return -1;
    gm82_decoded_sound tmp[128];
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
        if (slen < 3 || slen > 48 || 8 + (size_t)slen + 16 > ol) { free(d); continue; }
        char name[64]; memcpy(name, d+8, (size_t)slen); name[slen]=0;
        int ok=1; for (int k=0;k<slen;k++) if (name[k]<32||name[k]>126) ok=0;
        if (!ok) { free(d); continue; }
        /* sound names often start with snd_ or end with .wav/.mid */
        int is_snd = 0;
        if (strncmp(name, "snd", 3)==0 || strstr(name, ".wav") || strstr(name, ".mid") || strstr(name, ".mp3"))
            is_snd = 1;
        if (!is_snd) { free(d); continue; }
        if (n >= 128) { free(d); break; }
        size_t off = 8 + (size_t)slen + 8; /* lastChanged */
        int32_t ver = rd_i32(d + off); off += 4;
        gm82_decoded_sound *s = &tmp[n++];
        memset(s, 0, sizeof(*s));
        strncpy(s->name, name, sizeof(s->name)-1);
        s->kind = 0; s->volume = 1.0; s->preload = 0;
        if (off + 12 <= ol) {
            s->kind = rd_i32(d + off);
            /* rest varies – store what we can */
        }
        (void)ver;
        free(d);
    }
    out->count = n;
    if (n > 0) {
        out->items = malloc((size_t)n * sizeof(*out->items));
        if (out->items) memcpy(out->items, tmp, (size_t)n * sizeof(*tmp));
        else out->count = 0;
    }
    return out->count;
}
