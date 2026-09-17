#ifndef GM82_GAME_START_H
#define GM82_GAME_START_H

#include "gm82_project_ir.h"
#include <stddef.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

/*
 * Single entry point that Android JNI / bridge must call
 * before any rendering or instance creation.
 *
 * Returns true  → IR is complete and safe to start the game loop.
 * Returns false → fill errbuf with a clear message and show it in the UI
 *                 instead of a black empty surface.
 */
bool gm82_game_try_start(gm82_project_ir *ir, char *errbuf, size_t errbuf_size);

#ifdef __cplusplus
}
#endif

#endif /* GM82_GAME_START_H */
