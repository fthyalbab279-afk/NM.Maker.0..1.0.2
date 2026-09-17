#ifndef GM82_TIMELINE_H
#define GM82_TIMELINE_H

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

#define GM82_TIMELINE_MAX_MOMENTS 64
#define GM82_TIMELINE_MAX 16

typedef struct {
    int32_t step;           /* moment position */
    char    code[128];      /* simple GML line or empty */
} gm82_timeline_moment;

typedef struct {
    char name[64];
    gm82_timeline_moment moments[GM82_TIMELINE_MAX_MOMENTS];
    int count;
} gm82_timeline;

typedef struct {
    gm82_timeline items[GM82_TIMELINE_MAX];
    int count;
} gm82_timeline_list;

void gm82_timeline_list_init(gm82_timeline_list *L);
int  gm82_timeline_add(gm82_timeline_list *L, const char *name);
int  gm82_timeline_add_moment(gm82_timeline_list *L, int tl_index, int32_t step, const char *code);

/* Instance timeline state is path-like: timeline_index + timeline_position */

#ifdef __cplusplus
}
#endif

#endif
