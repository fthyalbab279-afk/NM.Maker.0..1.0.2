# NOR Maker – Honest Gaps and Implementation Matrix

Date: 2026-09-21

---

## Verified Features (Host & Core Engine)

| Feature | Status | Verification |
|---------|--------|--------------|
| GMK Header & Zlib Stream Parsing | PASS | All 4 sample games decode correctly |
| Sprite & Background Materialization | PASS | Nonzero RGBA pixel buffers generated |
| Objects & Room Instance Parsing | PASS | Rooms and instances extracted for Mario, Zelda, Shooter, Plataformas |
| Room Name Patterns | PASS | Decodes room*, rm_*, r001, r_* patterns |
| Runtime Engine Execution | PASS | Step/Draw cycles verified for 100+ simulation frames |
| Physics & Solid AABB Collision | PASS | Ground collision and movement verified on Mario |
| Sound Queue & OpenSL ES Bridge | PASS | Audio playback commands queued safely without crash |
| GML Expression & AST Evaluation | PASS | Builtins, math functions, conditionals, and script calls supported |
| Dual Loader (.gmk / .gm82) | PASS | Format auto-detection and guard validation passed |
| Android JNI Interface | PASS | Native JNI bridge linked for host and Android build targets |

---

## Remaining Gaps / Future Target Features

| Feature | Status | Description / Reason |
|---------|--------|----------------------|
| On-Device Hardware GLES Surface | BLOCKED / PENDING | Requires active Android device GLES context testing |
| On-Device OpenSL ES Sound Output | BLOCKED / PENDING | Audio queued natively; physical speaker output requires hardware device test |
| Full 100% GML Engine Parity | INCOMPLETE | Advanced GML edge cases and complex extension DLLs capped at MVP level |

Note: Progress percentage is capped at 85% (release tag HOST_MVP) until full on-device hardware testing passes.
