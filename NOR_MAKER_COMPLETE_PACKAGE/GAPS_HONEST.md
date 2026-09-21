# NOR Maker – ما يوجد فعلاً vs ما ناقص (تحديث صادق ملزم)

تاريخ التحديث: 2026-09-20
النسبة الحالية: **85% (HOST_MVP)** — ليس 100%

---

## ✅ موجود ومُختبر على المضيف (Host MVP) باختبارات مسجلة في الريبو

| المكوّن | الدليل والاختبارات في الريبو |
|---------|------------------------------|
| فك GMK header + zlib | test_core_suite (4 عيّنات: mario_bros, plataformas, shooter, zelda) |
| Materialize pixels (BGRA→RGBA) | test_runtime_guard & test_dual_load |
| Background pixels | materialize_backgrounds |
| Objects (اسم + sprite_index + solid) | test_phase2_smoke_4games (mario: 47 obj) |
| Rooms + instances + tiles | test_phase2_smoke_4games |
| أسماء غرف مرنة | room*, rm_*, r001, r_* في gm82_object_room_decode.c |
| Runtime Create/Step/Draw | test_phase2_smoke_4games (4/4 PASS) |
| Mario Player physics parity | test_mario_physics_parity (100 إطار, dx=96.0, MARIO_PLAYABLE_PASS_HOST) |
| GML builtins + AST evaluation | test_phase3_gml PASS (x=, coins+=, if, repeat, with) |
| DnD actions parsing + apply | test_dnd_args & test_dnd_action_apply PASS |
| Alarms countdown + fire | alarms[0..11] |
| Keyboard input state | test_phase4_input_sound |
| View/camera follow | view_x/y follow target |
| Sound decode + command queue | test_sound_playback & test_audio_device PASS |
| Soft render → framebuffer | non-zero pixels > 1000 PASS |
| Runtime Guard | test_runtime_guard PASS |

---

## ❌ غير موجود / متبقي للجهاز (REMAINING)

| المكوّن | الوضع الحالي |
|---------|---------------|
| GLES Hardware Texture Upload & Draw على جهاز أندرويد | BLOCKED (لا يوجد emulator أو أندرويد متصل في البيئة) |
| OpenSL ES Hardware Audio Playback على جهاز أندرويد | BLOCKED (طابور الأوامر جاهز على Host، الجهاز غير متصل) |
| Per-pixel precise mask collision | المتاح AABB + Tile BBox platforms |
| Full GM82 Bytecode Compiler | المتاح GML AST Evaluator للعديد من التعابير والأوامر |
| Particles / mp_grid / ds_* | stubs ومكتبات جزئية فقط |
| Networking | غير موجود |

---

## ⚠️ جزئي / محدود

| المكوّن | القيد |
|---------|--------|
| Dual loader .gm82 | يكتشف الملف النصي؛ الفك الكامل جزئي |
| Parent objects / inheritance | غير مدعوم بالكامل |
| Persistent instances عبر الغرف | جزئي |

---

## التسمية الصريحة

- **المهمة الحالية:** `DEVICE_GLES_ONE_FRAME`
- **النتيجة:** `DEVICE_GLES_BLOCKED` (عدم توفر emulator / جهاز أندرويد في البيئة)
- **النسبة:** `85%` (HOST_MVP)
