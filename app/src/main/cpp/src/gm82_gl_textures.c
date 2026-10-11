#define _POSIX_C_SOURCE 200809L
#include "gm82_gl_textures.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>

#if defined(__ANDROID__) || defined(GM82_HAVE_GLES)
#include <GLES2/gl2.h>
#endif

void gm82_gl_atlas_init(gm82_gl_atlas *a) {
    memset(a, 0, sizeof(*a));
}

void gm82_gl_atlas_free(gm82_gl_atlas *a) {
    if (!a) return;
#if defined(__ANDROID__) || defined(GM82_HAVE_GLES)
    if (a->sprites) {
        for (int i = 0; i < a->sprite_count; i++) {
            if (a->sprites[i].tex_id != 0) {
                GLuint id = (GLuint)a->sprites[i].tex_id;
                glDeleteTextures(1, &id);
            }
        }
    }
    if (a->backgrounds) {
        for (int i = 0; i < a->background_count; i++) {
            if (a->backgrounds[i].tex_id != 0) {
                GLuint id = (GLuint)a->backgrounds[i].tex_id;
                glDeleteTextures(1, &id);
            }
        }
    }
#endif
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
#if defined(__ANDROID__) || defined(GM82_HAVE_GLES)
        if (sprites->frames[i].rgba && sprites->frames[i].width > 0 && sprites->frames[i].height > 0) {
            GLuint id = 0;
            glGenTextures(1, &id);
            glBindTexture(GL_TEXTURE_2D, id);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
            glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA,
                         sprites->frames[i].width, sprites->frames[i].height,
                         0, GL_RGBA, GL_UNSIGNED_BYTE, sprites->frames[i].rgba);
            a->sprites[i].tex_id = (uint32_t)id;
        } else {
            a->sprites[i].tex_id = (uint32_t)(i + 1000);
        }
#else
        a->sprites[i].tex_id = (uint32_t)(i + 1000);
#endif
    }
    a->ready = 1;
    return true;
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
#if defined(__ANDROID__) || defined(GM82_HAVE_GLES)
        if (bgs->items[i].rgba && bgs->items[i].width > 0 && bgs->items[i].height > 0) {
            GLuint id = 0;
            glGenTextures(1, &id);
            glBindTexture(GL_TEXTURE_2D, id);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
            glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA,
                         bgs->items[i].width, bgs->items[i].height,
                         0, GL_RGBA, GL_UNSIGNED_BYTE, bgs->items[i].rgba);
            a->backgrounds[i].tex_id = (uint32_t)id;
        } else {
            a->backgrounds[i].tex_id = (uint32_t)(i + 2000);
        }
#else
        a->backgrounds[i].tex_id = (uint32_t)(i + 2000);
#endif
    }
    return true;
}

bool gm82_gl_draw_texture(uint32_t tex_id, float x, float y, float w, float h) {
    if (tex_id == 0) return false;

    /* Quad vertex attributes layout: {x, y, u, v} */
    float verts[16] = {
        x,     y,     0.0f, 0.0f,
        x + w, y,     1.0f, 0.0f,
        x,     y + h, 0.0f, 1.0f,
        x + w, y + h, 1.0f, 1.0f
    };
    (void)verts;

#if defined(__ANDROID__) || defined(GM82_HAVE_GLES)
    glBindTexture(GL_TEXTURE_2D, (GLuint)tex_id);
    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float), verts);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float), verts + 2);
    glEnableVertexAttribArray(1);
    glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
    return true;
#else
    return true;
#endif
}
