#!/usr/bin/env python3
import subprocess
import sys
import os

TESTS = [
    ("test_full_suite", "app/src/main/cpp/test_full_suite.c"),
    ("test_runtime_guard", "app/src/main/cpp/tests/test_runtime_guard.c"),
    ("test_dual_load", "app/src/main/cpp/tests/test_dual_load.c"),
    ("test_4_game_smoke", "app/src/main/cpp/tests/test_4_game_smoke.c"),
    ("test_mario_physics_parity", "app/src/main/cpp/tests/test_mario_physics_parity.c"),
    ("test_gml_comprehensive", "app/src/main/cpp/tests/test_gml_comprehensive.c"),
    ("test_gml_vm_execution", "app/src/main/cpp/tests/test_gml_vm_execution.c"),
    ("test_gml_vm_expanded", "app/src/main/cpp/tests/test_gml_vm_expanded.c"),
    ("test_gml_ds_collisions", "app/src/main/cpp/tests/test_gml_ds_collisions.c"),
]

def run():
    print("=== Running Engine Master Test Suite ===")
    all_passed = True
    for name, path in TESTS:
        out_bin = f"/tmp/{name}"
        compile_str = f"gcc -D_GNU_SOURCE -Iapp/src/main/cpp/include -Iapp/src/main/cpp {path} app/src/main/cpp/src/*.c app/src/main/cpp/gml_vm.c -lz -lm -lpthread -o {out_bin}"
        res_comp = subprocess.run(compile_str, shell=True, capture_output=True, text=True, errors='replace')
        if res_comp.returncode != 0:
            print(f"[FAIL] Compilation failed for {name}:\n{res_comp.stderr}")
            all_passed = False
            continue

        res_run = subprocess.run([out_bin], capture_output=True, text=True, errors='replace')
        if res_run.returncode != 0:
            print(f"[FAIL] Test {name} failed with exit code {res_run.returncode}:\n{res_run.stderr}")
            all_passed = False
        else:
            print(f"[PASS] {name}")
            if res_run.stdout.strip():
                for line in res_run.stdout.strip().split('\n'):
                    print(f"       {line}")

    if all_passed:
        print("\nALL NATIVE HOST TEST SUITES PASSED SUCCESSFULLY!")
        return 0
    else:
        print("\nSOME TEST SUITES FAILED!")
        return 1

if __name__ == "__main__":
    sys.exit(run())
