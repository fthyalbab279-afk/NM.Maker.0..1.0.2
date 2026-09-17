#define _POSIX_C_SOURCE 200809L
#include "gm82_timeline_decode.h"
#include <stdlib.h>
#include <string.h>
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

int gm82_decode_timelines_from_gmk(const uint8_t *data, size_t size, gm82_timeline_list *out) {
    gm82_timeline_list_init(out);
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
        if (slen < 4 || slen > 48 || 8+(size_t)slen+16 > ol) { free(d); continue; }
        char name[64]; memcpy(name, d+8, (size_t)slen); name[slen]=0;
        int ok=1; for(int k=0;k<slen;k++) if(name[k]<32||name[k]>126) ok=0;
        if (!ok) { free(d); continue; }
        if (strncmp(name,"time",4)!=0 && strncmp(name,"tl_",3)!=0) { free(d); continue; }
        size_t off = 8+(size_t)slen+8+4; /* lastChanged + ver */
        if (off+4 > ol) { free(d); continue; }
        int32_t nmoments = rd_i32(d + off); off += 4;
        if (nmoments < 0 || nmoments > 256) { free(d); continue; }
        int ti = gm82_timeline_add(out, name);
        if (ti < 0) { free(d); break; }
        for (int m = 0; m < nmoments && off+8 <= ol; m++) {
            int32_t step = rd_i32(d + off); off += 4;
            /* event count for this moment – skip nested structure best-effort */
            int32_t evcount = rd_i32(d + off); off += 4;
            char code[128] = "";
            /* try read one string as code if present */
            if (off+4 <= ol) {
                int32_t n = rd_i32(d + off);
                if (n > 0 && n < 120 && off+4+(size_t)n <= ol) {
                    memcpy(code, d+off+4, (size_t)n); code[n]=0;
                    off += 4+(size_t)n;
                }
            }
            /* skip remaining event payload roughly */
            for (int e = 0; e < evcount && e < 32; e++) {
                if (off + 16 > ol) break;
                off += 16; /* rough skip */
            }
            gm82_timeline_add_moment(out, ti, step, code);
        }
        free(d);
    }
    return out->count;
}
