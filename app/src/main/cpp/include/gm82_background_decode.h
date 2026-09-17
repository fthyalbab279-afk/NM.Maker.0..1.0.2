#ifndef GM82_BACKGROUND_DECODE_H
#define GM82_BACKGROUND_DECODE_H

#include <stddef.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef struct {
    char     name[64];
    int32_t  width;
    int32_t  height;
    uint8_t *rgba;
    size_t   rgba_size;
} gm82_decoded_background;

typedef struct {
    gm82_decoded_background *items;
    int count;
    int capacity;
} gm82_decoded_background_list;

void gm82_decoded_background_list_init(gm82_decoded_background_list *L);
void gm82_decoded_background_list_free(gm82_decoded_background_list *L);

int gm82_decode_backgrounds_from_gmk(const uint8_t *data, size_t size,
                                     gm82_decoded_background_list *out);
int gm82_decode_backgrounds_from_file(const char *path,
                                      gm82_decoded_background_list *out);

#ifdef __cplusplus
}
#endif

#endif
