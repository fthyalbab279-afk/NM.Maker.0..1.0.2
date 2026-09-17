#ifndef GM82_SOUND_DECODE_H
#define GM82_SOUND_DECODE_H
#include <stddef.h>
#include <stdint.h>
#ifdef __cplusplus
extern "C" {
#endif
typedef struct {
    char name[64];
    int32_t kind;      /* 0=normal 1=background 3=3d etc */
    int32_t effects;
    double volume;
    int32_t preload;
    size_t data_offset; /* in source file if known */
    size_t data_size;
} gm82_decoded_sound;
typedef struct {
    gm82_decoded_sound *items;
    int count;
} gm82_decoded_sound_list;
void gm82_decoded_sound_list_free(gm82_decoded_sound_list *L);
int gm82_decode_sounds_from_gmk(const uint8_t *data, size_t size, gm82_decoded_sound_list *out);
#ifdef __cplusplus
}
#endif
#endif
