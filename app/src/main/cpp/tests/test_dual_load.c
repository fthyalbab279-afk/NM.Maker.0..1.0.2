#include "gm82_loader.h"
#include "gm82_project_ir.h"
#include "gm82_runtime_guard.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>

static void test_gmk_sample(const char *path) {
    printf("Loading %s ...\n", path);
    char err[512];
    gm82_project_ir *ir = NULL;
    bool playable = gm82_load_and_prepare(path, &ir, err, sizeof(err));

    /* Should be playable when sprites/backgrounds materialize */
    assert(playable == true);
    assert(ir != NULL);
    assert(ir->complete == true);
    printf("  playable=%d, complete=%d, partial_count=%d\n",
           playable, ir->complete, ir->partial_count);
    printf("  message: %s\n", err);
    gm82_project_ir_free(ir);
    puts("test_gmk_sample PASS");
}

static void test_auto_detect(void) {
    gm82_load_result r = gm82_load_auto("/tmp/does_not_exist.gmk");
    assert(r.ok == false);
    puts("test_auto_detect (missing file) PASS");
}

int main(int argc, char **argv) {
    const char *sample = NULL;
    if (argc > 1) sample = argv[1];
    else sample = "app/src/main/assets/www/samples/mario_bros.gmk";

    test_auto_detect();
    test_gmk_sample(sample);
    puts("ALL_DUAL_LOAD_TESTS_PASS");
    return 0;
}
