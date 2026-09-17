#ifndef GM82_TEXT_READER_H
#define GM82_TEXT_READER_H

#include "gm82_project_ir.h"
#include <stddef.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

/*
 * Minimal .gm82 (text project) reader.
 * GM82 projects are directory-based / text-based (git-friendly).
 * This reader only handles the subset needed to build a Project IR
 * so the same materialize + guard path can be used.
 *
 * Full fidelity with the official GM82 IDE is NOT claimed.
 */

typedef struct gm82_text_load_result {
    gm82_project_ir *ir;
    bool             ok;
    char             error[256];
} gm82_text_load_result;

/* path = root of a .gm82 project directory or a single .gm82 index file */
gm82_text_load_result gm82_text_load_project(const char *path);

#ifdef __cplusplus
}
#endif

#endif /* GM82_TEXT_READER_H */
