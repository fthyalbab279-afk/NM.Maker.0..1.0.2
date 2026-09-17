#include "gm82_project_ir.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>

gm82_project_ir *gm82_project_ir_create(void) {
    gm82_project_ir *ir = (gm82_project_ir *)calloc(1, sizeof(gm82_project_ir));
    if (!ir) return NULL;
    ir->ir_version = GM82_IR_VERSION;
    ir->complete = false;
    return ir;
}

static void free_sprite(gm82_res_sprite *s) {
    if (!s) return;
    free(s->name);
    free(s->gl_textures);
    free(s->rgba);
    memset(s, 0, sizeof(*s));
}

static void free_background(gm82_res_background *b) {
    if (!b) return;
    free(b->name);
    free(b->rgba);
    memset(b, 0, sizeof(*b));
}

static void free_room(gm82_res_room *r) {
    if (!r) return;
    free(r->name);
    free(r->caption);
    free(r->background_ids);
    free(r->background_visible);
    free(r->background_foreground);
    free(r->background_x);
    free(r->background_y);
    free(r->views);
    if (r->instances) {
        for (int32_t i = 0; i < r->instance_count; i++)
            free(r->instances[i].creation_code);
        free(r->instances);
    }
    free(r->tiles);
    memset(r, 0, sizeof(*r));
}

void gm82_project_ir_free(gm82_project_ir *ir) {
    if (!ir) return;
    for (int32_t i = 0; i < ir->sprite_count; i++) free_sprite(&ir->sprites[i]);
    free(ir->sprites);
    for (int32_t i = 0; i < ir->background_count; i++) free_background(&ir->backgrounds[i]);
    free(ir->backgrounds);
    for (int32_t i = 0; i < ir->room_count; i++) free_room(&ir->rooms[i]);
    free(ir->rooms);
    free(ir->source_path);
    free(ir->source_data);
    free(ir->error_message);
    free(ir);
}

void gm82_project_ir_recompute_complete(gm82_project_ir *ir) {
    if (!ir) return;
    int32_t partial = 0;
    for (int32_t i = 0; i < ir->sprite_count; i++)
        if (ir->sprites[i].status != GM82_RES_DECODED) partial++;
    for (int32_t i = 0; i < ir->background_count; i++)
        if (ir->backgrounds[i].status != GM82_RES_DECODED) partial++;
    for (int32_t i = 0; i < ir->room_count; i++)
        if (ir->rooms[i].status != GM82_RES_DECODED) partial++;
    ir->partial_count = partial;
    bool has_pixels = false;
    for (int32_t i = 0; i < ir->sprite_count; i++)
        if (ir->sprites[i].status == GM82_RES_DECODED && ir->sprites[i].rgba) has_pixels = true;
    for (int32_t i = 0; i < ir->background_count; i++)
        if (ir->backgrounds[i].status == GM82_RES_DECODED && ir->backgrounds[i].rgba) has_pixels = true;
    /* complete when no partial resources, has a room, and at least one pixel resource */
    ir->complete = (partial == 0) && (ir->room_count > 0) && has_pixels;
}

bool gm82_project_ir_is_playable(const gm82_project_ir *ir) {
    if (!ir) return false;
    return ir->complete && ir->room_count > 0;
}
