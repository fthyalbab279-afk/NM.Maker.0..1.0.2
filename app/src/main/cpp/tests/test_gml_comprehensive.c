#include "gm82_runtime.h"
#include "gm82_gml_builtins.h"
#include "gm82_gml_eval.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
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

    char sbuf[128];
    assert(gml_string_copy("GameMaker", 1, 4, sbuf, sizeof(sbuf)) == 4.0);
    assert(strcmp(sbuf, "Game") == 0);

    assert(gml_string_replace("foo bar", "bar", "baz", sbuf, sizeof(sbuf)) == 7.0);
    assert(strcmp(sbuf, "foo baz") == 0);

    assert(gml_string_replace_all("a b a b", "b", "c", sbuf, sizeof(sbuf)) == 7.0);
    assert(strcmp(sbuf, "a c a c") == 0);

    assert(gml_string_count("la", "lalala") == 3.0);

    assert(gml_string_delete("GameMaker", 5, 5, sbuf, sizeof(sbuf)) == 4.0);
    assert(strcmp(sbuf, "Game") == 0);

    assert(gml_string_insert("Maker", "Game", 5, sbuf, sizeof(sbuf)) == 9.0);
    assert(strcmp(sbuf, "GameMaker") == 0);

    /* Test math functions */
    assert(gml_lerp(0.0, 100.0, 0.5) == 50.0);
    assert(gml_clamp(150.0, 0.0, 100.0) == 100.0);
    assert(gml_point_distance(0, 0, 3, 4) == 5.0);
    assert(gml_point_direction(0, 0, 10, 0) == 0.0);
    assert(gml_lengthdir_x(10, 0) > 9.99);
    assert(gml_lengthdir_y(10, 90) < -9.99);
    assert(fabs(gml_dsin(90.0) - 1.0) < 0.0001);
    assert(fabs(gml_dcos(0.0) - 1.0) < 0.0001);
    assert(fabs(gml_darcsin(1.0) - 90.0) < 0.0001);

    /* Test GML expression evaluation of math, trig & vector functions */
    double dist = 0, dir = 0, lx = 0, val_sqr = 0, val_frac = 0, val_mean = 0, val_disp = 0, val_surf = 0;
    gm82_gml_eval_expr(&rt, inst, "point_distance(0, 0, 6, 8)", &dist);
    assert(dist == 10.0);
    gm82_gml_eval_expr(&rt, inst, "point_direction(0, 0, 0, 10)", &dir);
    assert(dir == 270.0);
    gm82_gml_eval_expr(&rt, inst, "lengthdir_x(5, 0)", &lx);
    assert(lx == 5.0);

    gm82_gml_eval_expr(&rt, inst, "sqr(4)", &val_sqr);
    assert(val_sqr == 16.0);
    gm82_gml_eval_expr(&rt, inst, "frac(5.75)", &val_frac);
    assert(fabs(val_frac - 0.75) < 0.0001);
    gm82_gml_eval_expr(&rt, inst, "mean(10, 20, 30)", &val_mean);
    assert(val_mean == 20.0);
    gm82_gml_eval_expr(&rt, inst, "display_get_width()", &val_disp);
    assert(val_disp > 0);

    /* Test surface lifecycle in evaluator */
    gm82_gml_eval_expr(&rt, inst, "surface_create(64, 64)", &val_surf);
    assert(val_surf >= 0);
    double surf_exists = 0;
    gm82_gml_eval_expr(&rt, inst, "surface_exists(0)", &surf_exists);
    assert(surf_exists == 1.0);
    gm82_gml_eval_expr(&rt, inst, "surface_free(0)", &surf_exists);

    puts("GML_COMPREHENSIVE_VM_TEST_PASS");
    return 0;
}
