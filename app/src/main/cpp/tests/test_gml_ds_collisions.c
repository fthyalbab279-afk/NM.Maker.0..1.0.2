#include "gm82_runtime.h"
#include "gm82_gml_builtins.h"
#include "gm82_gml_eval.h"
#include "gm82_sound_runtime.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>

static void test_bbox_builtins(gm82_runtime *rt) {
    gm82_instance *inst = gm82_runtime_instance_create(rt, 0, 50, 60);
    assert(inst != NULL);

    gm82_gml_set_self(inst);

    assert(gml_get_bbox_left() == 50.0);
    assert(gml_get_bbox_top() == 60.0);
    assert(gml_get_bbox_right() == 65.0);  /* 50 + 16 - 1 */
    assert(gml_get_bbox_bottom() == 75.0); /* 60 + 16 - 1 */

    double bbox_l = 0, bbox_t = 0;
    gm82_gml_eval_expr(rt, inst, "bbox_left", &bbox_l);
    gm82_gml_eval_expr(rt, inst, "bbox_top", &bbox_t);
    assert(bbox_l == 50.0);
    assert(bbox_t == 60.0);

    puts("test_bbox_builtins PASS");
}

static void test_ds_data_structures(void) {
    /* Test ds_list */
    double list_id = gml_ds_list_create();
    assert(list_id >= 0);
    assert(gml_ds_list_add(list_id, 42.0) == 1.0);
    assert(gml_ds_list_add(list_id, 99.0) == 1.0);
    assert(gml_ds_list_size(list_id) == 2.0);
    assert(gml_ds_list_find_value(list_id, 0) == 42.0);
    assert(gml_ds_list_find_value(list_id, 1) == 99.0);
    assert(gml_ds_list_destroy(list_id) == 1.0);

    /* Test ds_map */
    double map_id = gml_ds_map_create();
    assert(map_id >= 0);
    assert(gml_ds_map_add(map_id, 10, 500.0) == 1.0);
    assert(gml_ds_map_exists(map_id, 10) == 1.0);
    assert(gml_ds_map_find_value(map_id, 10) == 500.0);
    assert(gml_ds_map_destroy(map_id) == 1.0);

    puts("test_ds_data_structures PASS");
}

static void test_sound_controls(void) {
    gm82_decoded_sound items[2];
    memset(items, 0, sizeof(items));
    items[0].volume = 1.0;
    items[1].volume = 0.8;

    gm82_decoded_sound_list slist;
    slist.count = 2;
    slist.items = items;

    gm82_sound_runtime sr;
    gm82_sound_runtime_init(&sr);
    gm82_sound_runtime_bind(&sr, &slist);
    gm82_sound_set_global(&sr);

    assert(gml_sound_pitch(0, 1.25) == 1.0);
    assert(gml_sound_pan(0, -0.5) == 1.0);

    puts("test_sound_controls PASS");
}

static void test_collision_functions(gm82_runtime *rt) {
    gm82_instance *inst = gm82_runtime_instance_create(rt, 1, 100.0, 100.0);
    assert(inst != NULL);

    /* Point collision */
    assert(gml_collision_point(105.0, 105.0, 1, 0, 0) == (double)inst->id);
    assert(gml_collision_point(300.0, 300.0, 1, 0, 0) == -4.0);

    /* Rectangle collision */
    assert(gml_collision_rectangle(90.0, 90.0, 110.0, 110.0, 1, 0, 0) == (double)inst->id);
    assert(gml_collision_rectangle(0.0, 0.0, 10.0, 10.0, 1, 0, 0) == -4.0);

    /* Circle collision */
    assert(gml_collision_circle(100.0, 100.0, 20.0, 1, 0, 0) == (double)inst->id);
    assert(gml_collision_circle(0.0, 0.0, 5.0, 1, 0, 0) == -4.0);

    /* Line collision */
    assert(gml_collision_line(0.0, 0.0, 120.0, 120.0, 1, 0, 0) == (double)inst->id);
    assert(gml_collision_line(0.0, 0.0, 50.0, 50.0, 1, 0, 0) == -4.0);

    /* Ellipse collision */
    assert(gml_collision_ellipse(80.0, 80.0, 120.0, 120.0, 1, 0, 0) == (double)inst->id);
    assert(gml_collision_ellipse(0.0, 0.0, 10.0, 10.0, 1, 0, 0) == -4.0);

    puts("test_collision_functions PASS");
}

int main(void) {
    puts("=== Testing GML Data Structures & Collision Built-ins ===");
    gm82_runtime rt;
    gm82_runtime_init(&rt);
    gm82_gml_set_runtime(&rt);

    test_bbox_builtins(&rt);
    test_ds_data_structures();
    test_sound_controls();
    test_collision_functions(&rt);

    puts("GML_DS_COLLISIONS_TEST_PASS");
    return 0;
}
