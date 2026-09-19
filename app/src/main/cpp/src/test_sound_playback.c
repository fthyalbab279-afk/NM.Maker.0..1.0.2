#include "gm82_sound_decode.h"
#include "gm82_sound_runtime.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

int main(int argc, char **argv) {
    const char *path = (argc > 1) ? argv[1] : "NOR_MAKER_COMPLETE_PACKAGE/02_SAMPLES/shooter.gmk";
    printf("=== Testing Sound Playback on %s ===\n", path);

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
    for (int i = 0; i < sounds.count && i < 10; i++) {
        printf("Sound [%d]: name=%s | kind=%d | volume=%.2f\n",
               i, sounds.items[i].name, sounds.items[i].kind, sounds.items[i].volume);
    }

    gm82_sound_runtime sr;
    gm82_sound_runtime_init(&sr);
    gm82_sound_runtime_bind(&sr, &sounds);
    gm82_sound_set_global(&sr);

    /* Play sound 0 via index and via GML function */
    int played1 = gm82_sound_play(&sr, 0, 0);
    double played2 = gml_sound_play(0);

    printf("play_count=%d | queue_len=%d | last_played=%d\n",
           sr.play_count, sr.queue_len, sr.last_played_index);

    if (sound_count > 0 && sr.play_count >= 2 && sr.queue_len >= 2) {
        printf("SOUND_PLAYBACK_TEST_PASS: Decoded %d sounds and queued playback successfully\n", sound_count);
        gm82_decoded_sound_list_free(&sounds);
        return 0;
    } else if (sound_count == 0) {
        printf("SOUND_PLAYBACK_TEST_FAIL: 0 sounds decoded\n");
        gm82_decoded_sound_list_free(&sounds);
        return 1;
    } else {
        printf("SOUND_PLAYBACK_TEST_FAIL: Sound queuing failed (played1=%d, played2=%.0f)\n", played1, played2);
        gm82_decoded_sound_list_free(&sounds);
        return 1;
    }
}
