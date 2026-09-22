#define _POSIX_C_SOURCE 200809L
#include "gm82_gml_builtins.h"
#include "gml_vm.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <math.h>
#include <sys/stat.h>
#include <sys/types.h>

double nor_import_format_native(const char *path) {
    return (path && path[0]) ? 1.0 : 0.0;
}

double nor_validate_rom_native(const char *path, double kind) {
    (void)kind;
    if (!path || !path[0]) return 0.0;
    FILE *f = fopen(path, "rb");
    if (!f) return 0.0;
    fclose(f);
    return 1.0;
}

static void ensure_dir_exists(const char *path) {
    if (!path || !*path) return;
    char buf[256];
    strncpy(buf, path, sizeof(buf)-1);
    buf[sizeof(buf)-1] = 0;
    char *p = buf;
    if (*p == '/') p++;
    while (*p) {
        if (*p == '/') {
            *p = 0;
            mkdir(buf, 0755);
            *p = '/';
        }
        p++;
    }
    mkdir(buf, 0755);
}

static int write_stub_rom(const char *path, const char *header, size_t hlen) {
    if (!path) return 0;
    char parent_dir[256];
    strncpy(parent_dir, path, sizeof(parent_dir)-1);
    parent_dir[sizeof(parent_dir)-1] = 0;
    char *slash = strrchr(parent_dir, '/');
    if (slash) {
        *slash = 0;
        ensure_dir_exists(parent_dir);
    }
    FILE *f = fopen(path, "wb");
    if (!f) return 0;
    fwrite(header, 1, hlen, f);
    fclose(f);
    return 1;
}

double nor_export_nes_native(const char *project, const char *output) {
    (void)project;
    static const unsigned char nes_hdr[16] = {'N','E','S',0x1A, 2, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0};
    return write_stub_rom(output, (const char *)nes_hdr, sizeof(nes_hdr)) ? 1.0 : 0.0;
}

double nor_export_gbc_native(const char *project, const char *output) {
    (void)project;
    static const unsigned char gbc_hdr[16] = {'G','B','C',0x00, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0};
    return write_stub_rom(output, (const char *)gbc_hdr, sizeof(gbc_hdr)) ? 1.0 : 0.0;
}

double nor_export_gba_native(const char *project, const char *output) {
    (void)project;
    static const unsigned char gba_hdr[16] = {'G','B','A',0x00, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0};
    return write_stub_rom(output, (const char *)gba_hdr, sizeof(gba_hdr)) ? 1.0 : 0.0;
}

int gm82_native_call(void *userdata, const char *name, const gml_value *args, size_t count, gml_value *out) {
    (void)userdata;
    if (!name || !out) return 0;

    if (strcmp(name, "point_distance") == 0 && count >= 4) {
        double d = gml_point_distance(args[0].real, args[1].real, args[2].real, args[3].real);
        *out = gml_value_real(d);
        return 1;
    }
    if (strcmp(name, "point_direction") == 0 && count >= 4) {
        double d = gml_point_direction(args[0].real, args[1].real, args[2].real, args[3].real);
        *out = gml_value_real(d);
        return 1;
    }
    if (strcmp(name, "string_length") == 0 && count >= 1) {
        const char *s = args[0].kind == GML_V_STRING && args[0].string ? args[0].string : "";
        *out = gml_value_real((double)strlen(s));
        return 1;
    }
    if (strcmp(name, "string_copy") == 0 && count >= 3) {
        const char *s = args[0].kind == GML_V_STRING && args[0].string ? args[0].string : "";
        int start = (int)args[1].real;
        int len = (int)args[2].real;
        size_t nstr = strlen(s);
        if (start < 1) start = 1;
        if (len < 0) len = 0;
        size_t begin = (size_t)(start - 1);
        if (begin > nstr) begin = nstr;
        if ((size_t)len > nstr - begin) len = (int)(nstr - begin);
        char *buf = (char *)malloc((size_t)len + 1);
        if (buf) {
            memcpy(buf, s + begin, (size_t)len);
            buf[len] = 0;
            *out = gml_value_string(buf);
            free(buf);
        } else {
            *out = gml_value_string("");
        }
        return 1;
    }
    if (strcmp(name, "string_pos") == 0 && count >= 2) {
        const char *sub = args[0].kind == GML_V_STRING && args[0].string ? args[0].string : "";
        const char *str = args[1].kind == GML_V_STRING && args[1].string ? args[1].string : "";
        const char *p = strstr(str, sub);
        *out = gml_value_real(p ? (double)(p - str + 1) : 0.0);
        return 1;
    }
    if (strcmp(name, "string_digits") == 0 && count >= 1) {
        const char *s = args[0].kind == GML_V_STRING && args[0].string ? args[0].string : "";
        size_t slen = strlen(s);
        char *buf = (char *)malloc(slen + 1);
        if (buf) {
            size_t o = 0;
            for (size_t i = 0; i < slen; i++) {
                if (isdigit((unsigned char)s[i])) buf[o++] = s[i];
            }
            buf[o] = 0;
            *out = gml_value_string(buf);
            free(buf);
        } else {
            *out = gml_value_string("");
        }
        return 1;
    }
    if (strcmp(name, "sqr") == 0 && count >= 1) {
        double v = args[0].real;
        *out = gml_value_real(v * v);
        return 1;
    }
    if (strcmp(name, "sqrt") == 0 && count >= 1) {
        *out = gml_value_real(sqrt(args[0].real));
        return 1;
    }
    if (strcmp(name, "log2") == 0 && count >= 1) {
        *out = gml_value_real(log2(args[0].real));
        return 1;
    }

    return 0;
}
