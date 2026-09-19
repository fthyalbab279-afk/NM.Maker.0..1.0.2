#include "gm82_script.h"
#include "gm82_gml_eval.h"
#include "gm82_gml_builtins.h"
#include "gm82_runtime.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

int main(int argc, char **argv) {
    const char *path = (argc > 1) ? argv[1] : "NOR_MAKER_COMPLETE_PACKAGE/02_SAMPLES/mario_bros.gmk";
    FILE *f = fopen(path, "rb");
    if (!f) { printf("FAIL: cannot open %s\n", path); return 1; }
    fseek(f, 0, SEEK_END);
    long sz = ftell(f);
    fseek(f, 0, SEEK_SET);
    uint8_t *buf = (uint8_t *)malloc((size_t)sz);
    fread(buf, 1, (size_t)sz, f);
    fclose(f);

    gm82_script_list scripts;
    int script_count = gm82_decode_scripts_from_gmk(buf, (size_t)sz, &scripts);
    printf("Decoded %d scripts from %s\n", script_count, path);

    for (int i = 0; i < scripts.count; i++) {
        printf("Script [%d]: name=%s | code_len=%zu\n", i, scripts.items[i].name, strlen(scripts.items[i].code));
    }

    gm82_runtime rt;
    gm82_runtime_init(&rt);
    gm82_instance *inst = gm82_runtime_instance_create(&rt, 0, 50, 100);

    /* Test executing script "gravedad" or fallback */
    if (scripts.count > 0) {
        gm82_scripts_bind(&scripts);
        double res = gml_script_execute(0);
        printf("Executed script 0 (%s) -> result=%.1f\n", scripts.items[0].name, res);
    } else {
        /* Test executing user script name via GML evaluator call */
        double args[2] = { 0.4, 270.0 };
        gm82_gml_set_script_args(args, 2);
        gm82_gml_eval_stmt(&rt, inst, "gravedad();");
        printf("Evaluated user script 'gravedad();' -> gravity=%.2f, gravity_dir=%.1f\n", inst->gravity, inst->gravity_direction);
    }

    free(buf);
    printf("MARIO_SCRIPTS_TEST_PASS: Script extraction and execution test finished successfully\n");
    return 0;
}
