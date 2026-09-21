#include "gm82_loader.h"
#include "gm82_project_ir.h"
#include "gm82_runtime.h"
#include "gm82_runtime_guard.h"
#include "gm82_sprite_decode.h"
#include "gm82_background_decode.h"
#include "gm82_object_room_decode.h"
#include "gm82_actions.h"
#include "gm82_input.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>

int main(void) {
    printf("--- Running Mario Physics Parity Test ---\n");
    const char *sample = "app/src/main/assets/www/samples/mario_bros.gmk";

    FILE *f = fopen(sample, "rb");
    assert(f != NULL);
    fseek(f, 0, SEEK_END);
    long len = ftell(f);
    fseek(f, 0, SEEK_SET);
    uint8_t *data = (uint8_t *)malloc(len);
    assert(data != NULL);
    fread(data, 1, len, f);
    fclose(f);

    gm82_decoded_sprite_list sprites;
    gm82_decoded_background_list bgs;
    gm82_decoded_object_list objs;
    gm82_decoded_room_list rooms;
    gm82_action_table actions;
    gm82_sprite_group_list sprite_groups;

    gm82_decoded_sprite_list_init(&sprites);
    gm82_decoded_background_list_init(&bgs);
    memset(&objs, 0, sizeof(objs));
    memset(&rooms, 0, sizeof(rooms));
    memset(&actions, 0, sizeof(actions));

    int n_spr = gm82_decode_sprites_from_gmk(data, len, &sprites);
    int n_bg = gm82_decode_backgrounds_from_gmk(data, len, &bgs);
    int n_obj = gm82_decode_objects_from_gmk(data, len, &objs);
    int n_rm = gm82_decode_rooms_from_gmk(data, len, &rooms);
    gm82_actions_scan_gmk(data, len, &actions);
    gm82_sprite_groups_build(&sprites, &sprite_groups);

    printf("Decoded: spr=%d, bg=%d, obj=%d, rm=%d\n", n_spr, n_bg, n_obj, n_rm);
    assert(n_obj >= 15);
    assert(n_rm > 0);

    gm82_runtime rt;
    gm82_runtime_init(&rt);
    gm82_runtime_bind_assets(&rt, &objs, &sprites, &bgs, &rooms, &actions);
    gm82_runtime_bind_sprite_groups(&rt, &sprite_groups);

    gm82_input_state input_state;
    gm82_input_init(&input_state);
    gm82_input_bind_global(&input_state);

    /* Press right arrow key (39) */
    gm82_input_key_down(&input_state, 39);

    bool room_ok = gm82_runtime_goto_room(&rt, 0);
    assert(room_ok);
    assert(rt.instance_count > 0);

    /* Find Mario/Player instance by object name (obj_minimario or obj_mario) */
    gm82_instance *player = NULL;
    for (int i = 0; i < rt.instance_count; i++) {
        gm82_instance *inst = &rt.instances[i];
        if (!inst->alive) continue;
        if (inst->object_index >= 0 && inst->object_index < objs.count) {
            const char *oname = objs.items[inst->object_index].name;
            if (strstr(oname, "mario") || strstr(oname, "player")) {
                player = inst;
                break;
            }
        }
    }
    assert(player != NULL);

    double start_x = player->x;
    double start_y = player->y;
    printf("Initial Player position: x=%.2f, y=%.2f\n", start_x, start_y);

    /* Simulate 100 frames with right arrow key held */
    for (int frame = 0; frame < 100; frame++) {
        gm82_runtime_step(&rt);
    }

    assert(rt.frame == 100);
    double end_x = player->x;
    double end_y = player->y;
    double dx = end_x - start_x;
    printf("Final Player position: x=%.2f, y=%.2f, dx=%.2f\n", end_x, end_y, dx);

    /* Phase 3.6 Assertions: dx > 20 and y within room bounds */
    assert(dx > 20.0);
    assert(end_y >= 0.0 && end_y <= (rt.room_height > 0 ? rt.room_height : 480));

    /* Render soft frame */
    int buf_w = rt.room_width > 0 ? rt.room_width : 640;
    int buf_h = rt.room_height > 0 ? rt.room_height : 480;
    uint8_t *rgba = (uint8_t *)calloc(1, (size_t)buf_w * (size_t)buf_h * 4);
    assert(rgba != NULL);
    bool drawn = gm82_runtime_draw(&rt, rgba, buf_w, buf_h);
    assert(drawn);

    size_t nonzero_pixels = 0;
    for (size_t i = 0; i < (size_t)buf_w * (size_t)buf_h * 4; i += 4) {
        if (rgba[i] != 0 || rgba[i+1] != 0 || rgba[i+2] != 0 || rgba[i+3] != 0) {
            nonzero_pixels++;
        }
    }
    free(rgba);
    free(data);

    printf("Nonzero rendered pixels: %zu\n", nonzero_pixels);
    assert(nonzero_pixels > 1000);

    printf("[PASS] MARIO_PLAYABLE_PASS_HOST\n");
    return 0;
}
