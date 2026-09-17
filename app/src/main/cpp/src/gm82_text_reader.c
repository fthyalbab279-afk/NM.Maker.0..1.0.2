#define _POSIX_C_SOURCE 200809L
#include "gm82_text_reader.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <dirent.h>

/*
 * Minimal .gm82 text project loader.
 * Official GM82 projects are a directory tree with text files
 * (one of the main reasons the format was created – git friendly).
 *
 * This implementation only creates a valid (incomplete) Project IR
 * so the same materialize + guard pipeline can be used.
 * Full parsing of every resource file is left for later iterations.
 */

static int is_dir(const char *path) {
    struct stat st;
    if (stat(path, &st) != 0) return 0;
    return S_ISDIR(st.st_mode);
}

gm82_text_load_result gm82_text_load_project(const char *path) {
    gm82_text_load_result r;
    memset(&r, 0, sizeof(r));

    if (!path) {
        snprintf(r.error, sizeof(r.error), "null path");
        return r;
    }

    gm82_project_ir *ir = gm82_project_ir_create();
    if (!ir) {
        snprintf(r.error, sizeof(r.error), "out of memory");
        return r;
    }

    ir->format  = GM82_GMK_FORMAT_GM82;
    ir->version = 82;          /* marker for text format */
    ir->complete = false;

    /* Very small heuristic: if path is a directory, look for common
       GM82 resource folders (sprites, backgrounds, rooms, objects). */
    if (is_dir(path)) {
        /* Create one placeholder room so the IR is non-empty.
           Real resource enumeration will walk the folders later. */
        ir->room_count = 1;
        ir->rooms = (gm82_res_room *)calloc(1, sizeof(gm82_res_room));
        if (ir->rooms) {
            ir->rooms[0].id = 0;
            ir->rooms[0].name = strdup("room0");
            ir->rooms[0].width = 640;
            ir->rooms[0].height = 480;
            ir->rooms[0].speed = 30;
            ir->rooms[0].status = GM82_RES_PARTIAL;
        }
        snprintf(r.error, sizeof(r.error),
                 "GM82 directory project loaded (partial) – full text parse not yet implemented");
    } else {
        /* Single file – treat as index / main project file.
           Content parsing is still TODO. */
        ir->room_count = 1;
        ir->rooms = (gm82_res_room *)calloc(1, sizeof(gm82_res_room));
        if (ir->rooms) {
            ir->rooms[0].id = 0;
            ir->rooms[0].name = strdup("room0");
            ir->rooms[0].width = 640;
            ir->rooms[0].height = 480;
            ir->rooms[0].speed = 30;
            ir->rooms[0].status = GM82_RES_PARTIAL;
        }
        snprintf(r.error, sizeof(r.error),
                 "GM82 text file loaded (partial) – full text parse not yet implemented");
    }

    gm82_project_ir_recompute_complete(ir);
    r.ir = ir;
    r.ok = true;   /* load succeeded, but still incomplete */
    return r;
}
