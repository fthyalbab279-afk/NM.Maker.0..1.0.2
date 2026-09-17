#ifndef GM82_FONT_DECODE_H
#define GM82_FONT_DECODE_H
#include <stddef.h>
#include <stdint.h>
#ifdef __cplusplus
extern "C" {
#endif
typedef struct {
    char name[64];
    char font_name[64];
    int32_t size;
    int32_t bold, italic;
} gm82_decoded_font;
typedef struct {
    gm82_decoded_font *items;
    int count;
} gm82_decoded_font_list;
void gm82_decoded_font_list_free(gm82_decoded_font_list *L);
/* Headers only – no glyph atlas yet */
int gm82_decode_fonts_from_gmk(const uint8_t *data, size_t size, gm82_decoded_font_list *out);
#ifdef __cplusplus
}
#endif
#endif
