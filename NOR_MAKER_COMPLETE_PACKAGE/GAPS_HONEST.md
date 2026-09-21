# NOR Maker – ما يوجد فعلاً vs ما ناقص (بدون وهم)

تاريخ: 2026-09-13

---

## ✅ موجود ومُختبر على mario_bros.gmk

| المكوّن | الدليل |
|---------|--------|
| فك GMK header + zlib | يعمل |
| Sprite pixels (BGRA→RGBA) | 78 frame |
| Background pixels | 3 خلفيات |
| Objects (اسم + sprite_index + solid) | 15 object |
| Rooms + instances | room0: 181 instance |
| Runtime Create/Step/Draw | 181 instance تتحرك |
| Collision AABB solid | لاعب يستقر على الأرض |
| GML builtins (C API) | instance_create, place_meeting, motion_* ... |
| Behaviors بالاسم | mario gravity, enemy patrol, blocks solid |
| Keyboard input state | keyboard_check / pressed / released |
| View/camera follow | view_x/y + clamp للغرفة |
| Soft render → PPM | ملفات معاينة حقيقية |
| Runtime Guard + complete flag | يمنع الشاشة السوداء عند نقص الموارد |

---

## ❌ غير موجود (لا يُدعى وجوده)

| المكوّن | الوضع |
|---------|--------|
| مفسّر GML / bytecode من السكربتات داخل GMK | غير موجود |
| فك Action lists (DnD) بالكامل | فشل محاذاة الـ format – لم يُنفَّذ |
| تنفيذ أحداث من بيانات الملف الأصلية | غير موجود (behaviors بديل بالاسم فقط) |
| Collision precise / mask | غير موجود (AABB فقط) |
| Audio playback | غير موجود |
| OpenGL/EGL على Android | غير موجود في هذه الطبقة (soft CPU فقط) |
| Particles / paths / timelines | غير موجود |
| Surfaces / blend modes | غير موجود |
| gm82core DLL compatibility كاملة | غير موجود |
| دعم .gm82 text format كامل | skeleton فقط |
| Zelda/plataformas sprite layout مختلف | غير مغطى بالكامل |
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
| APK Android UI integration لهذه النواة | لم يُدمج في الـ APK بعد |

---

## ⚠️ جزئي / محدود

| المكوّن | القيد |
|---------|--------|
| Dual loader .gm82 | يكتشف الامتداد لكن لا يفك كل الحقول |
| draw_sprite من السكربت | hook فاضي – الرسم عبر runtime_draw فقط |
| Alarms | العداد ينقص، لا يوجد event handler حقيقي |
| Views من بيانات الـ room | تهيئة يدوية، لا قراءة views من GMK |
| Parent objects / inheritance | غير مدعوم |
| Persistent instances عبر الغرف | جزئي (علم موجود، اختبار محدود) |

---

## ترتيب النواقص المتبقية (واقعي)

1. Action list / GML script parser من الـ GMK
2. ربط النواة بـ Android (JNI + GLSurfaceView)
3. Audio
4. Precise collision
5. توسيع صيغ GMK (zelda وغيرها)
6. gm82core subset

أي ادعاء "100% GM82" قبل إنهاء القائمة أعلاه = غير صحيح.
