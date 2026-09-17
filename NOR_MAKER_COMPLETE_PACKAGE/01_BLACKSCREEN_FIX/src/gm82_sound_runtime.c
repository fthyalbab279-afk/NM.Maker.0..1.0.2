#define _POSIX_C_SOURCE 200809L
#include "gm82_sound_runtime.h"
#include <string.h>
#include <stdio.h>

static gm82_sound_runtime *g_sr = NULL;

void gm82_sound_set_global(gm82_sound_runtime *sr) { g_sr = sr; }

void gm82_sound_runtime_init(gm82_sound_runtime *sr) {
    memset(sr, 0, sizeof(*sr));
    sr->last_played_index = -1;
}

void gm82_sound_runtime_bind(gm82_sound_runtime *sr, const gm82_decoded_sound_list *sounds) {
    sr->sounds = sounds;
}

int gm82_sound_play(gm82_sound_runtime *sr, int sound_index, int loop) {
    if (!sr || !sr->sounds) return 0;
    if (sound_index < 0 || sound_index >= sr->sounds->count) return 0;
    sr->play_count++;
    sr->last_played_index = sound_index;
    if (sr->queue_len < 32) {
        sr->queue[sr->queue_len].sound_index = sound_index;
        sr->queue[sr->queue_len].volume = sr->sounds->items[sound_index].volume;
        sr->queue[sr->queue_len].loop = loop;
        sr->queue_len++;
    }
    /* No device – event queued only */
    return 1;
}

int gm82_sound_play_by_name(gm82_sound_runtime *sr, const char *name, int loop) {
    if (!sr || !sr->sounds || !name) return 0;
    for (int i = 0; i < sr->sounds->count; i++) {
        if (strcmp(sr->sounds->items[i].name, name) == 0)
            return gm82_sound_play(sr, i, loop);
    }
    return 0;
}

void gm82_sound_stop_all(gm82_sound_runtime *sr) {
    if (!sr) return;
    sr->queue_len = 0;
}

double gml_sound_play(double sound_index) {
    if (!g_sr) return 0;
    return gm82_sound_play(g_sr, (int)sound_index, 0) ? 1.0 : 0.0;
}

double gml_sound_loop(double sound_index) {
    if (!g_sr) return 0;
    return gm82_sound_play(g_sr, (int)sound_index, 1) ? 1.0 : 0.0;
}

double gml_sound_stop_all(void) {
    if (!g_sr) return 0;
    gm82_sound_stop_all(g_sr);
    return 1;
}

double gml_sound_is_playing(double sound_index) {
    (void)sound_index;
    /* Without audio device we cannot know – return 0 honestly */
    return 0;
}

double gml_sound_exists(double sound_index) {
    if (!g_sr || !g_sr->sounds) return 0;
    int i = (int)sound_index;
    return (i >= 0 && i < g_sr->sounds->count) ? 1.0 : 0.0;
}

double gml_sound_volume(double sound_index, double volume) {
    if (!g_sr || !g_sr->sounds) return 0;
    int i = (int)sound_index;
    if (i < 0 || i >= g_sr->sounds->count) return 0;
    if (volume < 0) volume = 0;
    if (volume > 1) volume = 1;
    /* stored on decoded meta – actual gain needs device */
    ((gm82_decoded_sound *)g_sr->sounds->items)[i].volume = volume;
    return 1;
}

double gml_sound_get_name_count(void) {
    if (!g_sr || !g_sr->sounds) return 0;
    return (double)g_sr->sounds->count;
}
