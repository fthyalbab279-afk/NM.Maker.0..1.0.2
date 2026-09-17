#define _POSIX_C_SOURCE 200809L
#include "gm82_path_decode.h"
#include <stdlib.h>
#include <string.h>
#include <zlib.h>

static int32_t rd_i32(const uint8_t *p) {
    return (int32_t)(p[0]|p[1]<<8|p[2]<<16|p[3]<<24);
}
static double rd_f64(const uint8_t *p) {
    double v; memcpy(&v, p, 8); return v;
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

int gm82_decode_paths_from_gmk(const uint8_t *data, size_t size, gm82_path_list *out) {
    gm82_path_list_init(out);
    if (!data || size < 12) return -1;
    for (size_t i = 12; i + 2 < size; i++) {
        if (!(data[i]==0x78 && (data[i+1]==0x9c||data[i+1]==0xda||data[i+1]==0x01||data[i+1]==0x5e)))
            continue;
        size_t ol = 0;
        uint8_t *d = inflate_at(data + i, size - i, &ol);
        i += 16;
        if (!d || ol < 40) { free(d); continue; }
        if (rd_i32(d) != 1) { free(d); continue; }
        int32_t slen = rd_i32(d + 4);
        if (slen < 4 || slen > 48 || 8 + (size_t)slen + 20 > ol) { free(d); continue; }
        char name[64];
        memcpy(name, d + 8, (size_t)slen); name[slen] = 0;
        int ok = 1;
        for (int k = 0; k < slen; k++) if (name[k] < 32 || name[k] > 126) ok = 0;
        if (!ok || (strncmp(name, "path", 4) != 0 && strncmp(name, "pth_", 4) != 0)) {
            free(d); continue;
        }
        /* After name + lastChanged(8) + ver(4): smooth, closed, precision, point count */
        size_t off = 8 + (size_t)slen + 8 + 4;
        if (off + 16 > ol) { free(d); continue; }
        int32_t smooth = rd_i32(d + off); off += 4;
        int32_t closed = rd_i32(d + off); off += 4;
        int32_t prec = rd_i32(d + off); off += 4;
        int32_t npoints = rd_i32(d + off); off += 4;
        (void)smooth; (void)prec;
        if (npoints < 0 || npoints > 512) { free(d); continue; }
        int pi = gm82_path_add(out, name, closed);
        if (pi < 0) { free(d); break; }
        for (int p = 0; p < npoints && off + 24 <= ol; p++) {
            /* GM8 path point: x(f64), y(f64), speed(f64) */
            double x = rd_f64(d + off); off += 8;
            double y = rd_f64(d + off); off += 8;
            double spd = rd_f64(d + off); off += 8;
            gm82_path_add_point(out, pi, x, y, spd);
        }
        free(d);
    }
    return out->count;
}
