#include "gml_frontend.h"
#include "gml_vm.h"
#include <assert.h>
#include <stdio.h>

int main(void) {
    const char *src = "x = 0; do { x += 1; } until (x >= 3);";
    gml_ast *root = NULL; char error[160] = {0};
    assert(gml_parse_program(src, &root, error, sizeof error));
    gml_vm vm; gml_vm_init(&vm);
    assert(gml_vm_execute(&vm, root));
    gml_value x = gml_vm_get(&vm, "x");
    assert(x.kind == GML_V_REAL && x.real == 3.0);
    gml_value_free(&x); gml_ast_free(root);

    const char *src2 = "x = 0; do { x += 1; if (x < 2) { continue; } if (x >= 4) { break; } } until (x >= 10);";
    root = NULL; error[0] = 0;
    assert(gml_parse_program(src2, &root, error, sizeof error));
    gml_vm_init(&vm);
    assert(gml_vm_execute(&vm, root));
    x = gml_vm_get(&vm, "x");
    assert(x.kind == GML_V_REAL && x.real == 4.0);
    gml_value_free(&x); gml_ast_free(root);
    puts("GML_DO_UNTIL_TEST_PASS");
    return 0;
}
