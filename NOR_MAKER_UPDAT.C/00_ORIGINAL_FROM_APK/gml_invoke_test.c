#include "gml_frontend.h"
#include "gml_vm.h"
#include <assert.h>
#include <stdio.h>

int main(void) {
    gml_ast *root = NULL; char error[160] = {0};
    assert(gml_parse_program("return arg0 * 2 + 1;", &root, error, sizeof error));
    gml_vm vm; gml_vm_init(&vm);
    gml_value arg = gml_value_real(20.0), out = {0};
    assert(gml_vm_invoke(&vm, root, &arg, 1, &out));
    assert(out.kind == GML_V_REAL && out.real == 41.0);
    assert(gml_vm_get(&vm, "arg0").kind == GML_V_UNDEFINED);
    gml_value_free(&arg); gml_value_free(&out); gml_ast_free(root);
    puts("GML_INVOKE_SCOPE_TEST_PASS");
    return 0;
}
