#include "gm82_script.h"
#include "gm82_gml_eval.h"
#include "gm82_runtime.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

int main(int argc, char **argv) {
    gm82_runtime rt;
    gm82_runtime_init(&rt);
    gm82_instance *inst = gm82_runtime_instance_create(&rt, 0, 100, 200);

    /* Test script argument binding */
    double args[3] = { 15.0, 25.0, 5.0 };
    gm82_gml_set_script_args(args, 3);

    double val0 = gm82_gml_get_script_arg(0);
    double val1 = gm82_gml_get_script_arg(1);
    double val2 = gm82_gml_get_script_arg(2);

    if (val0 != 15.0 || val1 != 25.0 || val2 != 5.0) {
        printf("FAIL: script args unexpected values %f %f %f\n", val0, val1, val2);
        return 1;
    }

    /* Test evaluating code block with argument0, argument1 */
    const char *code = "x = argument0 + argument1; y = argument1 - argument2;";
    gm82_gml_eval_block(&rt, inst, code);

    if (inst->x != 40.0 || inst->y != 20.0) {
        printf("FAIL: evaluated instance x=%.1f y=%.1f, expected x=40.0 y=20.0\n", inst->x, inst->y);
        return 1;
    }

    printf("GMK_SCRIPT_TEST_PASS: Script arguments evaluated successfully (x=%.1f, y=%.1f)\n", inst->x, inst->y);
    return 0;
}
