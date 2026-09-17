#define _POSIX_C_SOURCE 200809L
/*
 * Phase 1.1 – Walk GMK 800 resource lists.
 * Based on observed structure of real samples + known GMK 800 layout.
 *
 * This does NOT claim full LateralGM parity yet.
 * It extracts what we can safely: counts, names where visible,
 * and compressed blobs for later materialize.
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <zlib.h>
#include <stdbool.h>

typedef struct {
    uint8_t *data;
    size_t   size;
    size_t   pos;
} gm_stream;

static uint32_t s_u32(gm_stream *s) {
    if (s->pos + 4 > s->size) return 0;
    uint32_t v = (uint32_t)s->data[s->pos] |
                 ((uint32_t)s->data[s->pos+1] << 8) |
                 ((uint32_t)s->data[s->pos+2] << 16) |
                 ((uint32_t)s->data[s->pos+3] << 24);
    s->pos += 4;
    return v;
}

static int32_t s_i32(gm_stream *s) { return (int32_t)s_u32(s); }

static char *s_str(gm_stream *s) {
    int32_t len = s_i32(s);
    if (len < 0 || (size_t)len > s->size - s->pos || len > 1024*1024) return NULL;
    char *out = (char *)malloc((size_t)len + 1);
    if (!out) return NULL;
    memcpy(out, s->data + s->pos, (size_t)len);
    out[len] = 0;
    s->pos += (size_t)len;
    return out;
}

static uint8_t *inflate_all(const uint8_t *src, size_t src_len, size_t *out_len) {
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
            size_t used = cap;
            cap *= 2;
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

typedef struct {
    int   sprite_count;
    int   background_count;
    int   sound_count;
    int   object_count;
    int   room_count;
    char  names[64][64];  /* first few resource names found */
    int   name_count;
    size_t total_inflated;
} walk_stats;

static void collect_name(walk_stats *st, const char *n) {
    if (!n || !n[0] || st->name_count >= 64) return;
    strncpy(st->names[st->name_count], n, 63);
    st->names[st->name_count][63] = 0;
    st->name_count++;
}

/* Scan inflated buffers for readable resource-like names */
static void scan_names(walk_stats *st, const uint8_t *buf, size_t len) {
    for (size_t i = 0; i + 8 < len; i++) {
        /* GM string: int32 length then ascii */
        int32_t L = (int32_t)(buf[i] | (buf[i+1]<<8) | (buf[i+2]<<16) | (buf[i+3]<<24));
        if (L < 3 || L > 48) continue;
        if (i + 4 + (size_t)L > len) continue;
        int ok = 1;
        for (int k = 0; k < L; k++) {
            unsigned char c = buf[i+4+k];
            if (c < 32 || c > 126) { ok = 0; break; }
        }
        if (!ok) continue;
        char tmp[64];
        memcpy(tmp, buf+i+4, (size_t)L);
        tmp[L] = 0;
        /* heuristic: resource names often start with letter and contain no spaces */
        if ((tmp[0] >= 'A' && tmp[0] <= 'Z') || (tmp[0] >= 'a' && tmp[0] <= 'z') || tmp[0] == '_') {
            if (!strchr(tmp, ' ') && !strchr(tmp, '/'))
                collect_name(st, tmp);
        }
    }
}

int gm82_gmk_walk_file(const char *path, walk_stats *st) {
    memset(st, 0, sizeof(*st));
    FILE *f = fopen(path, "rb");
    if (!f) return -1;
    fseek(f, 0, SEEK_END);
    long sz = ftell(f);
    fseek(f, 0, SEEK_SET);
    if (sz < 12 || sz > 200*1024*1024) { fclose(f); return -2; }
    uint8_t *data = (uint8_t *)malloc((size_t)sz);
    if (!data) { fclose(f); return -3; }
    if (fread(data, 1, (size_t)sz, f) != (size_t)sz) { free(data); fclose(f); return -4; }
    fclose(f);

    uint32_t magic = data[0]|(data[1]<<8)|(data[2]<<16)|(data[3]<<24);
    uint32_t ver   = data[4]|(data[5]<<8)|(data[6]<<16)|(data[7]<<24);
    if (magic != 1234321 || ver < 500 || ver > 810) {
        free(data);
        return -5;
    }

    /* Find and inflate all zlib streams */
    for (size_t i = 12; i + 2 < (size_t)sz; i++) {
        if (data[i] == 0x78 && (data[i+1] == 0x9c || data[i+1] == 0xda ||
                                data[i+1] == 0x01 || data[i+1] == 0x5e)) {
            size_t out_len = 0;
            uint8_t *inf = inflate_all(data + i, (size_t)sz - i, &out_len);
            if (inf && out_len > 16) {
                st->total_inflated += out_len;
                scan_names(st, inf, out_len);
                free(inf);
                /* skip ahead a bit to avoid re-parsing same stream */
                i += 64;
            }
        }
    }

    free(data);
    return 0;
}

#ifdef GM82_WALK_MAIN
int main(int argc, char **argv) {
    const char *path = argc > 1 ? argv[1] :
        "/home/workdir/artifacts/apk_extract/assets/www/samples/mario_bros.gmk";
    walk_stats st;
    int rc = gm82_gmk_walk_file(path, &st);
    printf("walk rc=%d inflated_total=%zu names=%d\n", rc, st.total_inflated, st.name_count);
    for (int i = 0; i < st.name_count && i < 40; i++)
        printf("  name[%d]=%s\n", i, st.names[i]);
    return rc == 0 ? 0 : 1;
}
#endif
