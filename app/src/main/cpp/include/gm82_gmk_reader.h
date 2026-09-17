#ifndef GM82_GMK_READER_H
#define GM82_GMK_READER_H

#include "gm82_project_ir.h"
#include <stddef.h>
#include <stdint.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

/*
 * GMK 800 / GM82-compatible binary reader.
 * Design stays the same: produces a gm82_project_ir that starts incomplete.
 * Only after materialize_* succeeds does complete become true.
 *
 * This is NOT a full reimplementation of the Windows runner.
 * It only fills the IR fields we need to stop the black screen and
 * eventually draw real sprites/backgrounds.
 */

typedef struct gm82_gmk_load_result {
    gm82_project_ir *ir;          /* owned by caller, free with gm82_project_ir_free */
    bool             ok;
    char             error[256];
} gm82_gmk_load_result;

/* Load a classic .gmk (magic 1234321, version 800) into Project IR v5.
 * Resources start as PARTIAL / RAW. Call gm82_materialize_all afterwards. */
gm82_gmk_load_result gm82_gmk_load_from_memory(const uint8_t *data, size_t size);

gm82_gmk_load_result gm82_gmk_load_from_file(const char *path);

#ifdef __cplusplus
}
#endif

#endif /* GM82_GMK_READER_H */
