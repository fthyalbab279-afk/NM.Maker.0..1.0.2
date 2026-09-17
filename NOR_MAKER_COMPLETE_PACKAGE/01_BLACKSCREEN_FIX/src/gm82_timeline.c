#define _POSIX_C_SOURCE 200809L
#include "gm82_timeline.h"
#include <string.h>

void gm82_timeline_list_init(gm82_timeline_list *L) {
    memset(L, 0, sizeof(*L));
}

int gm82_timeline_add(gm82_timeline_list *L, const char *name) {
    if (!L || L->count >= GM82_TIMELINE_MAX) return -1;
    gm82_timeline *t = &L->items[L->count];
    memset(t, 0, sizeof(*t));
    if (name) strncpy(t->name, name, sizeof(t->name)-1);
    return L->count++;
}

int gm82_timeline_add_moment(gm82_timeline_list *L, int tl_index, int32_t step, const char *code) {
    if (!L || tl_index < 0 || tl_index >= L->count) return 0;
    gm82_timeline *t = &L->items[tl_index];
    if (t->count >= GM82_TIMELINE_MAX_MOMENTS) return 0;
    t->moments[t->count].step = step;
    if (code) strncpy(t->moments[t->count].code, code, sizeof(t->moments[t->count].code)-1);
    t->count++;
    return 1;
}
