#include "gm82_runtime.h"
#include "gm82_gml_builtins.h"
#include "gm82_gml_eval.h"
#include <stdio.h>
#include <stdlib.h>
#include <assert.h>

int main(void) {
    puts("=== Testing Comprehensive GML VM Control Flow & String Functions ===");

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

    /* Test string functions */
    assert(gml_string_length("Hello") == 5.0);
    assert(gml_string_pos("world", "Hello world") == 7.0);
    assert(gml_string_char_at("ABC", 2) == (double)'B');

    /* Test math functions */
    assert(gml_lerp(0.0, 100.0, 0.5) == 50.0);
    assert(gml_clamp(150.0, 0.0, 100.0) == 100.0);

    puts("GML_COMPREHENSIVE_VM_TEST_PASS");
    return 0;
}
