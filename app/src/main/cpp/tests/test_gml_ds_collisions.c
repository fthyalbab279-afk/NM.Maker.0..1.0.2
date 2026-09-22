#include "gm82_gml_builtins.h"
#include "gm82_sound_runtime.h"
#include "gm82_gml_eval.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>

static void test_bbox_builtins(void) {
    gm82_instance inst;
    memset(&inst, 0, sizeof(inst));
    inst.x = 100.0;
    inst.y = 200.0;
    inst.sprite_index = -1;

    gm82_gml_set_self(&inst);

    assert(gml_get_bbox_left() == 100.0);
    assert(gml_get_bbox_top() == 200.0);
    assert(gml_get_bbox_right() == 115.0);  /* 100 + 16 - 1 */
    assert(gml_get_bbox_bottom() == 215.0); /* 200 + 16 - 1 */

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

int main(void) {
    puts("--- Running GML DS & Collisions Unit Suite ---");
    test_bbox_builtins();
    test_ds_data_structures();
    test_sound_controls();
    puts("--- GML DS & Collisions Unit Suite PASS ---");
    return 0;
}
