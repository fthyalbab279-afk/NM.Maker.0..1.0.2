#ifndef GM82_SPRITE_DECODE_H
#define GM82_SPRITE_DECODE_H

#include <stddef.h>
#include <stdint.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef struct {
    char     name[64];
    int32_t  width;
    int32_t  height;
    uint8_t *rgba;       /* width*height*4, caller frees via list_free */
    size_t   rgba_size;
} gm82_decoded_frame;

typedef struct {
    gm82_decoded_frame *frames;
    int                 count;
    int                 capacity;
} gm82_decoded_sprite_list;

/* Group consecutive frames that share the same resource name */
typedef struct {
    char    name[64];
    int32_t frame_start; /* index into frames[] */
    int32_t frame_count;
    int32_t width, height;
} gm82_sprite_group;

typedef struct {
    gm82_sprite_group *items;
    int count;
} gm82_sprite_group_list;

void gm82_sprite_groups_build(const gm82_decoded_sprite_list *frames, gm82_sprite_group_list *out);
void gm82_sprite_group_list_free(gm82_sprite_group_list *G);
/* Map a flat frame index to group index; -1 if invalid */
int gm82_sprite_group_index_for_frame(const gm82_sprite_group_list *G, int frame_index);
/* Resolve image_index within group from a frame-based sprite_index */
int gm82_sprite_resolve_frame(const gm82_sprite_group_list *G, int sprite_index, int image_index);

void gm82_decoded_sprite_list_init(gm82_decoded_sprite_list *L);
void gm82_decoded_sprite_list_free(gm82_decoded_sprite_list *L);

/* Returns number of frames decoded, or negative on error. */
int gm82_decode_sprites_from_gmk(const uint8_t *data, size_t size,
                                 gm82_decoded_sprite_list *out);

int gm82_decode_sprites_from_file(const char *path, gm82_decoded_sprite_list *out);

#ifdef __cplusplus
}
#endif

#endif
