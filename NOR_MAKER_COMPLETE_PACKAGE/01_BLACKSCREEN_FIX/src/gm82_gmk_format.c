#include "gm82_gmk_format.h"
#include <string.h>

gm82_gmk_probe_result gm82_gmk_probe(const uint8_t *data, size_t size) {
    gm82_gmk_probe_result r;
    memset(&r, 0, sizeof(r));
    r.status = GM82_GMK_PARSE_INVALID;
    r.error_code = "buffer_too_small";

    if (!data || size < 12) return r;

    int32_t magic = (int32_t)(data[0] | (data[1] << 8) | (data[2] << 16) | (data[3] << 24));
    int32_t version = (int32_t)(data[4] | (data[5] << 8) | (data[6] << 16) | (data[7] << 24));
    int32_t app_id = (int32_t)(data[8] | (data[9] << 8) | (data[10] << 16) | (data[11] << 24));

    r.magic = magic;
    r.version = version;
    r.app_id = app_id;
    r.header_bytes = 12;

    if (magic != 1234321) {
        r.error_code = "bad_magic";
        return r;
    }
    if (version < 500 || version > 810) {
        r.error_code = "unsupported_version";
        return r;
    }

    r.format_kind = (version >= 810) ? GM82_GMK_FORMAT_GM81 : GM82_GMK_FORMAT_GM7_GM8;
    r.status = GM82_GMK_PARSE_PARTIAL;   /* header ok, full decode still required */
    r.error_code = NULL;
    return r;
}
