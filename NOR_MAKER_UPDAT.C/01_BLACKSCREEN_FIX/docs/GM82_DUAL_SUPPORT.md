# GM82 Dual Support (GMK + .gm82 text)

## Design (unchanged)

```
load (GMK or .gm82) → Project IR (usually incomplete)
        ↓
materialize_all()
        ↓
runtime_guard (must be complete)
        ↓
game loop / draw
```

No path skips the guard. Black screen is impossible.

## What was added

| File | Role |
|------|------|
| `gm82_gmk_reader.c/h` | Binary .gmk / .gm81 loader (version 800) |
| `gm82_text_reader.c/h` | Minimal .gm82 text / directory loader |
| `gm82_loader.c/h` | Auto-detect + `gm82_load_and_prepare` |
| `gm82_gmk_format.c` | Real `gm82_gmk_probe` implementation |

## What is still incomplete (honest)

- Full sprite frame pixel decode (zlib + BGRA) – structure is ready, decoder body still TODO
- Full background pixel decode – same
- Full .gm82 text resource walk (sprites/, backgrounds/, rooms/ folders)
- Objects, events, actions, collision, audio

Until those are finished, `complete` stays `false` and the guard correctly refuses to start the game.

## How to use

```c
gm82_project_ir *ir = NULL;
char err[512];
if (!gm82_load_and_prepare("/path/to/game.gmk", &ir, err, sizeof(err))) {
    // show err – do not draw
    return;
}
// only reaches here when ir->complete == true
```

Same call works for a .gm82 project directory.
