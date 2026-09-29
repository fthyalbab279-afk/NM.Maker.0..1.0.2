#include "gm82_loader.h"
#include "gm82_project_ir.h"
#include "gm82_runtime.h"
#include "gm82_sprite_decode.h"
#include "gm82_background_decode.h"
#include "gm82_object_room_decode.h"
#include "gm82_actions.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>

static const char *samples[4] = {
    "app/src/main/assets/www/samples/mario_bros.gmk",
    "app/src/main/assets/www/samples/plataformas.gmk",
    "app/src/main/assets/www/samples/shooter.gmk",
    "app/src/main/assets/www/samples/zelda.gmk"
};

int main(void) {
    printf("--- Running 4-Game Smoke Test Suite ---\n");

    for (int i = 0; i < 4; i++) {
        printf("Loading sample %d: %s ...\n", i + 1, samples[i]);
        char err[512] = {0};
        gm82_project_ir *ir = NULL;
        bool playable = gm82_load_and_prepare(samples[i], &ir, err, sizeof(err));
        assert(playable == true);
        assert(ir != NULL);

        FILE *f = fopen(samples[i], "rb");
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

        int n_spr = gm82_decode_sprites_from_gmk(gmk_data, (size_t)fsize, &sprs);
        int n_bg = gm82_decode_backgrounds_from_gmk(gmk_data, (size_t)fsize, &bgs);
        int n_obj = gm82_decode_objects_from_gmk(gmk_data, (size_t)fsize, &objs);
        int n_rm = gm82_decode_rooms_from_gmk(gmk_data, (size_t)fsize, &rms);
        gm82_actions_scan_gmk(gmk_data, (size_t)fsize, &actions);
        gm82_sprite_groups_build(&sprs, &sgroups);

        free(gmk_data);

        gm82_runtime rt;
        gm82_runtime_init(&rt);
        gm82_runtime_bind_assets(&rt, &objs, &sprs, &bgs, &rms, &actions);
        gm82_runtime_bind_sprite_groups(&rt, &sgroups);

        bool room_ok = gm82_runtime_goto_room(&rt, 0);
        assert(room_ok == true);

        /* Execute 10 frames */
        for (int frame = 0; frame < 10; frame++) {
            gm82_runtime_step(&rt);
        }

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
        printf("  [PASS] Sample %d: room_count=%d, sprites=%d, objs=%d, rooms=%d, nonzero_pixels=%d\n",
               i + 1, ir->room_count, n_spr, n_obj, n_rm, nonzero_pixels);

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
    }

    printf("SMOKE_4_GAME_TEST_PASS\n");
    return 0;
}
