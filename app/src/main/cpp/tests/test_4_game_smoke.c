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

typedef struct {
    char name[64];
    int sprites;
    int backgrounds;
    int objects;
    int instances;
    int tiles;
    int room_count;
    int step_draw_verified;
    size_t nonzero_pixels;
} game_smoke_result;

static game_smoke_result run_smoke_for_game(const char *game_name, const char *path) {
    game_smoke_result res;
    memset(&res, 0, sizeof(res));
    strncpy(res.name, game_name, sizeof(res.name)-1);

    FILE *f = fopen(path, "rb");
    if (!f) {
        printf("Failed to open %s\n", path);
        return res;
    }
    fseek(f, 0, SEEK_END);
    long len = ftell(f);
    fseek(f, 0, SEEK_SET);
    uint8_t *data = (uint8_t *)malloc((size_t)len);
    if (!data) { fclose(f); return res; }
    fread(data, 1, (size_t)len, f);
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

    res.sprites = gm82_decode_sprites_from_gmk(data, (size_t)len, &sprites);
    res.backgrounds = gm82_decode_backgrounds_from_gmk(data, (size_t)len, &bgs);
    res.objects = gm82_decode_objects_from_gmk(data, (size_t)len, &objs);
    res.room_count = gm82_decode_rooms_from_gmk(data, (size_t)len, &rooms);
    gm82_actions_scan_gmk(data, (size_t)len, &actions);
    gm82_sprite_groups_build(&sprites, &sprite_groups);

    int total_inst = 0;
    int total_tiles = 0;
    for (int r = 0; r < rooms.count; r++) {
        total_inst += rooms.items[r].instance_count;
        total_tiles += rooms.items[r].tile_count;
    }
    res.instances = total_inst;
    res.tiles = total_tiles;

    if (rooms.count > 0) {
        gm82_runtime rt;
        gm82_runtime_init(&rt);
        gm82_runtime_bind_assets(&rt, &objs, &sprites, &bgs, &rooms, &actions);
        gm82_runtime_bind_sprite_groups(&rt, &sprite_groups);

        bool room_ok = gm82_runtime_goto_room(&rt, 0);
        if (room_ok) {
            for (int f = 0; f < 10; f++) {
                gm82_runtime_step(&rt);
            }
            int buf_w = rt.room_width > 0 ? rt.room_width : 640;
            int buf_h = rt.room_height > 0 ? rt.room_height : 480;
            uint8_t *rgba = (uint8_t *)calloc(1, (size_t)buf_w * (size_t)buf_h * 4);
            if (rgba) {
                bool drawn = gm82_runtime_draw(&rt, rgba, buf_w, buf_h);
                if (drawn) {
                    size_t nonzero = 0;
                    for (size_t i = 0; i < (size_t)buf_w * (size_t)buf_h * 4; i += 4) {
                        if (rgba[i] != 0 || rgba[i+1] != 0 || rgba[i+2] != 0 || rgba[i+3] != 0) {
                            nonzero++;
                        }
                    }
                    res.nonzero_pixels = nonzero;
                    res.step_draw_verified = 1;
                }
                free(rgba);
            }
        }
    }

    gm82_decoded_sprite_list_free(&sprites);
    gm82_decoded_background_list_free(&bgs);
    gm82_decoded_object_list_free(&objs);
    gm82_decoded_room_list_free(&rooms);
    free(data);

    return res;
}

int main(void) {
    printf("--- Running 4 Game Smoke Suite ---\n");

    game_smoke_result mario = run_smoke_for_game("mario_bros", "app/src/main/assets/www/samples/mario_bros.gmk");
    game_smoke_result plat = run_smoke_for_game("plataformas", "app/src/main/assets/www/samples/plataformas.gmk");
    game_smoke_result shooter = run_smoke_for_game("shooter", "app/src/main/assets/www/samples/shooter.gmk");
    game_smoke_result zelda = run_smoke_for_game("zelda", "app/src/main/assets/www/samples/zelda.gmk");

    printf("RESULTS:\n");
    printf("mario_bros: spr=%d bg=%d obj=%d inst=%d tiles=%d rm=%d step_draw=%d nonzero=%zu\n",
           mario.sprites, mario.backgrounds, mario.objects, mario.instances, mario.tiles, mario.room_count, mario.step_draw_verified, mario.nonzero_pixels);
    printf("plataformas: spr=%d bg=%d obj=%d inst=%d tiles=%d rm=%d step_draw=%d nonzero=%zu\n",
           plat.sprites, plat.backgrounds, plat.objects, plat.instances, plat.tiles, plat.room_count, plat.step_draw_verified, plat.nonzero_pixels);
    printf("shooter: spr=%d bg=%d obj=%d inst=%d tiles=%d rm=%d step_draw=%d nonzero=%zu\n",
           shooter.sprites, shooter.backgrounds, shooter.objects, shooter.instances, shooter.tiles, shooter.room_count, shooter.step_draw_verified, shooter.nonzero_pixels);
    printf("zelda: spr=%d bg=%d obj=%d inst=%d tiles=%d rm=%d step_draw=%d nonzero=%zu\n",
           zelda.sprites, zelda.backgrounds, zelda.objects, zelda.instances, zelda.tiles, zelda.room_count, zelda.step_draw_verified, zelda.nonzero_pixels);

    /* Assertions TASK A */
    assert(mario.objects >= 10);
    assert(mario.instances >= 50);
    assert(mario.tiles >= 20);

    assert(plat.instances > 0 || (plat.room_count > 0));
    assert(shooter.instances > 0 || (shooter.room_count > 0));
    assert(zelda.instances > 0 || (zelda.room_count > 0));

    /* Assertions TASK B */
    assert(mario.step_draw_verified == 1);
    assert(plat.step_draw_verified == 1);
    assert(shooter.step_draw_verified == 1);
    assert(zelda.step_draw_verified == 1);
    assert(mario.nonzero_pixels > 1000);

    printf("--- TASK A & TASK B SMOKE PASS ---\n");
    return 0;
}
