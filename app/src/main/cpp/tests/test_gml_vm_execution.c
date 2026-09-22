#include "gml_frontend.h"
#include "gml_vm.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>

void test_ast_parse_and_execute(void) {
    const char *code =
        "a = 10;\n"
        "b = 20;\n"
        "c = a * b + 5;\n"
        "return c;\n";

    gml_ast *ast = NULL;
    char err[160] = {0};
    int ok = gml_parse_program(code, &ast, err, sizeof(err));
    assert(ok);
    assert(ast != NULL);

    gml_vm vm;
    gml_vm_init(&vm);
    ok = gml_vm_execute(&vm, ast);
    assert(ok);
    assert(vm.returned == 1);
    assert(vm.return_value.kind == GML_V_REAL);
    assert(vm.return_value.real == 205.0);

    gml_ast_free(ast);
    printf("  AST Parse & Execute PASS\n");
}

void test_script_invoke_args(void) {
    const char *code =
        "res = arg0 + arg1 * 2;\n"
        "return res;\n";

    gml_ast *ast = NULL;
    char err[160] = {0};
    int ok = gml_parse_program(code, &ast, err, sizeof(err));
    assert(ok);

    gml_vm vm;
    gml_vm_init(&vm);

    gml_value args[2];
    args[0] = gml_value_real(5.0);
    args[1] = gml_value_real(10.0);
    gml_value out;

    ok = gml_vm_invoke(&vm, ast, args, 2, &out);
    assert(ok);
    assert(out.kind == GML_V_REAL);
    assert(out.real == 25.0);

    gml_ast_free(ast);
    gml_value_free(&out);
    printf("  Script Invoke Args PASS\n");
}

int main(void) {
    printf("--- Running GML VM Execution Test Suite ---\n");
    test_ast_parse_and_execute();
    test_script_invoke_args();
    printf("--- GML VM Execution Tests Passed! ---\n");
    return 0;
}
