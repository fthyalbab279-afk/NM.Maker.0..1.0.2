#include "gm82_sound_decode.h"
#include "gm82_sound_runtime.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static int g_cmd_count = 0;
static int g_last_kind = 0;
static int g_last_sound_id = 0;

void gm82_enqueue_sound_command(int kind, int soundId, int loop, int prio, float volume) {
    g_cmd_count++;
    g_last_kind = kind;
    g_last_sound_id = soundId;
    (void)loop; (void)prio; (void)volume;
}

int main(int argc, char **argv) {
    const char *path = (argc > 1) ? argv[1] : "NOR_MAKER_COMPLETE_PACKAGE/02_SAMPLES/shooter.gmk";
    printf("=== Testing Audio Device Enqueue on %s ===\n", path);

    FILE *f = fopen(path, "rb");
    if (!f) { printf("FAIL: cannot open %s\n", path); return 1; }
    fseek(f, 0, SEEK_END);
    long sz = ftell(f);
    fseek(f, 0, SEEK_SET);
    uint8_t *buf = (uint8_t *)malloc((size_t)sz);
    fread(buf, 1, (size_t)sz, f);
    fclose(f);

    gm82_decoded_sound_list sounds;
    int sound_count = gm82_decode_sounds_from_gmk(buf, (size_t)sz, &sounds);
    free(buf);

    printf("sounds_decoded=%d\n", sound_count);

    gm82_sound_runtime sr;
    gm82_sound_runtime_init(&sr);
    gm82_sound_runtime_bind(&sr, &sounds);
    gm82_sound_set_global(&sr);

    gml_sound_play(0);
    gml_sound_play(1);

    printf("g_cmd_count=%d | last_kind=%d | last_sound_id=%d\n",
           g_cmd_count, g_last_kind, g_last_sound_id);

    gm82_decoded_sound_list_free(&sounds);

    if (g_cmd_count >= 2 && sound_count > 0) {
        printf("AUDIO_DEVICE_TEST_PASS: Enqueued %d audio commands successfully\n", g_cmd_count);
        return 0;
    } else {
        printf("AUDIO_DEVICE_TEST_FAIL: Enqueued %d commands (expected >= 2)\n", g_cmd_count);
        return 1;
    }
}
