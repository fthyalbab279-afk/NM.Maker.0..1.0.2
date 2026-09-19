#include "gm82_actions.h"
#include "gm82_gmk_reader.h"
#include <stdio.h>
#include <stdlib.h>

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

    gm82_action_table table;
    int obj_count = gm82_actions_scan_gmk(buf, (size_t)sz, &table);
    printf("Scanned %d object action groups from %s\n", obj_count, path);

    int printed = 0;
    for (int i = 0; i < table.count; i++) {
        for (int a = 0; a < table.objects[i].count; a++) {
            gm82_action_ref *ar = &table.objects[i].items[a];
            printf("Obj: %s | Action: %s | EventType: %d | ActionID/Arg0: %d | Kind/Arg1: %d\n",
                   table.objects[i].object_name, ar->name, ar->event_type, ar->action_id, ar->kind);
            printed++;
            if (printed >= 10) break;
        }
        if (printed >= 10) break;
    }

    gm82_action_table_free(&table);
    free(buf);
    if (printed >= 3) {
        printf("DND_EXTRACTION_TEST_PASS: Extracted %d real actions with args\n", printed);
        return 0;
    } else {
        printf("DND_EXTRACTION_TEST_FAIL: Extracted only %d actions\n", printed);
        return 1;
    }
}
