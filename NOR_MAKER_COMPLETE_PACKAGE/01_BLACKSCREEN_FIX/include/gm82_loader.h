#ifndef GM82_LOADER_H
#define GM82_LOADER_H

#include "gm82_project_ir.h"
#include <stddef.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

/*
 * Unified entry for both formats while keeping the existing design:
 *   load → IR (usually incomplete) → materialize → guard → play
 */

typedef enum gm82_source_kind {
    GM82_SOURCE_UNKNOWN = 0,
    GM82_SOURCE_GMK     = 1,   /* binary .gmk / .gm81 */
    GM82_SOURCE_GM82    = 2    /* text .gm82 project */
} gm82_source_kind;

typedef struct gm82_load_result {
    gm82_project_ir *ir;
    gm82_source_kind kind;
    bool             ok;
    char             error[256];
} gm82_load_result;

/* Auto-detect by extension / magic and load into IR v5. */
gm82_load_result gm82_load_auto(const char *path);

/* Convenience: load + materialize + guard in one call.
 * Returns true only when the game is actually playable. */
bool gm82_load_and_prepare(const char *path, gm82_project_ir **out_ir, char *errbuf, size_t errbuf_size);

#ifdef __cplusplus
}
#endif

#endif /* GM82_LOADER_H */
