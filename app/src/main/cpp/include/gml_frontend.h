#ifndef GML_FRONTEND_H
#define GML_FRONTEND_H

#include "gml_vm.h"
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

/* Parse GML source code string into an AST root node.
   Returns 1 on success, 0 on parse error (populating errbuf). */
int gml_parse_program(const char *code, gml_ast **out_ast, char *errbuf, size_t errbuf_size);

/* Free AST created by gml_parse_program */
void gml_ast_free(gml_ast *ast);

#ifdef __cplusplus
}
#endif

#endif
