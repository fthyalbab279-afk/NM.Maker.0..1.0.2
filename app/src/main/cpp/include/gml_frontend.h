#ifndef GML_FRONTEND_H
#define GML_FRONTEND_H

#include "gml_vm.h"
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

int gml_parse_program(const char *source, gml_ast **out_ast, char *err_buf, size_t err_buf_size);
void gml_ast_free(gml_ast *ast);

#ifdef __cplusplus
}
#endif

#endif
