#include "gm82_runtime_guard.h"
#include "gm82_materialize.h"
#include "gm82_project_ir.h"
#include <stdio.h>
#include <string.h>

/*
 * This is the single entry point that the Android JNI / WebView bridge
 * must call before any rendering or instance creation.
 *
 * Pseudocode for MainActivity / native layer:
 *
 *   gm82_project_ir *ir = gm82_gmk_load_and_parse(path);   // existing reader
 *   if (!gm82_game_try_start(ir, err, sizeof err)) {
 *       show_error_ui(err);   // instead of black screen
 *       return;
 *   }
 *   // only then: create GL surface, enter game loop, room_goto(first)
 */

bool gm82_game_try_start(gm82_project_ir *ir, char *errbuf, size_t errbuf_size) {
    if (!ir) {
        if (errbuf && errbuf_size) snprintf(errbuf, errbuf_size, "null project IR");
        return false;
    }

    /* 1. Force materialization pass */
    gm82_materialize_options opts = gm82_materialize_default_options();
    opts.fail_on_partial = false;   /* collect all failures */
    opts.upload_to_gpu = false;     /* GPU upload only after we have a context */

    gm82_materialize_all(ir, &opts);

    /* 2. Hard guard – never continue if incomplete */
    if (!gm82_runtime_require_playable(ir, errbuf, errbuf_size)) {
        return false;
    }

    /* 3. Optional second pass once a real GL context exists:
     *    opts.upload_to_gpu = true;
     *    gm82_materialize_sprites(ir, &opts);
     *    gm82_materialize_backgrounds(ir, &opts);
     */

    return true;
}
