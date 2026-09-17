#ifndef GM82_OBJECT_ROOM_DECODE_H
#define GM82_OBJECT_ROOM_DECODE_H

#include <stddef.h>
#include <stdint.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef struct {
    char    name[64];
    int32_t sprite_index;   /* -1 if none */
    int32_t solid;
    int32_t visible;
    int32_t depth;
    int32_t persistent;
} gm82_decoded_object;

typedef struct {
    gm82_decoded_object *items;
    int count;
} gm82_decoded_object_list;

typedef struct {
    int32_t x, y;
    int32_t object_index;
    int32_t id;
} gm82_decoded_instance;

typedef struct {
    int32_t x, y;
    int32_t background_index;
    int32_t xo, yo;
    int32_t width, height;
    int32_t depth;
    int32_t id;
} gm82_decoded_tile;

typedef struct {
    char    name[64];
    int32_t width, height;
    int32_t speed;
    gm82_decoded_instance *instances;
    int instance_count;
    gm82_decoded_tile *tiles;
    int tile_count;
} gm82_decoded_room;

typedef struct {
    gm82_decoded_room *items;
    int count;
} gm82_decoded_room_list;

void gm82_decoded_object_list_free(gm82_decoded_object_list *L);
void gm82_decoded_room_list_free(gm82_decoded_room_list *L);

int gm82_decode_objects_from_gmk(const uint8_t *data, size_t size, gm82_decoded_object_list *out);
int gm82_decode_rooms_from_gmk(const uint8_t *data, size_t size, gm82_decoded_room_list *out);

#ifdef __cplusplus
}
#endif

#endif
