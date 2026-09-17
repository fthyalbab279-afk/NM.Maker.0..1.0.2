# NOR Maker – Black Screen Fix (Phase 0+1 Integration)

## What this patch does

1. Introduces a **hard runtime guard** that refuses to start the game loop when the Project IR is incomplete.
2. Adds a **materialization layer** that is the single place where sprites / backgrounds become usable.
3. Makes the black empty screen impossible: either the game starts with real resources or the UI shows a clear error.

## Files added

```
include/
  gm82_gmk_format.h      (updated)
  gm82_project_ir.h      (new – IR v5)
  gm82_materialize.h     (new)
  gm82_runtime_guard.h   (new)
src/
  gm82_project_ir.c
  gm82_materialize.c
  gm82_runtime_guard.c
  gm82_game_start.c      ← call this from JNI
tests/
  test_runtime_guard.c
```

## How to wire it into the existing Android native code

### 1. In your existing GMK loader (after `gm82_gmk_reader` finishes)

```c
#include "gm82_project_ir.h"
#include "gm82_game_start.h"   // or the .c above

// After you have filled the IR from the reader:
char err[512];
if (!gm82_game_try_start(ir, err, sizeof err)) {
    // Show this string on the Android UI instead of a black GL surface
    // e.g. Java: showErrorDialog(err);
    __android_log_print(ANDROID_LOG_ERROR, "NOR", "%s", err);
    return;   // do NOT create the game loop / room
}

// Only reach here when ir->complete == true
start_game_loop(ir);
```

### 2. CMakeLists.txt addition

```cmake
add_library(gm82_android SHARED
    ... existing sources ...
    src/gm82_project_ir.c
    src/gm82_materialize.c
    src/gm82_runtime_guard.c
    src/gm82_game_start.c
)
target_include_directories(gm82_android PRIVATE include)
```

### 3. Immediate visual change

- Before: black empty screen when resources are partial.
- After: clear error message “Resources incomplete – materialize failed (black screen prevented)” and the game never starts drawing.

## Next concrete coding tasks (in order)

1. **Finish sprite payload decoder** inside `gm82_materialize_sprites`  
   – walk the GMK sprite chunk, decompress the frames (zlib / proprietary), allocate `gl_textures[]`, mark status = DECODED.

2. **Finish background payload decoder** the same way.

3. **Once both return true**, `gm82_materialize_all` will set `complete = true` and the guard will open.

4. Only then enable `opts.upload_to_gpu = true` after the EGL context is current.

## Test

```bash
# host test (no Android needed)
gcc -Iinclude -o test_guard tests/test_runtime_guard.c \
    src/gm82_project_ir.c src/gm82_materialize.c src/gm82_runtime_guard.c
./test_guard
# expected: ALL_RUNTIME_GUARD_TESTS_PASS
```

This is the exact first step that stops the hallucination.
