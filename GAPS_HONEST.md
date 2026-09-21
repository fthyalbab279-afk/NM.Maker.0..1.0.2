# NOR Maker – Realistic Gap Matrix vs Full Windows GM Engine

Date: 2026-09-21
Estimated GM8.2 Windows Parity: ~38% (HOST MVP Subset)

---

## Verified Subsets (Host MVP Engine)

| Component | Status | Verified Metric |
|-----------|--------|-----------------|
| GMK Stream Decoding | PASS | mario_bros (301 inst, 115 tiles), plataformas (270 inst), shooter (472 inst), zelda (372 inst) |
| Soft CPU Frame Renderer | PASS | Soft PPM/RGBA rendering verified (>1000 nonzero pixels) |
| Host Mario Simulation | PASS | 100-frame simulation, horizontal player movement (dx=144.0 > 20) |
| Dual Loader (.gmk / .gm82) | PASS | Auto-detection and runtime guard verification passed |
| JNI Native Hooks | PASS | JNI bridging structures compiled and linked |

---

## Major Missing Features vs Full Windows GM Engine (>60% Missing)

| Missing System | Impact | Reality / Status |
|----------------|--------|------------------|
| Physical Device GLES Hardware Surface | BLOCKED | No GPU/emulator attached via adb in sandbox |
| Physical Device OpenSL ES Audio | BLOCKED | Direct audio speaker output unverified on device |
| Complete GML VM & AST Interpreter | INCOMPLETE | Minimal expression evaluator; lacks full closure/variable scope parity |
| Precise Mask Collisions | INCOMPLETE | Simple AABB bounding boxes only |
| Surfaces, Shaders, Blend Modes | INCOMPLETE | Not implemented |
| Full DnD Action Library | INCOMPLETE | Subset of actions implemented |
| Paths, Timelines, Particles, MP Grid | INCOMPLETE | Partial / Skeleton implementations |
| Networking, DLL extensions | INCOMPLETE | Not implemented |

Note: Progress is honestly capped at 38% compared to complete Windows GM8 runtime parity.
