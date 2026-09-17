#include <assert.h>
#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include "../android_project_base/app/src/main/cpp/gm82_gmk_reader.h"

static void put32(uint8_t *p, uint32_t v) {
    p[0] = (uint8_t)v; p[1] = (uint8_t)(v >> 8);
    p[2] = (uint8_t)(v >> 16); p[3] = (uint8_t)(v >> 24);
}

int main(void) {
    uint8_t h[12] = {0};
    gm82_gmk_probe_result r = gm82_gmk_probe(h, 0);
    assert(r.status == GM82_GMK_PARSE_INVALID);
    assert(strcmp(r.error_code, "buffer_too_small") == 0);

    put32(h + 0, 1234321u);
    put32(h + 4, 800u);
    put32(h + 8, 42u);
    r = gm82_gmk_probe(h, sizeof(h));
    assert(r.format_kind == GM82_GMK_FORMAT_GM7_GM8);
    assert(r.status == GM82_GMK_PARSE_PARTIAL);
    assert(r.version == 800 && r.app_id == 42);
    assert(r.error_code == NULL);

    put32(h + 0, 0u);
    r = gm82_gmk_probe(h, sizeof(h));
    assert(r.status == GM82_GMK_PARSE_INVALID);
    assert(strcmp(r.error_code, "bad_magic") == 0);

    put32(h + 0, 1234321u);
    put32(h + 4, 499u);
    r = gm82_gmk_probe(h, sizeof(h));
    assert(r.status == GM82_GMK_PARSE_INVALID);
    assert(strcmp(r.error_code, "unsupported_version") == 0);

    puts("GMK_PROBE_CONTRACT_PASS");
    return 0;
}
