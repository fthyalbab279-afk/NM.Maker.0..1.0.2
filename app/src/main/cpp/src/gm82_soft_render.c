#define _POSIX_C_SOURCE 200809L
/*
 * Minimal CPU framebuffer – NOT a full game loop.
 * Proof that decoded backgrounds + sprites can be composed.
 */
#include "gm82_project_ir.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <stdint.h>
#include <stdbool.h>

typedef struct {
    int32_t width, height;
    uint8_t *rgba; /* width*height*4 */
} gm82_framebuffer;

bool gm82_framebuffer_create(gm82_framebuffer *fb, int32_t w, int32_t h) {
    memset(fb, 0, sizeof(*fb));
    if (w <= 0 || h <= 0 || w > 8192 || h > 8192) return false;
    fb->width = w; fb->height = h;
    fb->rgba = (uint8_t *)calloc((size_t)w * (size_t)h * 4, 1);
    return fb->rgba != NULL;
}

void gm82_framebuffer_free(gm82_framebuffer *fb) {
    if (!fb) return;
    free(fb->rgba);
    memset(fb, 0, sizeof(*fb));
}

static void blit_rgba(gm82_framebuffer *fb, const uint8_t *src,
                      int32_t sw, int32_t sh, int32_t dx, int32_t dy, bool use_alpha) {
    if (!fb || !src) return;
    for (int32_t y = 0; y < sh; y++) {
        int32_t dy2 = dy + y;
        if (dy2 < 0 || dy2 >= fb->height) continue;
        for (int32_t x = 0; x < sw; x++) {
            int32_t dx2 = dx + x;
            if (dx2 < 0 || dx2 >= fb->width) continue;
            const uint8_t *s = src + ((size_t)y * (size_t)sw + (size_t)x) * 4;
            uint8_t *d = fb->rgba + ((size_t)dy2 * (size_t)fb->width + (size_t)dx2) * 4;
            if (use_alpha && s[3] == 0) continue;
            d[0]=s[0]; d[1]=s[1]; d[2]=s[2]; d[3]=255;
        }
    }
}

/* Compose first decoded background (if any) + first few sprites into a room-sized buffer */
bool gm82_soft_render_preview(const gm82_project_ir *ir, gm82_framebuffer *fb) {
    if (!ir || !fb) return false;
    int32_t rw = 320, rh = 240;
    if (ir->room_count > 0 && ir->rooms[0].width > 0) {
        rw = ir->rooms[0].width;
        rh = ir->rooms[0].height;
    }
    if (!gm82_framebuffer_create(fb, rw, rh)) return false;

    /* clear dark */
    for (size_t i = 0; i < (size_t)rw * (size_t)rh; i++) {
        fb->rgba[i*4+0]=20; fb->rgba[i*4+1]=20; fb->rgba[i*4+2]=40; fb->rgba[i*4+3]=255;
    }

    if (ir->background_count > 0 && ir->backgrounds[0].rgba &&
        ir->backgrounds[0].status == GM82_RES_DECODED) {
        blit_rgba(fb, ir->backgrounds[0].rgba,
                  ir->backgrounds[0].width, ir->backgrounds[0].height, 0, 0, false);
    }

    /* place up to 8 sprites in a row as proof */
    int32_t x = 8, y = 8;
    int placed = 0;
    for (int32_t i = 0; i < ir->sprite_count && placed < 8; i++) {
        const gm82_res_sprite *s = &ir->sprites[i];
        if (s->status != GM82_RES_DECODED || !s->rgba) continue;
        blit_rgba(fb, s->rgba, s->width, s->height, x, y, true);
        x += s->width + 4;
        if (x + 32 > rw) { x = 8; y += 64; }
        placed++;
    }
    return placed > 0 || ir->background_count > 0;
}

bool gm82_framebuffer_write_ppm(const gm82_framebuffer *fb, const char *path) {
    if (!fb || !fb->rgba || !path) return false;
    FILE *f = fopen(path, "wb");
    if (!f) return false;
    fprintf(f, "P6\n%d %d\n255\n", fb->width, fb->height);
    for (int32_t i = 0; i < fb->width * fb->height; i++) {
        fputc(fb->rgba[i*4+0], f);
        fputc(fb->rgba[i*4+1], f);
        fputc(fb->rgba[i*4+2], f);
    }
    fclose(f);
    return true;
}
