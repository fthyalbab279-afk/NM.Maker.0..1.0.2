#ifndef GM82_RUNTIME_GUARD_H
#define GM82_RUNTIME_GUARD_H

#include "gm82_project_ir.h"
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

/*
 * HARD RULE against black-screen / hallucination:
 *
 * Never start the game loop, never call draw_*, never create instances
 * unless the Project IR is fully playable.
 *
 * Call this before every room_goto / game_start / draw frame.
 */

typedef enum gm82_guard_result {
    GM82_GUARD_OK = 0,
    GM82_GUARD_INCOMPLETE_IR,
    GM82_GUARD_NULL_IR,
    GM82_GUARD_NO_ROOMS,
    GM82_GUARD_SPRITE_MISSING,
    GM82_GUARD_BACKGROUND_MISSING
} gm82_guard_result;

gm82_guard_result gm82_runtime_can_play(const gm82_project_ir *ir);

/* Human readable message for the UI error screen. */
const char *gm82_guard_message(gm82_guard_result r);

/*
 * Convenience: if not playable, fill a fixed-size error buffer and return false.
 * buffer must be at least 256 bytes.
 */
bool gm82_runtime_require_playable(const gm82_project_ir *ir, char *errbuf, size_t errbuf_size);

#ifdef __cplusplus
}
#endif

#endif /* GM82_RUNTIME_GUARD_H */
