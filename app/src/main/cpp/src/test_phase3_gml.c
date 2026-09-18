#define _POSIX_C_SOURCE 200809L
#include "gm82_runtime.h"
#include "gm82_gml_eval.h"
#include "gm82_gml_builtins.h"
#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <string.h>

int main(void) {
    gm82_runtime rt;
    gm82_runtime_init(&rt);
    rt.running = 1;

    gm82_instance *inst = gm82_runtime_instance_create(&rt, 0, 100, 200);
    assert(inst != NULL);

    /* Test simple assignment and variable read */
    gm82_gml_eval_stmt(&rt, inst, "x = 150");
    assert(inst->x == 150);

    /* Test custom instance variable */
    gm82_gml_eval_stmt(&rt, inst, "coins = 10");
    double coins_val = 0;
    gm82_gml_eval_expr(&rt, inst, "coins", &coins_val);
    assert(coins_val == 10);

    /* Test compound assignment */
    gm82_gml_eval_stmt(&rt, inst, "coins += 5");
    gm82_gml_eval_expr(&rt, inst, "coins", &coins_val);
    assert(coins_val == 15);

    /* Test if condition */
    gm82_gml_eval_stmt(&rt, inst, "if (coins > 10) x = 300");
    assert(inst->x == 300);

    /* Test repeat loop */
    gm82_gml_eval_stmt(&rt, inst, "repeat (3) coins += 1");
    gm82_gml_eval_expr(&rt, inst, "coins", &coins_val);
    assert(coins_val == 18);

    /* Test with statement on instances */
    gm82_instance *inst2 = gm82_runtime_instance_create(&rt, 1, 50, 50);
    assert(inst2 != NULL);
    gm82_gml_eval_stmt(&rt, inst, "with (1) x = 77");
    assert(inst2->x == 77);

    puts("PHASE3_GML_TESTS_PASS");
    return 0;
}
