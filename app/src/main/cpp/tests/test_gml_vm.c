#include "gml_vm.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>

static void test_vm_values(void) {
    gml_value r = gml_value_real(42.5);
    assert(r.kind == GML_V_REAL);
    assert(r.real == 42.5);

    gml_value b = gml_value_bool(1);
    assert(b.kind == GML_V_BOOL);
    assert(b.boolean == 1);

    gml_value s = gml_value_string("NOR_MAKER_GM82");
    assert(s.kind == GML_V_STRING);
    assert(strcmp(s.string, "NOR_MAKER_GM82") == 0);
    gml_value_free(&s);

    gml_value arr = gml_value_array(5);
    assert(arr.kind == GML_V_ARRAY);
    assert(arr.array != NULL);
    assert(arr.array->count == 5);
    gml_value_free(&arr);

    puts("test_vm_values PASS");
}

static void test_vm_scopes(void) {
    gml_vm vm;
    gml_vm_init(&vm);

    gml_value v1 = gml_value_real(100.0);
    gml_vm_set(&vm, "score", v1);
    gml_value_free(&v1);

    gml_value fetched = gml_vm_get(&vm, "score");
    assert(fetched.kind == GML_V_REAL);
    assert(fetched.real == 100.0);

    gml_vm_push_scope(&vm);
    gml_value v2 = gml_value_real(200.0);
    gml_vm_set(&vm, "temp_val", v2);
    gml_value_free(&v2);

    gml_value fetched_local = gml_vm_get(&vm, "temp_val");
    assert(fetched_local.kind == GML_V_REAL);
    assert(fetched_local.real == 200.0);

    gml_vm_pop_scope(&vm);

    gml_value popped = gml_vm_get(&vm, "temp_val");
    assert(popped.kind == GML_V_UNDEFINED);

    puts("test_vm_scopes PASS");
}

static void test_ast_math_exec(void) {
    gml_vm vm;
    gml_vm_init(&vm);

    /* Construct AST for: return sqr(5) + sqrt(16); */
    gml_ast c1; memset(&c1, 0, sizeof(c1));
    c1.kind = GML_AST_NUMBER;
    c1.number = 5.0;

    gml_ast call1; memset(&call1, 0, sizeof(call1));
    call1.kind = GML_AST_CALL;
    call1.text = "sqr";
    call1.count = 1;
    gml_ast *items1[1] = { &c1 };
    call1.items = items1;

    gml_ast c2; memset(&c2, 0, sizeof(c2));
    c2.kind = GML_AST_NUMBER;
    c2.number = 16.0;

    gml_ast call2; memset(&call2, 0, sizeof(call2));
    call2.kind = GML_AST_CALL;
    call2.text = "sqrt";
    call2.count = 1;
    gml_ast *items2[1] = { &c2 };
    call2.items = items2;

    gml_ast add_op; memset(&add_op, 0, sizeof(add_op));
    add_op.kind = GML_AST_BINARY;
    add_op.op = GML_T_PLUS;
    add_op.left = &call1;
    add_op.right = &call2;

    gml_ast ret_stmt; memset(&ret_stmt, 0, sizeof(ret_stmt));
    ret_stmt.kind = GML_AST_RETURN;
    ret_stmt.left = &add_op;

    int ok = gml_vm_execute(&vm, &ret_stmt);
    assert(ok);
    assert(vm.returned);
    assert(vm.return_value.kind == GML_V_REAL);
    assert(vm.return_value.real == 29.0); /* 25 + 4 = 29 */

    gml_value_free(&vm.return_value);
    puts("test_ast_math_exec PASS");
}

int main(void) {
    puts("--- Running GML VM Unit Suite ---");
    test_vm_values();
    test_vm_scopes();
    test_ast_math_exec();
    puts("--- GML VM Unit Suite PASS ---");
    return 0;
}
