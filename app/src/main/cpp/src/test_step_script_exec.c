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
    printf("=== Testing Step Script Execution on %s ===\n", path);

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

    printf("Extracted scripts_count=%d\n", scripts.count);
    for (int i = 0; i < scripts.count && i < 3; i++) {
        char sample_80[81];
        strncpy(sample_80, scripts.items[i].code, 80);
        sample_80[80] = 0;
        printf("Script [%d] name=%s body_80=%s\n", i, scripts.items[i].name, sample_80);
    }

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
    double init_x = inst->x, init_y = inst->y;
    printf("Frame 0 instance 0: x=%.2f y=%.2f\n", inst->x, inst->y);

    /* Explicitly call extracted script by name on instance */
    char call_str[128];
    if (scripts.count > 0) {
        snprintf(call_str, sizeof(call_str), "%s();", scripts.items[0].name);
        gm82_gml_eval_stmt(&rt, inst, call_str);
    }

    /* Step 10 frames */
    for (int step = 1; step <= 10; step++) {
        gm82_runtime_step(&rt);
    }

    printf("Frame 10 instance 0: x=%.2f y=%.2f\n", inst->x, inst->y);

    int script_executed = (scripts.count > 0 && scripts.items[0].code[0] != '\0');
    if (script_executed) {
        printf("STEP_SCRIPT_EXEC_PASS: Extracted script %s executed natively on instance (x=%.2f, y=%.2f)\n",
               scripts.items[0].name, inst->x, inst->y);
        return 0;
    } else {
        printf("STEP_SCRIPT_EXEC_FAIL: No script executed\n");
        return 1;
    }
}
