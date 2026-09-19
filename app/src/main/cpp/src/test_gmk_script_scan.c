#include "gm82_script.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static int test_file(const char *path) {
    printf("=== Testing %s ===\n", path);
    FILE *f = fopen(path, "rb");
    if (!f) { printf("FAIL: cannot open %s\n", path); return 0; }
    fseek(f, 0, SEEK_END);
    long sz = ftell(f);
    fseek(f, 0, SEEK_SET);
    uint8_t *buf = (uint8_t *)malloc((size_t)sz);
    fread(buf, 1, (size_t)sz, f);
    fclose(f);

    gm82_script_list scripts;
    int count = gm82_decode_scripts_from_gmk(buf, (size_t)sz, &scripts);
    free(buf);

    printf("scripts_found=%d\n", count);
    if (count > 0) {
        char first_80[81];
        strncpy(first_80, scripts.items[0].code, 80);
        first_80[80] = 0;
        printf("first_script_name=%s\n", scripts.items[0].name);
        printf("first_script_body_80=%s\n", first_80);
        size_t len = strlen(scripts.items[0].code);
        printf("first_script_body_len=%zu\n", len);
        if (len >= 20) {
            printf("FILE_RESULT: PASS\n");
            return 1;
        }
    }
    printf("FILE_RESULT: FAIL (scripts_found=%d)\n", count);
    return 0;
}

int main(int argc, char **argv) {
    const char *files[] = {
        "NOR_MAKER_COMPLETE_PACKAGE/02_SAMPLES/mario_bros.gmk",
        "NOR_MAKER_COMPLETE_PACKAGE/02_SAMPLES/plataformas.gmk",
        "NOR_MAKER_COMPLETE_PACKAGE/02_SAMPLES/zelda.gmk",
        "NOR_MAKER_COMPLETE_PACKAGE/02_SAMPLES/shooter.gmk"
    };

    int passed_any = 0;
    for (int i = 0; i < 4; i++) {
        if (test_file(files[i])) passed_any++;
    }

    if (passed_any > 0) {
        printf("SCRIPT_SCAN_TEST_SUITE_PASS: Total sample files passing script decode test = %d/4\n", passed_any);
        return 0;
    } else {
        printf("SCRIPT_SCAN_TEST_SUITE_FAIL: 0 files passed script decode test\n");
        return 1;
    }
}
