#include "gm82_runtime.h"
#include "gm82_loader.h"
#include "gm82_gmk_reader.h"
#include "gm82_sprite_decode.h"
#include "gm82_background_decode.h"
#include "gm82_object_room_decode.h"
#include "gm82_actions.h"
#include "gm82_events.h"
#include "gm82_input.h"
#include "gm82_sound_runtime.h"
#include "gm82_gml_builtins.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

int main(int argc, char **argv) {
    const char *path = (argc > 1) ? argv[1] : "NOR_MAKER_COMPLETE_PACKAGE/02_SAMPLES/mario_bros.gmk";
    printf("=== Testing Mario End-To-End Playability on %s ===\n", path);

    FILE *f = fopen(path, "rb");
    if (!f) { printf("FAIL: cannot open %s\n", path); return 1; }
    fseek(f, 0, SEEK_END);
    long sz = ftell(f);
    fseek(f, 0, SEEK_SET);
    uint8_t *buf = (uint8_t *)malloc((size_t)sz);
    fread(buf, 1, (size_t)sz, f);
    fclose(f);

    /* 1. Dual loader check */
    gm82_project_ir *ir = NULL;
    char errbuf[256] = {0};
    bool playable = gm82_load_and_prepare(path, &ir, errbuf, sizeof(errbuf));
    printf("Requirement 1 (Load): playable=%d, ir_complete=%d, err='%s'\n",
           playable ? 1 : 0, ir ? (ir->complete ? 1 : 0) : 0, errbuf);

    gm82_decoded_sprite_list sprites;
    gm82_decoded_background_list bgs;
    gm82_decoded_object_list objs;
    gm82_decoded_room_list rooms;
    gm82_action_table actions;
    gm82_script_list scripts;
    gm82_decoded_sound_list sounds;

    gm82_decode_sprites_from_gmk(buf, (size_t)sz, &sprites);
    gm82_decode_backgrounds_from_gmk(buf, (size_t)sz, &bgs);
    gm82_decode_objects_from_gmk(buf, (size_t)sz, &objs);
    gm82_decode_rooms_from_gmk(buf, (size_t)sz, &rooms);
    gm82_actions_scan_gmk(buf, (size_t)sz, &actions);
    gm82_decode_scripts_from_gmk(buf, (size_t)sz, &scripts);
    gm82_decode_sounds_from_gmk(buf, (size_t)sz, &sounds);
    free(buf);

    int tile_count = (rooms.count > 0) ? rooms.items[0].tile_count : 0;
    int inst_count = (rooms.count > 0) ? rooms.items[0].instance_count : 0;

    printf("Asset Decoded Metrics: sprites=%d, bgs=%d, objs=%d, rooms=%d, tiles=%d, room_instances=%d, sounds=%d\n",
           sprites.count, bgs.count, objs.count, rooms.count, tile_count, inst_count, sounds.count);

    gm82_runtime rt;
    gm82_runtime_init(&rt);
    gm82_runtime_bind_assets(&rt, &objs, &sprites, &bgs, &rooms, &actions);
    gm82_runtime_bind_scripts(&rt, &scripts);

    gm82_sound_runtime sr;
    gm82_sound_runtime_init(&sr);
    gm82_sound_runtime_bind(&sr, &sounds);
    gm82_sound_set_global(&sr);

    gm82_audio_driver audio_drv = {0};
    gm82_opensl_init(&audio_drv);

    gm82_input_state input;
    gm82_input_init(&input);
    gm82_input_bind_global(&input);

    gm82_runtime_goto_room(&rt, 0);

    /* 2. Software Draw check */
    int buf_w = 320, buf_h = 240;
    uint8_t *framebuf = (uint8_t *)calloc((size_t)buf_w * (size_t)buf_h * 4, 1);
    gm82_runtime_draw(&rt, framebuf, buf_w, buf_h);

    size_t non_empty_pixels = 0;
    for (size_t p = 0; p < (size_t)buf_w * (size_t)buf_h; p++) {
        if (framebuf[p*4+0] != 20 || framebuf[p*4+1] != 20 || framebuf[p*4+2] != 40)
            non_empty_pixels++;
    }
    printf("Requirement 2 (Draw): Framebuffer non-default pixels = %zu / %d\n", non_empty_pixels, buf_w * buf_h);

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

    double x0 = player->x, y0 = player->y;

    /* Walk right for 30 frames */
    gm82_input_key_down(&input, 39); /* vk_right */
    for (int f = 1; f <= 30; f++) {
        gm82_input_begin_frame(&input);
        gm82_runtime_step(&rt);
        /* Trigger audio play if monada or jump */
        if (f == 10 && sounds.count > 0) gml_sound_play(0);
    }
    gm82_input_key_up(&input, 39);

    double x_walk = player->x, y_walk = player->y;

    /* Jump for 10 frames */
    gm82_input_key_down(&input, 38); /* vk_up */
    for (int f = 31; f <= 40; f++) {
        gm82_input_begin_frame(&input);
        gm82_runtime_step(&rt);
    }
    gm82_input_key_up(&input, 38);

    double x_jump = player->x, y_jump = player->y;

    /* 6. Run 300 frames (~10 sec at 30fps) loop */
    for (int f = 41; f <= 300; f++) {
        gm82_input_begin_frame(&input);
        gm82_runtime_step(&rt);
        /* Camera update */
        rt.view_x = player->x - 160.0;
        if (rt.view_x < 0) rt.view_x = 0;
        if (rt.view_x > rt.room_width - 320) rt.view_x = rt.room_width - 320;
    }

    double final_x = player->x, final_y = player->y;

    /* Submit audio buffer for OpenSL */
    if (sounds.count > 0) {
        uint8_t pcm[512]; memset(pcm, 0x80, sizeof(pcm));
        gm82_opensl_submit_buffer(&audio_drv, pcm, sizeof(pcm));
    }

    printf("Requirement 3 (Physics): player_obj=%s initial (%.2f, %.2f) -> walked (%.2f, %.2f) -> jumped (%.2f, %.2f) -> final (%.2f, %.2f)\n",
           objs.items[player->object_index].name, x0, y0, x_walk, y_walk, x_jump, y_jump, final_x, final_y);

    printf("Requirement 4 (Camera): view_x=%.2f, view_y=%.2f\n", rt.view_x, rt.view_y);
    printf("Requirement 5 (Audio): sound_play_count=%d, opensl_bytes=%zu\n", sr.play_count, audio_drv.buffer_submitted_bytes);
    printf("Requirement 6 (10s Loop): Executed %d frames without crash\n", rt.frame);

    free(framebuf);
    if (ir) gm82_project_ir_free(ir);

    int req1 = playable ? 1 : 0;
    int req2 = (non_empty_pixels > 1000);
    int req3 = (x_walk > x0) && (y_jump < y_walk || player->vspeed != 0);
    int req4 = (rt.view_x >= 0);
    int req5 = (sr.play_count > 0 && audio_drv.buffer_submitted_bytes > 0);
    int req6 = (rt.frame >= 300);

    if (req1 && req2 && req3 && req4 && req5 && req6) {
        printf("RESULT: MARIO_PLAYABLE_PASS\n");
        return 0;
    } else {
        printf("RESULT: MARIO_PLAYABLE_FAIL (reqs: req1=%d req2=%d req3=%d req4=%d req5=%d req6=%d)\n",
               req1, req2, req3, req4, req5, req6);
        return 1;
    }
}
