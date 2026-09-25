#include "gm82_runtime.h"
#include "gm82_actions.h"
#include "gm82_gml_builtins.h"
#include <stdio.h>
#include <stdlib.h>
#include <assert.h>

int main(void) {
    puts("=== Testing DnD Actions Execution Suite ===");

    gm82_runtime rt;
    gm82_runtime_init(&rt);
    rt.running = 1;
    gm82_gml_set_runtime(&rt);

    gm82_instance *inst = gm82_runtime_instance_create(&rt, 0, 50, 50);
    assert(inst != NULL);
    gm82_gml_set_self(inst);

    /* Test named action executions */
    gm82_action_execute_named(&rt, inst, "action_set_score");
    assert(gml_get_score() == 0.0);

    gm82_action_execute_named(&rt, inst, "action_set_lives");
    assert(gml_get_lives() == 3.0);

    gm82_action_execute_named(&rt, inst, "action_set_health");
    assert(gml_get_health() == 100.0);

    /* Test ref action executions */
    gm82_action_table table;
    memset(&table, 0, sizeof(table));

    gm82_object_actions oa;
    memset(&oa, 0, sizeof(oa));
    snprintf(oa.object_name, sizeof(oa.object_name), "obj_test");

    gm82_action_ref refs[3];
    memset(refs, 0, sizeof(refs));

    /* Action 1: set score to 2500 */
    refs[0].event_type = 0;
    snprintf(refs[0].name, sizeof(refs[0].name), "action_set_score");
    refs[0].action_id = 2500;

    /* Action 2: set lives to 5 */
    refs[1].event_type = 0;
    snprintf(refs[1].name, sizeof(refs[1].name), "action_set_lives");
    refs[1].action_id = 5;

    /* Action 3: set health to 80 */
    refs[2].event_type = 0;
    snprintf(refs[2].name, sizeof(refs[2].name), "action_set_health");
    refs[2].action_id = 80;

    oa.count = 3;
    oa.items = refs;

    table.count = 1;
    table.objects = &oa;

    /* Bind dummy object list so fire_create matches obj_test */
    gm82_decoded_object_list objs;
    memset(&objs, 0, sizeof(objs));
    gm82_decoded_object obj_item;
    memset(&obj_item, 0, sizeof(obj_item));
    snprintf(obj_item.name, sizeof(obj_item.name), "obj_test");
    objs.count = 1;
    objs.items = &obj_item;
    rt.objects = &objs;

    gm82_actions_fire_create(&rt, inst, &table);

    assert(gml_get_score() == 2500.0);
    assert(gml_get_lives() == 5.0);
    assert(gml_get_health() == 80.0);

    puts("DND_ACTIONS_TEST_PASS");
    return 0;
}
