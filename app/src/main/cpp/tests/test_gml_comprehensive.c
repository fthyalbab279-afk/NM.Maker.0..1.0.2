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
    double list_id = gml_ds_list_create();
    assert(list_id >= 0);
    assert(gml_ds_list_add(list_id, 100.0) == 1.0);
    assert(gml_ds_list_add(list_id, 200.0) == 1.0);
    assert(gml_ds_list_size(list_id) == 2.0);
    assert(gml_ds_list_find_value(list_id, 0.0) == 100.0);
    assert(gml_ds_list_find_value(list_id, 1.0) == 200.0);
    assert(gml_ds_list_destroy(list_id) == 1.0);
    printf("  DS Structures PASS\n");
}

void test_string_builtins(void) {
    assert(gml_string_length("NOR_Maker") == 9.0);
    assert(gml_real("123.45") == 123.45);
    printf("  String Builtins PASS\n");
}

int main(void) {
    printf("--- Running GML Comprehensive Test Suite ---\n");
    test_math_builtins();
    test_ds_structures();
    test_string_builtins();
    printf("--- GML Comprehensive Tests Passed! ---\n");
    return 0;
}
