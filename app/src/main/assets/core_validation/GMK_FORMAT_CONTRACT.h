#ifndef GM82_GMK_FORMAT_H
#define GM82_GMK_FORMAT_H

#include <stddef.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef enum gm82_gmk_format_kind {
    GM82_GMK_FORMAT_UNKNOWN = 0,
    GM82_GMK_FORMAT_GM7_GM8 = 1,
    GM82_GMK_FORMAT_GM81 = 2
} gm82_gmk_format_kind;

typedef enum gm82_gmk_parse_status {
    GM82_GMK_PARSE_INVALID = 0,
    GM82_GMK_PARSE_PARTIAL = 1,
    GM82_GMK_PARSE_DECODED = 2
} gm82_gmk_parse_status;

typedef struct gm82_gmk_probe_result {
    gm82_gmk_format_kind format_kind;
    gm82_gmk_parse_status status;
    int32_t magic;
    int32_t version;
    int32_t app_id;
    size_t header_bytes;
    const char *error_code;
} gm82_gmk_probe_result;

/* Probe only the stable header; never reads beyond size and never allocates. */
gm82_gmk_probe_result gm82_gmk_probe(const uint8_t *data, size_t size);

#ifdef __cplusplus
}
#endif

#endif
