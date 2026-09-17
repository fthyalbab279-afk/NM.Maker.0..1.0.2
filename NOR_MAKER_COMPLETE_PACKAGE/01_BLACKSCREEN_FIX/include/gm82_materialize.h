#ifndef GM82_MATERIALIZE_H
#define GM82_MATERIALIZE_H

#include "gm82_project_ir.h"
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

/*
 * Materialization is the critical missing piece that causes the black screen.
 *
 * Flow:
 *   1. Parse GMK → Project IR (many resources stay PARTIAL / RAW)
 *   2. Call gm82_materialize_all() → decode payloads + upload textures
 *   3. Only if IR becomes complete → allow game loop to start
 */

typedef struct gm82_materialize_options {
    const char *cache_dir;          /* where to write .payload files (can be NULL) */
    bool        upload_to_gpu;      /* create GL textures (needs valid GL context) */
    bool        fail_on_partial;    /* if true, return false as soon as any resource fails */
    int         max_texture_size;   /* 0 = use driver limit */
} gm82_materialize_options;

/* Default options: no GPU upload, do not fail early, no cache. */
gm82_materialize_options gm82_materialize_default_options(void);

/*
 * Attempt to fully decode and (optionally) upload every resource.
 * Updates status fields inside the IR.
 * Returns true only if after the pass the IR is complete.
 */
bool gm82_materialize_all(gm82_project_ir *ir, const gm82_materialize_options *opts);

/* Individual helpers (useful for progressive loading / debug). */
bool gm82_materialize_sprites(gm82_project_ir *ir, const gm82_materialize_options *opts);
bool gm82_materialize_backgrounds(gm82_project_ir *ir, const gm82_materialize_options *opts);
bool gm82_materialize_rooms(gm82_project_ir *ir, const gm82_materialize_options *opts);

#ifdef __cplusplus
}
#endif

#endif /* GM82_MATERIALIZE_H */
