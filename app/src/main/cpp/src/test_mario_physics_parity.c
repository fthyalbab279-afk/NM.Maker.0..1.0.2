#include "gm82_runtime.h"
#include "gm82_gmk_reader.h"
#include "gm82_sprite_decode.h"
#include "gm82_background_decode.h"
#include "gm82_object_room_decode.h"
#include "gm82_actions.h"
#include "gm82_events.h"
#include "gm82_input.h"
#include "gm82_gml_builtins.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

int main(int argc, char **argv) {
    const char *path = (argc > 1) ? argv[1] : "NOR_MAKER_COMPLETE_PACKAGE/02_SAMPLES/mario_bros.gmk";
    printf("=== Testing Mario Walk/Jump/Ground Physics Parity on %s ===\n", path);

    FILE *f = fopen(path, "rb");
    if (!f) { printf("FAIL: cannot open %s\n", path); return 1; }
    fseek(f, 0, SEEK_END);
    long sz = ftell(f);
    fseek(f, 0, SEEK_SET);
    uint8_t *buf = (uint8_t *)malloc((size_t)sz);
    fread(buf, 1, (size_t)sz, f);
    fclose(f);

    gm82_decoded_sprite_list sprites;
    gm82_decoded_background_list bgs;
    gm82_decoded_object_list objs;
    gm82_decoded_room_list rooms;
    gm82_action_table actions;

    gm82_decode_sprites_from_gmk(buf, (size_t)sz, &sprites);
    gm82_decode_backgrounds_from_gmk(buf, (size_t)sz, &bgs);
    gm82_decode_objects_from_gmk(buf, (size_t)sz, &objs);
    gm82_decode_rooms_from_gmk(buf, (size_t)sz, &rooms);
    gm82_actions_scan_gmk(buf, (size_t)sz, &actions);
    free(buf);

    gm82_runtime rt;
    gm82_runtime_init(&rt);
    gm82_runtime_bind_assets(&rt, &objs, &sprites, &bgs, &rooms, &actions);

    gm82_input_state input;
    gm82_input_init(&input);
    gm82_input_bind_global(&input);

    /* Go to room 0 */
    gm82_runtime_goto_room(&rt, 0);

    /* Find player instance (mario/minimario) */
    gm82_instance *player = NULL;
    for (int i = 0; i < rt.instance_count; i++) {
        if (!rt.instances[i].alive) continue;
        int oi = rt.instances[i].object_index;
        if (oi >= 0 && oi < objs.count) {
            const char *oname = objs.items[oi].name;
            if (strstr(oname, "mario") || strstr(oname, "player")) {
                player = &rt.instances[i];
                break;
            }
        }
    }

    if (!player && rt.instance_count > 0) player = &rt.instances[0];

    if (!player) {
        printf("FAIL: No player instance found in room 0\n");
        return 1;
    }

    printf("Frame 0: x=%.2f y=%.2f vspeed=%.2f\n", player->x, player->y, player->vspeed);
    double initial_x = player->x;
    double initial_y = player->y;

    /* Step 20 frames with LEFT arrow key down */
    gm82_input_key_down(&input, 37); /* vk_left */
    for (int f = 1; f <= 20; f++) {
        gm82_input_begin_frame(&input);
        gm82_runtime_step(&rt);
    }
    gm82_input_key_up(&input, 37);

    printf("Frame 20 after LEFT: x=%.2f y=%.2f hspeed=%.2f vspeed=%.2f\n",
           player->x, player->y, player->hspeed, player->vspeed);
    double x_after_left = player->x;

    /* Step 10 frames with JUMP key down */
    gm82_input_key_down(&input, 38); /* vk_up */
    for (int f = 21; f <= 30; f++) {
        gm82_input_begin_frame(&input);
        gm82_runtime_step(&rt);
    }
    gm82_input_key_up(&input, 38);

    printf("Frame 30 after JUMP: x=%.2f y=%.2f vspeed=%.2f\n", player->x, player->y, player->vspeed);
    double y_after_jump = player->y;

    /* Step 30 more frames to let gravity land on solid ground */
    for (int f = 31; f <= 60; f++) {
        gm82_input_begin_frame(&input);
        gm82_runtime_step(&rt);
    }

    printf("Frame 60 on GROUND: x=%.2f y=%.2f vspeed=%.2f\n", player->x, player->y, player->vspeed);

    int pass_left = (x_after_left < initial_x);
    int pass_jump = (y_after_jump < initial_y || player->vspeed != 0);
    int pass_ground = (player->vspeed <= 1.0);

    if (pass_left && pass_jump && pass_ground) {
        printf("MARIO_PHYSICS_PARITY_TEST_PASS: Walk (x decreased), Jump (vspeed changed), Ground landing (vspeed <= 1.0) verified on mario_bros.gmk\n");
        return 0;
    } else {
        printf("MARIO_PHYSICS_PARITY_TEST_FAIL: pass_left=%d pass_jump=%d pass_ground=%d\n", pass_left, pass_jump, pass_ground);
        return 1;
    }
}
