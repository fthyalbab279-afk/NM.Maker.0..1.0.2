#define _POSIX_C_SOURCE 200809L
#include "gm82_runtime.h"
#include "gm82_loader.h"
#include "gm82_runtime_guard.h"
#include "gm82_events.h"
#include "gm82_actions.h"
#include "gm82_gml_eval.h"
#include "gm82_gml_builtins.h"
#include "gm82_input.h"
#include "gm82_sound_runtime.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>

static void test_sample(const char *path) {
    printf("Validating sample: %s\n", path);
    char err[512] = {0};
    gm82_project_ir *ir = NULL;
    bool playable = gm82_load_and_prepare(path, &ir, err, sizeof(err));
    assert(playable == true);
    assert(ir != NULL);
    assert(ir->complete == true);
    assert(ir->sprite_count > 0);
    assert(ir->room_count > 0);
    printf("  [OK] %s: sprites=%d rooms=%d complete=%d\n",
           path, ir->sprite_count, ir->room_count, ir->complete);
    gm82_project_ir_free(ir);
}

int main(void) {
    puts("=================================================");
    puts("=== GM82 CORE FULL VALIDATION & REGRESSION ===");
    puts("=================================================");

    test_sample("app/src/main/assets/www/samples/mario_bros.gmk");
    test_sample("app/src/main/assets/www/samples/plataformas.gmk");
    test_sample("app/src/main/assets/www/samples/zelda.gmk");
    test_sample("app/src/main/assets/www/samples/shooter.gmk");

    puts("=================================================");
    puts("=== ALL 4 SAMPLE PROJECTS PASSED 100% VALIDATION! ===");
    puts("=================================================");
    return 0;
}
