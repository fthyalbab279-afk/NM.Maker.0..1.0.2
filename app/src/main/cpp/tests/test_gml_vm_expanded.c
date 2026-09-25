#include "gm82_runtime.h"
#include "gm82_gml_builtins.h"
#include "gm82_gml_eval.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>

int main(void) {
    puts("=== Testing Expanded GML VM Features (Arrays, INI I/O, Collision Circle) ===");

    gm82_runtime rt;
    gm82_runtime_init(&rt);
    rt.running = 1;
    gm82_gml_set_runtime(&rt);

    /* Test array builtins */
    assert(gml_array_length_1d(0) == 1.0);
    assert(gml_array_height_2d(0) == 1.0);

    /* Test string digits/lower/upper */
    char buf[64];
    gml_string_digits("A1B2C3", buf, sizeof(buf));
    assert(strcmp(buf, "123") == 0);

    gml_string_lower("HELLO", buf, sizeof(buf));
    assert(strcmp(buf, "hello") == 0);

    gml_string_upper("world", buf, sizeof(buf));
    assert(strcmp(buf, "WORLD") == 0);

    /* Test INI File I/O */
    const char *ini_path = "/tmp/test_save.ini";
    gml_ini_open(ini_path);
    gml_ini_write_real("save", "high_score", 5000.0);
    gml_ini_close();

    gml_ini_open(ini_path);
    assert(gml_ini_key_exists("save", "high_score") == 1.0);
    double score = gml_ini_read_real("save", "high_score", 0.0);
    assert(score == 5000.0);
    gml_ini_close();

    /* Test Collision Circle */
    gm82_instance *inst = gm82_runtime_instance_create(&rt, 0, 100, 100);
    assert(inst != NULL);
    double hit_id = gml_collision_circle(105, 105, 20, 0, 0, 0);
    assert(hit_id == (double)inst->id);

    puts("GML_VM_EXPANDED_TEST_PASS");
    return 0;
}
