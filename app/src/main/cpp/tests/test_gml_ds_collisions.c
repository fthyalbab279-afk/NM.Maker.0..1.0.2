#include "gm82_runtime.h"
#include "gm82_gml_builtins.h"
#include "gm82_gml_eval.h"
#include "gm82_sound_runtime.h"
#include <stdio.h>
#include <stdlib.h>
#include <assert.h>

int main(void) {
    puts("=== Testing GML Data Structures & Collision Built-ins ===");

    gm82_runtime rt;
    gm82_runtime_init(&rt);
    gm82_gml_set_runtime(&rt);

    /* Test ds_list operations */
    double list_id = gml_ds_list_create();
    assert(list_id >= 0);
    gml_ds_list_add(list_id, 42.0);
    gml_ds_list_add(list_id, 100.0);
    assert(gml_ds_list_size(list_id) == 2);
    assert(gml_ds_list_find_value(list_id, 0) == 42.0);
    assert(gml_ds_list_find_value(list_id, 1) == 100.0);
    gml_ds_list_destroy(list_id);

    /* Test ds_map operations */
    double map_id = gml_ds_map_create();
    assert(map_id >= 0);
    gml_ds_map_add(map_id, 1, 999.0);
    assert(gml_ds_map_exists(map_id, 1) == 1.0);
    assert(gml_ds_map_find_value(map_id, 1) == 999.0);
    gml_ds_map_destroy(map_id);

    /* Test Sound Functions */
    assert(gml_sound_pitch(0, 1.2) == 1.0);
    assert(gml_sound_pan(0, 0.5) == 1.0);

    /* Test Instance Bounding Box evaluation */
    gm82_instance *inst = gm82_runtime_instance_create(&rt, 0, 50, 60);
    assert(inst != NULL);

    double bbox_l = 0, bbox_t = 0;
    gm82_gml_eval_expr(&rt, inst, "bbox_left", &bbox_l);
    gm82_gml_eval_expr(&rt, inst, "bbox_top", &bbox_t);
    assert(bbox_l == 50.0);
    assert(bbox_t == 60.0);

    puts("GML_DS_COLLISIONS_TEST_PASS");
    return 0;
}
