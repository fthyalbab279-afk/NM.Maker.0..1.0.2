#define _POSIX_C_SOURCE 200809L
#include "gm82_runtime.h"
#include "gm82_events.h"
#include "gm82_actions.h"
#include "gm82_gml_builtins.h"
#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <string.h>

int main(void) {
    gm82_runtime rt;
    gm82_runtime_init(&rt);
    rt.running = 1;
    gm82_events_register_defaults();

    /* Create test instances */
    gm82_instance *inst1 = gm82_runtime_instance_create(&rt, 0, 10, 20);
    assert(inst1 != NULL);
    assert(inst1->id == 100001);
    assert(inst1->x == 10);
    assert(inst1->y == 20);
    assert(inst1->alive == 1);

    gm82_instance *inst2 = gm82_runtime_instance_create(&rt, 1, 50, 60);
    assert(inst2 != NULL);
    assert(inst2->id == 100002);

    /* Test Alarms */
    inst1->alarms[0] = 2;
    gm82_runtime_step(&rt); /* alarm 0 becomes 1 */
    assert(inst1->alarms[0] == 1);
    gm82_runtime_step(&rt); /* alarm 0 fires (reaches 0 then set to -1) */
    assert(inst1->alarms[0] == -1);

    /* Test instance destroy */
    gm82_runtime_instance_destroy(&rt, inst2);
    assert(inst2->alive == 0);

    /* Test action execution named */
    inst1->hspeed = 5.0;
    gm82_action_execute_named(&rt, inst1, "action_bounce");
    /* bounce toggles speed depending on vspeed vs hspeed */

    puts("PHASE2_EVENTS_TESTS_PASS");
    return 0;
}
