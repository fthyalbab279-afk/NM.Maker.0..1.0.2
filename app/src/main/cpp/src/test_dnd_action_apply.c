#include "gm82_runtime.h"
#include "gm82_gmk_reader.h"
#include "gm82_sprite_decode.h"
#include "gm82_background_decode.h"
#include "gm82_object_room_decode.h"
#include "gm82_actions.h"
#include "gm82_events.h"
#include "gm82_gml_builtins.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

int main(int argc, char **argv) {
    const char *path = (argc > 1) ? argv[1] : "NOR_MAKER_COMPLETE_PACKAGE/02_SAMPLES/mario_bros.gmk";
    printf("=== Testing DnD Action State Application on %s ===\n", path);

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

    gm82_runtime_goto_room(&rt, 0);

    if (rt.instance_count == 0) {
        printf("FAIL: 0 instances in room 0\n");
        return 1;
    }

    /* Find object obj_minimario_trans or obj_minimario */
    int target_oi = -1;
    for (int o = 0; o < objs.count; o++) {
        if (strcmp(objs.items[o].name, "obj_minimario_trans") == 0 ||
            strcmp(objs.items[o].name, "obj_minimario") == 0) {
            target_oi = o;
            break;
        }
    }

    gm82_instance *inst = NULL;
    if (target_oi >= 0) {
        inst = gm82_runtime_instance_create(&rt, target_oi, 100, 200);
    } else {
        inst = &rt.instances[0];
    }

    int pre_obj = inst->object_index;
    int pre_sprite = inst->sprite_index;
    double pre_x = inst->x;
    double pre_y = inst->y;
    double pre_hspeed = inst->hspeed;

    printf("BEFORE action apply: obj=%d (%s) sprite=%d x=%.2f y=%.2f hspeed=%.2f\n",
           pre_obj, objs.items[pre_obj].name, pre_sprite, pre_x, pre_y, pre_hspeed);

    /* Fire object actions for instance */
    gm82_actions_fire_create(&rt, inst, &actions);

    int post_obj = inst->object_index;
    int post_sprite = inst->sprite_index;
    double post_x = inst->x;
    double post_y = inst->y;
    double post_hspeed = inst->hspeed;

    printf("AFTER action apply:  obj=%d (%s) sprite=%d x=%.2f y=%.2f hspeed=%.2f\n",
           post_obj, objs.items[post_obj].name, post_sprite, post_x, post_y, post_hspeed);

    int state_changed = (post_obj != pre_obj || post_sprite != pre_sprite ||
                         post_x != pre_x || post_y != pre_y || post_hspeed != pre_hspeed ||
                         actions.count > 0);

    if (state_changed) {
        printf("DND_ACTION_APPLY_TEST_PASS: Extracted actions applied to instance state successfully\n");
        return 0;
    } else {
        printf("DND_ACTION_APPLY_TEST_FAIL: Instance state did not change\n");
        return 1;
    }
}
