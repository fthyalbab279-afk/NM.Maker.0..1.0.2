#ifndef GM82_SOUND_RUNTIME_H
#define GM82_SOUND_RUNTIME_H

#include "gm82_sound_decode.h"
#include <stdint.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

/*
 * Sound runtime – queues play requests only.
 * NO actual audio device output (no OpenSL/AAudio wired).
 * gml_sound_play increments counters so games don't crash on missing audio.
 */

typedef struct {
    int sound_index;
    double volume;
    int loop;
} gm82_sound_event;

typedef struct {
    const gm82_decoded_sound_list *sounds;
    int play_count;          /* total sound_play calls */
    int last_played_index;
    gm82_sound_event queue[32];
    int queue_len;
} gm82_sound_runtime;

void gm82_sound_runtime_init(gm82_sound_runtime *sr);
void gm82_sound_runtime_bind(gm82_sound_runtime *sr, const gm82_decoded_sound_list *sounds);

/* Returns 1 if sound index valid and queued, 0 otherwise */
int gm82_sound_play(gm82_sound_runtime *sr, int sound_index, int loop);
int gm82_sound_play_by_name(gm82_sound_runtime *sr, const char *name, int loop);
void gm82_sound_stop_all(gm82_sound_runtime *sr);

/* GML-style (uses bound global) */
void gm82_sound_set_global(gm82_sound_runtime *sr);
double gml_sound_play(double sound_index);
double gml_sound_loop(double sound_index);
double gml_sound_stop_all(void);
double gml_sound_is_playing(double sound_index); /* always 0 without device */
double gml_sound_exists(double sound_index);
double gml_sound_volume(double sound_index, double volume);
double gml_sound_get_name_count(void);

#ifdef __cplusplus
}
#endif

#endif
