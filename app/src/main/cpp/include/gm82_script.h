#ifndef GM82_SCRIPT_H
#define GM82_SCRIPT_H

#include <stdint.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

#define GM82_SCRIPT_MAX 64
#define GM82_SCRIPT_CODE_MAX 512

typedef struct {
    char name[64];
    char code[GM82_SCRIPT_CODE_MAX];
} gm82_script;

typedef struct {
    gm82_script items[GM82_SCRIPT_MAX];
    int count;
} gm82_script_list;

void gm82_script_list_init(gm82_script_list *L);
int  gm82_script_add(gm82_script_list *L, const char *name, const char *code);
int  gm82_script_find(const gm82_script_list *L, const char *name);

/* Best-effort GMK decode: resources named scr_* / script* */
int gm82_decode_scripts_from_gmk(const uint8_t *data, size_t size, gm82_script_list *out);

/* Best-effort: collect length-prefixed strings that look like GML event code */
#define GM82_GML_FRAG_MAX 128
#define GM82_GML_FRAG_CODE 1024
typedef struct {
    char code[GM82_GML_FRAG_CODE];
    int32_t length;
} gm82_gml_fragment;

typedef struct {
    gm82_gml_fragment items[GM82_GML_FRAG_MAX];
    int count;
} gm82_gml_fragment_list;

void gm82_gml_fragment_list_init(gm82_gml_fragment_list *L);
int gm82_harvest_gml_fragments_from_gmk(const uint8_t *data, size_t size, gm82_gml_fragment_list *out);

#ifdef __cplusplus
}
#endif

#endif
