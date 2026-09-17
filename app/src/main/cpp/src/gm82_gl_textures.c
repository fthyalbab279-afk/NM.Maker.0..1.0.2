#define _POSIX_C_SOURCE 200809L
#include "gm82_gl_textures.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>

void gm82_gl_atlas_init(gm82_gl_atlas *a) {
    memset(a, 0, sizeof(*a));
}

void gm82_gl_atlas_free(gm82_gl_atlas *a) {
    if (!a) return;
    /* Would glDeleteTextures here when GLES is linked */
    free(a->sprites);
    free(a->backgrounds);
    memset(a, 0, sizeof(*a));
}

bool gm82_gl_upload_sprites(gm82_gl_atlas *a, const gm82_decoded_sprite_list *sprites) {
    if (!a || !sprites || sprites->count <= 0) return false;
    free(a->sprites);
    a->sprites = (gm82_gl_texture *)calloc((size_t)sprites->count, sizeof(gm82_gl_texture));
    if (!a->sprites) return false;
    a->sprite_count = sprites->count;
    for (int i = 0; i < sprites->count; i++) {
        a->sprites[i].width = sprites->frames[i].width;
        a->sprites[i].height = sprites->frames[i].height;
        a->sprites[i].tex_id = 0; /* no GLES – leave 0 */
        /* Real path:
           glGenTextures(1, &id);
           glBindTexture(GL_TEXTURE_2D, id);
           glTexImage2D(..., GL_RGBA, w, h, 0, GL_RGBA, GL_UNSIGNED_BYTE, rgba);
        */
    }
    a->ready = 0; /* not actually on GPU */
    return true;  /* structure filled; GPU upload pending */
}

bool gm82_gl_upload_backgrounds(gm82_gl_atlas *a, const gm82_decoded_background_list *bgs) {
    if (!a || !bgs || bgs->count <= 0) return false;
    free(a->backgrounds);
    a->backgrounds = (gm82_gl_texture *)calloc((size_t)bgs->count, sizeof(gm82_gl_texture));
    if (!a->backgrounds) return false;
    a->background_count = bgs->count;
    for (int i = 0; i < bgs->count; i++) {
        a->backgrounds[i].width = bgs->items[i].width;
        a->backgrounds[i].height = bgs->items[i].height;
        a->backgrounds[i].tex_id = 0;
    }
    return true;
}

bool gm82_gl_draw_texture(uint32_t tex_id, float x, float y, float w, float h) {
    (void)tex_id; (void)x; (void)y; (void)w; (void)h;
    /* STUB – needs GLES2 shader + VBO */
    return false;
}
