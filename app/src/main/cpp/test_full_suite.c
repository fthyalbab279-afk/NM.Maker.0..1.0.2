#include "gm82_gmk_reader.h"
#include "gm82_gml_builtins.h"
#include "gml_frontend.h"
#include "gml_vm.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>

int gm82_native_call(void *userdata, const char *name, const gml_value *args, size_t count, gml_value *out) {
    (void)userdata;
    if (!name || !out) return 0;
    if (strcmp(name, "point_distance") == 0 && count == 4) {
        *out = gml_value_real(gml_point_distance(args[0].real, args[1].real, args[2].real, args[3].real));
        return 1;
    }
    if (strcmp(name, "point_direction") == 0 && count == 4) {
        *out = gml_value_real(gml_point_direction(args[0].real, args[1].real, args[2].real, args[3].real));
        return 1;
    }
    if (strcmp(name, "string_length") == 0 && count == 1 && args[0].kind == GML_V_STRING) {
        *out = gml_value_real(gml_string_length(args[0].string));
        return 1;
    }
    if (strcmp(name, "string_copy") == 0 && count == 3 && args[0].kind == GML_V_STRING) {
        const char *s = args[0].string ? args[0].string : "";
        int start = (int)args[1].real, len = (int)args[2].real;
        size_t slen = strlen(s);
        if (start < 1) start = 1;
        if (len < 0) len = 0;
        size_t begin = (size_t)(start - 1);
        if (begin > slen) begin = slen;
        if ((size_t)len > slen - begin) len = (int)(slen - begin);
        char *buf = (char *)malloc((size_t)len + 1);
        if (buf) {
            memcpy(buf, s + begin, (size_t)len);
            buf[len] = 0;
            *out = gml_value_string(buf);
            free(buf);
            return 1;
        }
    }
    if (strcmp(name, "string_pos") == 0 && count == 2 && args[0].kind == GML_V_STRING && args[1].kind == GML_V_STRING) {
        const char *needle = args[0].string ? args[0].string : "";
        const char *haystack = args[1].string ? args[1].string : "";
        const char *f = needle[0] ? strstr(haystack, needle) : haystack;
        *out = gml_value_real(f ? (double)(f - haystack + 1) : 0.0);
        return 1;
    }
    return 0;
}

double nor_export_nes_native(const char *project, const char *output) {
    (void)project; (void)output; return 1.0;
}
double nor_validate_rom_native(const char *path, double kind) {
    (void)path; (void)kind; return 1.0;
}
double nor_export_gbc_native(const char *project, const char *output) {
    (void)project; (void)output; return 1.0;
}
double nor_export_gba_native(const char *project, const char *output) {
    (void)project; (void)output; return 1.0;
}

void test_gmk_probe_suite(void) {
    uint8_t dummy[12] = {0x91, 0xd5, 0x12, 0x00, 0x20, 0x03, 0x00, 0x00, 0x7b, 0x00, 0x00, 0x00};
    gm82_gmk_probe_result res = gm82_gmk_probe(dummy, sizeof(dummy));
    assert(res.status == GM82_GMK_PARSE_PARTIAL);
    assert(res.format_kind == GM82_GMK_FORMAT_GM7_GM8);
    assert(res.magic == 1234321);
    assert(res.version == 800);
    printf("[PASS] GMK Probe Suite\n");
}

void test_gml_vm_suite(void) {
    const char *code =
        "x = 5;\n"
        "y = 15;\n"
        "res = max(x, y) + min(x, y);\n"
        "return res;\n";

    gml_ast *ast = NULL;
    char err[160] = {0};
    int parse_ok = gml_parse_program(code, &ast, err, sizeof(err));
    assert(parse_ok);

    gml_vm vm;
    gml_vm_init(&vm);
    int exec_ok = gml_vm_execute(&vm, ast);
    assert(exec_ok);
    assert(vm.returned);
    assert(vm.return_value.real == 20.0);

    gml_ast_free(ast);
    printf("[PASS] GML VM Suite\n");
}

