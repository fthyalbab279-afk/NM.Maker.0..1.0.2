#include "gm82_runtime_guard.h"
#include "gm82_project_ir.h"
#include "gm82_materialize.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>

static void test_null_ir(void) {
    assert(gm82_runtime_can_play(NULL) == GM82_GUARD_NULL_IR);
    char buf[256];
    assert(!gm82_runtime_require_playable(NULL, buf, sizeof buf));
    puts("test_null_ir PASS");
}

static void test_incomplete_ir(void) {
    gm82_project_ir *ir = gm82_project_ir_create();
    assert(ir);
    ir->room_count = 1;                 /* has a room */
    ir->complete = false;               /* but not complete */
    ir->partial_count = 3;

    assert(gm82_runtime_can_play(ir) == GM82_GUARD_INCOMPLETE_IR);

    char buf[256];
    assert(!gm82_runtime_require_playable(ir, buf, sizeof buf));
    assert(strstr(buf, "incomplete") != NULL || strstr(buf, "Materialize") != NULL);

    gm82_project_ir_free(ir);
    puts("test_incomplete_ir PASS");
}

static void test_playable_ir(void) {
    gm82_project_ir *ir = gm82_project_ir_create();
    assert(ir);

    ir->room_count = 1;
    ir->rooms = calloc(1, sizeof(gm82_res_room));
    ir->rooms[0].id = 0;
    ir->rooms[0].width = 320;
    ir->rooms[0].height = 240;
    ir->rooms[0].status = GM82_RES_DECODED;

    ir->sprite_count = 1;
    ir->sprites = calloc(1, sizeof(gm82_res_sprite));
    ir->sprites[0].id = 0;
    ir->sprites[0].width = 16;
    ir->sprites[0].height = 16;
    ir->sprites[0].subimage_count = 1;
    ir->sprites[0].status = GM82_RES_DECODED;

    ir->background_count = 0;

    gm82_project_ir_recompute_complete(ir);
    assert(ir->complete == true);
    assert(gm82_runtime_can_play(ir) == GM82_GUARD_OK);

    char buf[256];
    assert(gm82_runtime_require_playable(ir, buf, sizeof buf));

    gm82_project_ir_free(ir);
    puts("test_playable_ir PASS");
}

static void test_materialize_keeps_guard_closed(void) {
    gm82_project_ir *ir = gm82_project_ir_create();
    ir->sprite_count = 1;
    ir->sprites = calloc(1, sizeof(gm82_res_sprite));
    ir->sprites[0].width = 16;
    ir->sprites[0].height = 16;
    ir->sprites[0].subimage_count = 1;
    ir->sprites[0].status = GM82_RES_RAW;   /* still raw */

    ir->room_count = 1;
    ir->rooms = calloc(1, sizeof(gm82_res_room));
    ir->rooms[0].width = 320;
    ir->rooms[0].height = 240;
    ir->rooms[0].status = GM82_RES_PARTIAL;

    gm82_materialize_options opts = gm82_materialize_default_options();
    bool ok = gm82_materialize_all(ir, &opts);

    /* Must stay false until real decoders exist */
    assert(ok == false);
    assert(ir->complete == false);
    assert(gm82_runtime_can_play(ir) != GM82_GUARD_OK);

    gm82_project_ir_free(ir);
    puts("test_materialize_keeps_guard_closed PASS");
}

int main(void) {
    test_null_ir();
    test_incomplete_ir();
    test_playable_ir();
    test_materialize_keeps_guard_closed();
    puts("ALL_RUNTIME_GUARD_TESTS_PASS");
    return 0;
}
