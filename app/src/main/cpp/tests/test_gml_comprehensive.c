#include "gm82_gml_builtins.h"
#include "gm82_runtime.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <math.h>

void test_math_builtins(void) {
    assert(fabs(gml_abs(-15.5) - 15.5) < 0.001);
    assert(gml_sign(-10.0) == -1.0);
    assert(gml_sign(0.0) == 0.0);
    assert(gml_sign(25.0) == 1.0);
    assert(gml_clamp(15.0, 0.0, 10.0) == 10.0);
    assert(gml_lerp(0.0, 100.0, 0.5) == 50.0);
    assert(fabs(gml_point_distance(0.0, 0.0, 3.0, 4.0) - 5.0) < 0.001);
    printf("  Math Builtins PASS\n");
}

void test_ds_structures(void) {
    /* Test DS List */
    double list_id = gml_ds_list_create();
    assert(list_id >= 0);
    assert(gml_ds_list_add(list_id, 100.0) == 1.0);
    assert(gml_ds_list_add(list_id, 200.0) == 1.0);
    assert(gml_ds_list_size(list_id) == 2.0);
    assert(gml_ds_list_find_value(list_id, 0.0) == 100.0);
    assert(gml_ds_list_find_value(list_id, 1.0) == 200.0);
    assert(gml_ds_list_destroy(list_id) == 1.0);

    /* Test DS Map */
    double map_id = gml_ds_map_create();
    assert(map_id >= 0);
    assert(gml_ds_map_add(map_id, 10.0, 500.0) == 1.0);
    assert(gml_ds_map_exists(map_id, 10.0) == 1.0);
    assert(gml_ds_map_find_value(map_id, 10.0) == 500.0);
    assert(gml_ds_map_size(map_id) == 1.0);
    assert(gml_ds_map_destroy(map_id) == 1.0);

    /* Test DS Stack */
    double stack_id = gml_ds_stack_create();
    assert(stack_id >= 0);
    assert(gml_ds_stack_push(stack_id, 42.0) == 1.0);
    assert(gml_ds_stack_size(stack_id) == 1.0);
    assert(gml_ds_stack_pop(stack_id) == 42.0);
    assert(gml_ds_stack_empty(stack_id) == 1.0);
    assert(gml_ds_stack_destroy(stack_id) == 1.0);

    /* Test DS Queue */
    double queue_id = gml_ds_queue_create();
    assert(queue_id >= 0);
    assert(gml_ds_queue_enqueue(queue_id, 88.0) == 1.0);
    assert(gml_ds_queue_size(queue_id) == 1.0);
    assert(gml_ds_queue_dequeue(queue_id) == 88.0);
    assert(gml_ds_queue_empty(queue_id) == 1.0);
    assert(gml_ds_queue_destroy(queue_id) == 1.0);

    printf("  DS Structures (List, Map, Stack, Queue) PASS\n");
}

void test_string_builtins(void) {
    assert(gml_string_length("NOR_Maker") == 9.0);
    assert(gml_real("123.45") == 123.45);
    printf("  String Builtins PASS\n");
}

void test_ini_and_file_io(void) {
    const char *tmp_ini = "/tmp/test_config.ini";
    assert(gml_ini_open(tmp_ini) == 1.0);
    assert(gml_ini_write_real("Settings", "Volume", 80.0) == 1.0);
    assert(gml_ini_close() == 1.0);

    assert(gml_ini_open(tmp_ini) == 1.0);
    assert(gml_ini_read_real("Settings", "Volume", 0.0) == 80.0);
    assert(gml_ini_close() == 1.0);

    printf("  INI & File I/O PASS\n");
}

int main(void) {
    printf("--- Running GML Comprehensive Test Suite ---\n");
    test_math_builtins();
    test_ds_structures();
    test_string_builtins();
    test_ini_and_file_io();
    printf("--- GML Comprehensive Tests Passed! ---\n");
    return 0;
}
