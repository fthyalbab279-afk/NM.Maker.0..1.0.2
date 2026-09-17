#ifndef GM82_PATH_DECODE_H
#define GM82_PATH_DECODE_H
#include "gm82_path.h"
#include <stddef.h>
#include <stdint.h>
#ifdef __cplusplus
extern "C" {
#endif
/* Best-effort: find zlib streams named path_* and parse point lists.
   Returns number of paths found (0 if game has none – not an error). */
int gm82_decode_paths_from_gmk(const uint8_t *data, size_t size, gm82_path_list *out);
#ifdef __cplusplus
}
#endif
#endif
