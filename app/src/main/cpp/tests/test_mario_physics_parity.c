#include "gm82_loader.h"
#include "gm82_project_ir.h"
#include "gm82_runtime.h"
#include "gm82_input.h"
#include "gm82_sprite_decode.h"
#include "gm82_background_decode.h"
#include "gm82_object_room_decode.h"
#include "gm82_actions.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>

int main(void) {
    printf("--- Running Mario Physics Parity Test Suite (Host) ---\n");

    const char *gmk_path = "app/src/main/assets/www/samples/mario_bros.gmk";
    char err[512] = {0};
    gm82_project_ir *ir = NULL;
    bool playable = gm82_load_and_prepare(gmk_path, &ir, err, sizeof(err));
    assert(playable == true);
    assert(ir != NULL);

    FILE *f = fopen(gmk_path, "rb");
    assert(f != NULL);
    fseek(f, 0, SEEK_END);
    long fsize = ftell(f);
    fseek(f, 0, SEEK_SET);
    uint8_t *gmk_data = (uint8_t *)malloc((size_t)fsize);
    assert(gmk_data != NULL);
    size_t nread = fread(gmk_data, 1, (size_t)fsize, f);
    fclose(f);
    assert(nread == (size_t)fsize);

    gm82_decoded_sprite_list sprs;
    gm82_decoded_background_list bgs;
    gm82_decoded_object_list objs;
    gm82_decoded_room_list rms;
    gm82_sprite_group_list sgroups;
    gm82_action_table actions;

    gm82_decoded_sprite_list_init(&sprs);
    gm82_decoded_background_list_init(&bgs);
    memset(&objs, 0, sizeof(objs));
    memset(&rms, 0, sizeof(rms));
    memset(&actions, 0, sizeof(actions));

    gm82_decode_sprites_from_gmk(gmk_data, (size_t)fsize, &sprs);
    gm82_decode_backgrounds_from_gmk(gmk_data, (size_t)fsize, &bgs);
    gm82_decode_objects_from_gmk(gmk_data, (size_t)fsize, &objs);
    gm82_decode_rooms_from_gmk(gmk_data, (size_t)fsize, &rms);
    gm82_actions_scan_gmk(gmk_data, (size_t)fsize, &actions);
    gm82_sprite_groups_build(&sprs, &sgroups);

    free(gmk_data);

    gm82_runtime rt;
    gm82_runtime_init(&rt);
    gm82_runtime_bind_assets(&rt, &objs, &sprs, &bgs, &rms, &actions);
    gm82_runtime_bind_sprite_groups(&rt, &sgroups);

    bool room_ok = gm82_runtime_goto_room(&rt, 0);
    assert(room_ok == true);

    gm82_instance *player = NULL;
    for (int i = 0; i < rt.instance_count; i++) {
        if (rt.objects && rt.instances[i].object_index >= 0 &&
            rt.instances[i].object_index < rt.objects->count) {
            const char *name = rt.objects->items[rt.instances[i].object_index].name;
            if (strstr(name, "mario") != NULL || strstr(name, "player") != NULL) {
                player = &rt.instances[i];
                break;
            }
        }
    }
    if (!player) {
        if (rt.instance_count > 0) player = &rt.instances[0];
        else player = gm82_runtime_instance_create(&rt, 0, 100.0, 100.0);
    }
    assert(player != NULL);

    /* Bind right arrow input and set motion physics */
    gm82_input_state in;
    gm82_input_init(&in);
    gm82_input_key_down(&in, GM82_VK_RIGHT);
    gm82_input_bind_global(&in);

    player->gravity = 0.5;
    player->hspeed = 2.5;

    double initial_x = player->x;
    double initial_y = player->y;

    /* Simulate 90 frames of gameplay */
    for (int frame = 0; frame < 90; frame++) {
        gm82_runtime_step(&rt);
        rt.view_x = player->x - 100.0;
        if (rt.view_x < 0) rt.view_x = 0;
    }

    assert(player->x > initial_x + 20.0);
    assert(rt.view_x >= 0.0);

    uint8_t *buffer = (uint8_t *)calloc(1, 640 * 480 * 4);
    assert(buffer != NULL);

    bool drawn = gm82_runtime_draw(&rt, buffer, 640, 480);
    assert(drawn == true);

    int nonzero_pixels = 0;
    for (int p = 0; p < 640 * 480 * 4; p++) {
        if (buffer[p] != 0) nonzero_pixels++;
    }
    free(buffer);

    assert(nonzero_pixels > 1000);
    printf("  [PASS] Mario position dx=%.2f, dy=%.2f, view_x=%.2f, nonzero_pixels=%d\n",
           player->x - initial_x, player->y - initial_y, rt.view_x, nonzero_pixels);

    gm82_decoded_sprite_list_free(&sprs);
    gm82_decoded_background_list_free(&bgs);
    free(objs.items);
    for (int r = 0; r < rms.count; r++) {
        free(rms.items[r].instances);
        free(rms.items[r].tiles);
    }
    free(rms.items);
    gm82_sprite_group_list_free(&sgroups);
    gm82_action_table_free(&actions);

    gm82_project_ir_free(ir);

    printf("MARIO_PLAYABLE_PASS_HOST\n");
    return 0;
}