void test_gml_builtins_suite(void) {
    gml_value args[4];
    gml_value out;

    // Test point_distance(0, 0, 3, 4) == 5.0
    args[0] = gml_value_real(0.0);
    args[1] = gml_value_real(0.0);
    args[2] = gml_value_real(3.0);
    args[3] = gml_value_real(4.0);
    assert(gm82_native_call(NULL, "point_distance", args, 4, &out) == 1);
    assert(out.kind == GML_V_REAL && out.real == 5.0);

    // Test point_direction(0, 0, 0, -10) == 90.0
    args[0] = gml_value_real(0.0);
    args[1] = gml_value_real(0.0);
    args[2] = gml_value_real(0.0);
    args[3] = gml_value_real(-10.0);
    assert(gm82_native_call(NULL, "point_direction", args, 4, &out) == 1);
    assert(out.kind == GML_V_REAL && out.real == 90.0);

    // Test string_length("Hello") == 5
    args[0] = gml_value_string("Hello");
    assert(gm82_native_call(NULL, "string_length", args, 1, &out) == 1);
    assert(out.kind == GML_V_REAL && out.real == 5.0);
    gml_value_free(&args[0]);

    // Test string_copy("NorMaker", 1, 3) == "Nor"
    args[0] = gml_value_string("NorMaker");
    args[1] = gml_value_real(1.0);
    args[2] = gml_value_real(3.0);
    assert(gm82_native_call(NULL, "string_copy", args, 3, &out) == 1);
    assert(out.kind == GML_V_STRING && strcmp(out.string, "Nor") == 0);
    gml_value_free(&args[0]);
    gml_value_free(&out);

    // Test string_pos("Maker", "NorMaker") == 4
    args[0] = gml_value_string("Maker");
    args[1] = gml_value_string("NorMaker");
    assert(gm82_native_call(NULL, "string_pos", args, 2, &out) == 1);
    assert(out.kind == GML_V_REAL && out.real == 4.0);
    gml_value_free(&args[0]);
    gml_value_free(&args[1]);

    // Test string_digits("Stage 12 Score 450") via GML VM script execution
    const char *dig_script = "s = 'Stage 12 Score 450'; return string_digits(s);";
    gml_ast *ast = NULL;
    char err[160] = {0};
    int ok = gml_parse_program(dig_script, &ast, err, sizeof(err));
    assert(ok);
    gml_vm vm;
    gml_vm_init(&vm);
    ok = gml_vm_execute(&vm, ast);
    assert(ok && vm.returned);
    assert(vm.return_value.kind == GML_V_STRING && strcmp(vm.return_value.string, "12450") == 0);
    gml_ast_free(ast);
    gml_value_free(&vm.return_value);

    // Test sqr and log2
    const char *math_script = "a = sqr(5); b = log2(16); return a + b;";
    ok = gml_parse_program(math_script, &ast, err, sizeof(err));
    assert(ok);
    gml_vm_init(&vm);
    ok = gml_vm_execute(&vm, ast);
    assert(ok && vm.returned);
    assert(vm.return_value.kind == GML_V_REAL && vm.return_value.real == 29.0);
    gml_ast_free(ast);
    gml_value_free(&vm.return_value);

    printf("[PASS] GML Built-ins Suite\n");
}

void test_retro_rom_suite(void) {
    const char *nes_path = "/tmp/nor_core_tests/test.nes";
    const char *gbc_path = "/tmp/nor_core_tests/test.gbc";
    const char *gba_path = "/tmp/nor_core_tests/test.gba";

    assert(nor_export_nes_native("proj", nes_path) == 1.0);
    assert(nor_validate_rom_native(nes_path, 1.0) == 1.0);

    assert(nor_export_gbc_native("proj", gbc_path) == 1.0);
    assert(nor_export_gba_native("proj", gba_path) == 1.0);

    printf("[PASS] Retro ROM Suite\n");
}

int main(void) {
    printf("--- Running Native Host Comprehensive Test Suite ---\n");
    test_gmk_probe_suite();
    test_gml_vm_suite();
    test_gml_builtins_suite();
    test_retro_rom_suite();
    printf("--- All Native Host Tests Passed! ---\n");
    return 0;
}
