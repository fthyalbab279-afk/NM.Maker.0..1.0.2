#include "gm82_runtime_guard.h"
#include <stdio.h>
#include <string.h>

gm82_guard_result gm82_runtime_can_play(const gm82_project_ir *ir) {
    if (!ir) return GM82_GUARD_NULL_IR;
    if (!ir->complete) return GM82_GUARD_INCOMPLETE_IR;
    if (ir->room_count <= 0) return GM82_GUARD_NO_ROOMS;

    /* Extra safety: every sprite that is referenced must be decoded.
       (Full reference check is future work; for now we already enforce complete.) */
    for (int32_t i = 0; i < ir->sprite_count; i++) {
        if (ir->sprites[i].status != GM82_RES_DECODED)
            return GM82_GUARD_SPRITE_MISSING;
    }
    for (int32_t i = 0; i < ir->background_count; i++) {
        if (ir->backgrounds[i].status != GM82_RES_DECODED)
            return GM82_GUARD_BACKGROUND_MISSING;
    }
    return GM82_GUARD_OK;
}

const char *gm82_guard_message(gm82_guard_result r) {
    switch (r) {
        case GM82_GUARD_OK:                 return "OK";
        case GM82_GUARD_NULL_IR:            return "Project IR is null";
        case GM82_GUARD_INCOMPLETE_IR:      return "Resources incomplete – materialize failed (black screen prevented)";
        case GM82_GUARD_NO_ROOMS:           return "No rooms in project";
        case GM82_GUARD_SPRITE_MISSING:     return "One or more sprites not fully decoded";
        case GM82_GUARD_BACKGROUND_MISSING: return "One or more backgrounds not fully decoded";
        default:                            return "Unknown guard error";
    }
}

bool gm82_runtime_require_playable(const gm82_project_ir *ir, char *errbuf, size_t errbuf_size) {
    gm82_guard_result r = gm82_runtime_can_play(ir);
    if (r == GM82_GUARD_OK) return true;
    if (errbuf && errbuf_size > 0) {
        snprintf(errbuf, errbuf_size, "%s (partial_count=%d)",
                 gm82_guard_message(r),
                 ir ? ir->partial_count : -1);
    }
    return false;
}
