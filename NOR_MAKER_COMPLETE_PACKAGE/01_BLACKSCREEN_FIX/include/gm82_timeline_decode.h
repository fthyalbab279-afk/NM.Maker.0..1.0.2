#ifndef GM82_TIMELINE_DECODE_H
#define GM82_TIMELINE_DECODE_H
#include "gm82_timeline.h"
#include <stddef.h>
#include <stdint.h>
#ifdef __cplusplus
extern "C" {
#endif
int gm82_decode_timelines_from_gmk(const uint8_t *data, size_t size, gm82_timeline_list *out);
#ifdef __cplusplus
}
#endif
#endif
