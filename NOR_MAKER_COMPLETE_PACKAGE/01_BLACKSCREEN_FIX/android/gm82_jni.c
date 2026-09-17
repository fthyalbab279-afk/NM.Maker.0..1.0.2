#define _POSIX_C_SOURCE 200809L
/*
 * JNI-facing native layer – connects existing core.
 * GL draw is intentionally STUB (returns without drawing) until EGL textures exist.
 */
#include "gm82_jni.h"
#include "gm82_runtime.h"
#include "gm82_events.h"
#include "gm82_input.h"
#include "gm82_view.h"
#include "gm82_object_room_decode.h"
#include "gm82_sprite_decode.h"
#include "gm82_background_decode.h"
#include "gm82_actions.h"
#include "gm82_gml_builtins.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>

static struct {
    int inited;
    int running;
    int surf_w, surf_h;
    gm82_runtime rt;
    gm82_input_state input;
    gm82_view_state views;
    gm82_decoded_object_list objs;
    gm82_decoded_room_list rooms;
    gm82_decoded_sprite_list sprites;
    gm82_decoded_background_list bgs;
    gm82_action_table actions;
    uint8_t *file_buf;
    size_t file_size;
} g;

bool gm82_native_init(int surface_width, int surface_height) {
    memset(&g, 0, sizeof(g));
    g.surf_w = surface_width;
    g.surf_h = surface_height;
    gm82_runtime_init(&g.rt);
    gm82_input_init(&g.input);
    gm82_input_bind_global(&g.input);
    gm82_events_register_defaults();
    g.inited = 1;
    return true;
}

void gm82_native_shutdown(void) {
    if (!g.inited) return;
    gm82_decoded_object_list_free(&g.objs);
    gm82_decoded_room_list_free(&g.rooms);
    gm82_decoded_sprite_list_free(&g.sprites);
    gm82_decoded_background_list_free(&g.bgs);
    gm82_action_table_free(&g.actions);
    free(g.file_buf);
    memset(&g, 0, sizeof(g));
}

bool gm82_native_load_game(const char *path, char *err, int err_len) {
    if (!g.inited) {
        if (err && err_len > 0) snprintf(err, (size_t)err_len, "not inited");
        return false;
    }
    FILE *f = fopen(path, "rb");
    if (!f) {
        if (err && err_len > 0) snprintf(err, (size_t)err_len, "open failed: %s", path);
        return false;
    }
    fseek(f, 0, SEEK_END);
    long sz = ftell(f);
    fseek(f, 0, SEEK_SET);
    if (sz < 12) { fclose(f); if (err) snprintf(err, (size_t)err_len, "file too small"); return false; }
    free(g.file_buf);
    g.file_buf = (uint8_t *)malloc((size_t)sz);
    if (!g.file_buf) { fclose(f); return false; }
    if (fread(g.file_buf, 1, (size_t)sz, f) != (size_t)sz) {
        fclose(f); free(g.file_buf); g.file_buf = NULL;
        if (err) snprintf(err, (size_t)err_len, "read failed");
        return false;
    }
    fclose(f);
    g.file_size = (size_t)sz;

    gm82_decoded_object_list_free(&g.objs);
    gm82_decoded_room_list_free(&g.rooms);
    gm82_decoded_sprite_list_free(&g.sprites);
    gm82_decoded_background_list_free(&g.bgs);
    gm82_action_table_free(&g.actions);
    memset(&g.objs, 0, sizeof(g.objs));
    memset(&g.rooms, 0, sizeof(g.rooms));
    memset(&g.sprites, 0, sizeof(g.sprites));
    memset(&g.bgs, 0, sizeof(g.bgs));
    memset(&g.actions, 0, sizeof(g.actions));

    int ns = gm82_decode_sprites_from_gmk(g.file_buf, g.file_size, &g.sprites);
    int nb = gm82_decode_backgrounds_from_gmk(g.file_buf, g.file_size, &g.bgs);
    int no = gm82_decode_objects_from_gmk(g.file_buf, g.file_size, &g.objs);
    int nr = gm82_decode_rooms_from_gmk(g.file_buf, g.file_size, &g.rooms);
    gm82_actions_scan_gmk(g.file_buf, g.file_size, &g.actions);

    if (ns <= 0 && nb <= 0) {
        if (err) snprintf(err, (size_t)err_len, "no sprites/backgrounds decoded");
        return false;
    }
    if (nr <= 0) {
        if (err) snprintf(err, (size_t)err_len, "no rooms decoded");
        return false;
    }

    gm82_runtime_init(&g.rt);
    gm82_runtime_bind_assets(&g.rt, &g.objs, &g.sprites, &g.bgs, &g.rooms, &g.actions);
    if (!gm82_runtime_goto_room(&g.rt, 0)) {
        if (err) snprintf(err, (size_t)err_len, "goto_room(0) failed");
        return false;
    }
    gm82_view_init(&g.views, g.rt.room_width, g.rt.room_height);
    if (g.objs.count > 0)
        g.views.views[0].follow_object = 0;
    g.running = 1;

    if (err) snprintf(err, (size_t)err_len, "ok spr=%d bg=%d obj=%d room=%d act_obj=%d",
                      ns, nb, no, nr, g.actions.count);
    return true;
}

void gm82_native_resize(int w, int h) {
    g.surf_w = w; g.surf_h = h;
}

void gm82_native_step(void) {
    if (!g.running) return;
    gm82_input_begin_frame(&g.input);
    /* note: keys must be re-asserted by Java each frame or we track held state in input.down */
    gm82_runtime_step(&g.rt);
    gm82_view_update(&g.views, &g.rt);
}

void gm82_native_draw(void) {
    /* STUB: no GL context binding here.
       Soft framebuffer path exists in gm82_runtime_draw for offline tests.
       Android must upload textures – not implemented in this scaffold. */
}

void gm82_native_key_down(int vk) { gm82_input_key_down(&g.input, vk); }
void gm82_native_key_up(int vk) { gm82_input_key_up(&g.input, vk); }

void gm82_native_touch(int x, int y, int action) {
    gm82_input_set_mouse(&g.input, x, y);
    if (action == 0) g.input.mouse_button[0] = 1;
    else if (action == 1) g.input.mouse_button[0] = 0;
}

int gm82_native_is_running(void) { return g.running; }
int gm82_native_room_width(void) { return g.rt.room_width; }
int gm82_native_room_height(void) { return g.rt.room_height; }
