# NOR Maker – ما يوجد فعلاً vs ما ناقص (تحديث صادق شامل)

تاريخ التحديث: 2026-09-20
الإصدار الحالي: **HOST_MVP (85%)** — ليس 100%

---

## ✅ موجود ومُختبر ومؤكد باختبارات وحدة ومخرجات تشغيل (Host MVP)

| المكوّن | الدليل واختبارات التشغيل |
|---------|---------------------------|
| فك GMK header + zlib | 4 عيّنات: mario_bros, plataformas, shooter, zelda |
| Materialize pixels (BGRA→RGBA) | materialize_all يفحص السبرايتات والخلفيات ويملأ البكسلات |
| Background pixels | materialize_backgrounds لفك الخلفيات |
| Objects (اسم + sprite_index + solid) | 47 obj لماريو، 32 لزيلدا |
| Rooms + instances + tiles | room0/room1 وباقي الغرف مع فكInstances والبلاط |
| أسماء غرف مرنة | room*, rm_*, r001, r_* مع دعم كامل |
| Runtime Create/Step/Draw | 4/4 sample games step 10 PASS |
| Mario Player physics parity | اختبار 100 إطار: dx = 96.0 (>20)، استقرار على الأرض الصلبة وبلاط المنصات |
| GML builtins + AST evaluation | test_phase3_gml PASS (x=, coins+=, if, repeat, with) |
| DnD actions parsing + apply | test_dnd_args & test_dnd_action_apply PASS |
| Alarms countdown + fire | alarms[0..11] decrement and fire script callbacks |
| Keyboard input state | keyboard_check / pressed / released |
| View/camera follow | view_x/y + follow target |
| Sound decode + command queue | test_sound_playback & test_audio_device PASS |
| Soft render → framebuffer | non-zero pixels > 1000 PASS |
| Runtime Guard | يمنع الشاشة السوداء عند الموارد الناقصة |

---

## ❌ غير موجود / متبقي لم يتكتمل بعد (REMAINING)

| المكوّن | الوضع الحقيقي |
|---------|----------------|
| OpenSL ES / Audio Synthesis على جهاز أندرويد حقيقي | طابور الأوامر الجاهز يعمل على Host؛ التشغيل الفعلي على العتاد لسه REMAINING |
| Hardware GLES Surface Rendering على أندرويد | JNI lifecycle جاهز؛ العرض الهاردويري المباشر على جهاز حقيقي لسه REMAINING |
| Precise mask collision (Per-pixel mask) | المتاح حالياً AABB + Tile BBox platforms |
| Full Windows GM82 Bytecode Compiler | المتاح حالياً GML AST Evaluator للعديد من الأوامر والعبارات |
| Complete Particles / mp_grid / ds_* | stubs ومكتبات جزئية فقط |
| Networking | غير موجود |

---

## ⚠️ جزئي / محدود

| المكوّن | القيد |
|---------|--------|
| Dual loader .gm82 | يكتشف الملف النصي؛ الفك الكامل للهياكل المعقدة جزئي |
| Parent objects / inheritance | غير مدعوم بالكامل |
| Persistent instances عبر الغرف | مدعوم بنسبة جزئية |

---

## الخلاصة والتسمية الصريحة

- **الوضع الحالي:** `HOST_MVP` بنسبة **85%**.
- **ممنوع ادعاء 100% أو Complete Engine** حتى يتم اختبار الهاردوير (GLES + OpenSL) على جهاز أندرويد حقيقي وتغطية كافة فجوات GAPS.
