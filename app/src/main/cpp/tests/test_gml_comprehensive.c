#include "gm82_runtime.h"
#include "gm82_gml_builtins.h"
#include "gm82_gml_eval.h"
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

void test_eval_control_flow(void) {
    gm82_runtime rt;
    gm82_runtime_init(&rt);
    rt.running = 1;

    gm82_instance *inst = gm82_runtime_instance_create(&rt, 0, 10, 20);
    assert(inst != NULL);

    /* Test while loop */
    inst->x = 0;
    gm82_gml_eval_stmt(&rt, inst, "while (x < 5) x += 1;");
    assert(inst->x == 5.0);

    /* Test do ... until loop */
    inst->y = 0;
    gm82_gml_eval_stmt(&rt, inst, "do { y += 2; } until (y >= 10);");
    assert(inst->y == 10.0);

    printf("  Control Flow Eval PASS\n");
}

void test_view_variables(void) {
    gm82_runtime rt;
    gm82_runtime_init(&rt);
    gm82_instance *inst = gm82_runtime_instance_create(&rt, 0, 0, 0);

    gm82_gml_eval_block(&rt, inst, "view_xview = 120; view_yview = 80;");
    assert(rt.view_x == 120.0);
    assert(rt.view_y == 80.0);

    double vx = 0, vy = 0;
    gm82_gml_eval_expr(&rt, inst, "view_xview", &vx);
    gm82_gml_eval_expr(&rt, inst, "view_yview", &vy);
    assert(vx == 120.0);
    assert(vy == 80.0);

    printf("  View Variables Eval PASS\n");
}

int main(void) {
    printf("--- Running GML Comprehensive Test Suite ---\n");
    test_math_builtins();
    test_ds_structures();
    test_string_builtins();
    test_ini_and_file_io();
    test_eval_control_flow();
    test_view_variables();
    printf("--- GML Comprehensive Tests Passed! ---\n");
    return 0;
}
