#define _POSIX_C_SOURCE 200809L
#include "gm82_loader.h"
#include "gm82_gmk_reader.h"
#include "gm82_text_reader.h"
#include "gm82_materialize.h"
#include "gm82_runtime_guard.h"
#include <stdio.h>
#include <string.h>
#include <strings.h>

static int ends_with(const char *s, const char *suf) {
    size_t ls = strlen(s), lf = strlen(suf);
    if (ls < lf) return 0;
    return strcasecmp(s + ls - lf, suf) == 0;
}

gm82_load_result gm82_load_auto(const char *path) {
    gm82_load_result r;
    memset(&r, 0, sizeof(r));
    if (!path) {
        snprintf(r.error, sizeof(r.error), "null path");
        return r;
    }

    if (ends_with(path, ".gmk") || ends_with(path, ".gm81") || ends_with(path, ".gm6")) {
        gm82_gmk_load_result gr = gm82_gmk_load_from_file(path);
        r.ir   = gr.ir;
        r.kind = GM82_SOURCE_GMK;
        r.ok   = gr.ok;
        snprintf(r.error, sizeof(r.error), "%s", gr.error);
        return r;
    }

    if (ends_with(path, ".gm82") || ends_with(path, ".gm82project")) {
        gm82_text_load_result tr = gm82_text_load_project(path);
        r.ir   = tr.ir;
        r.kind = GM82_SOURCE_GM82;
        r.ok   = tr.ok;
        snprintf(r.error, sizeof(r.error), "%s", tr.error);
        return r;
    }

    /* Try directory as GM82 project */
    gm82_text_load_result tr = gm82_text_load_project(path);
    if (tr.ok) {
        r.ir   = tr.ir;
        r.kind = GM82_SOURCE_GM82;
        r.ok   = true;
        snprintf(r.error, sizeof(r.error), "%s", tr.error);
        return r;
    }

    /* Last resort: try as binary GMK */
    gm82_gmk_load_result gr = gm82_gmk_load_from_file(path);
    r.ir   = gr.ir;
    r.kind = GM82_SOURCE_GMK;
    r.ok   = gr.ok;
    snprintf(r.error, sizeof(r.error), "%s", gr.error);
    return r;
}

bool gm82_load_and_prepare(const char *path, gm82_project_ir **out_ir, char *errbuf, size_t errbuf_size) {
    if (out_ir) *out_ir = NULL;
    if (!path) {
        if (errbuf && errbuf_size) snprintf(errbuf, errbuf_size, "null path");
        return false;
    }

    gm82_load_result lr = gm82_load_auto(path);
    if (!lr.ok || !lr.ir) {
        if (errbuf && errbuf_size) snprintf(errbuf, errbuf_size, "%s", lr.error);
        if (lr.ir) gm82_project_ir_free(lr.ir);
        return false;
    }

    /* Materialize (currently keeps most resources PARTIAL – correct) */
    gm82_materialize_options opts = gm82_materialize_default_options();
    gm82_materialize_all(lr.ir, &opts);

    /* Hard guard – never claim playable until complete */
    if (!gm82_runtime_require_playable(lr.ir, errbuf, errbuf_size)) {
        /* Keep the IR so the caller can inspect partial_count / error_message */
        if (out_ir) *out_ir = lr.ir;
        else gm82_project_ir_free(lr.ir);
        return false;
    }

    if (out_ir) *out_ir = lr.ir;
    else gm82_project_ir_free(lr.ir);
    return true;
}
