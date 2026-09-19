#include "gm82_runtime.h"
#include "gm82_gmk_reader.h"
#include "gm82_sprite_decode.h"
#include "gm82_background_decode.h"
#include "gm82_object_room_decode.h"
#include "gm82_actions.h"
#include "gm82_events.h"
#include "gm82_script.h"
#include "gm82_gml_eval.h"
#include "gm82_input.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

int main(int argc, char **argv) {
    const char *path = (argc > 1) ? argv[1] : "NOR_MAKER_COMPLETE_PACKAGE/02_SAMPLES/plataformas.gmk";
    printf("=== Testing Script Execution State Change on %s ===\n", path);

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
    gm82_script_list scripts;

    gm82_decode_sprites_from_gmk(buf, (size_t)sz, &sprites);
    gm82_decode_backgrounds_from_gmk(buf, (size_t)sz, &bgs);
    gm82_decode_objects_from_gmk(buf, (size_t)sz, &objs);
    gm82_decode_rooms_from_gmk(buf, (size_t)sz, &rooms);
    gm82_actions_scan_gmk(buf, (size_t)sz, &actions);
    gm82_decode_scripts_from_gmk(buf, (size_t)sz, &scripts);
    free(buf);

    gm82_runtime rt;
    gm82_runtime_init(&rt);
    gm82_runtime_bind_assets(&rt, &objs, &sprites, &bgs, &rooms, &actions);
    gm82_runtime_bind_scripts(&rt, &scripts);

    gm82_runtime_goto_room(&rt, 0);

    if (rt.instance_count == 0) {
        printf("FAIL: 0 instances in room 0\n");
        return 1;
    }

    gm82_instance *inst = &rt.instances[0];

    /* Place instance in air to trigger gravity logic in Juego_plataformas script */
    inst->y = 100.0;

    /* Find script Juego_plataformas or fallback to first script */
    int sidx = gm82_script_find(&scripts, "Juego_plataformas");
    if (sidx < 0 && scripts.count > 0) sidx = 0;

    if (sidx < 0) {
        printf("FAIL: no script found\n");
        return 1;
    }

    const char *sname = scripts.items[sidx].name;
    printf("Selected script [%d]: name=%s\n", sidx, sname);

    double init_grav = inst->gravity;
    double init_y = inst->y;
    printf("Pre-exec instance 0: gravity=%.2f y=%.2f vspeed=%.2f\n", inst->gravity, inst->y, inst->vspeed);

    /* Call script by name on instance */
    char call_str[128];
    snprintf(call_str, sizeof(call_str), "%s();", sname);
    gm82_gml_eval_stmt(&rt, inst, call_str);

    printf("Post-exec script call: gravity=%.2f gravity_dir=%.1f\n", inst->gravity, inst->gravity_direction);

    /* Step runtime for 5 steps */
    for (int step = 1; step <= 5; step++) {
        gm82_runtime_step(&rt);
    }

    printf("Post-step frame 5: x=%.2f y=%.2f vspeed=%.2f\n", inst->x, inst->y, inst->vspeed);

    int state_changed = (inst->gravity != init_grav || inst->y != init_y || inst->vspeed != 0.0);

    if (state_changed) {
        printf("SCRIPT_STATE_CHANGE_PASS: Script %s executed and modified instance state (gravity=%.2f, y=%.2f, vspeed=%.2f)\n",
               sname, inst->gravity, inst->y, inst->vspeed);
        return 0;
    } else {
        printf("SCRIPT_STATE_CHANGE_FAIL: Instance state did not change\n");
        return 1;
    }
}
