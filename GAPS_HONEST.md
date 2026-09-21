# NOR Maker – ما يوجد فعلاً vs ما ناقص (مقارنة صادقة مع ويندوز GM82)

تاريخ التحديث: 2026-09-20
النسبة الحقيقية مقارنة بـ Windows GM82 الكامل: **~38% (أقل من 40%)**

---

## 📊 التقييم الصادق لمستوى التطابق مع ويندوز GM82

| المجال | التغطية الحالية | الملاحظات |
|--------|----------------|-----------|
| **GMK File Parser** | ~60% | يفك الهيدر ومعظم الموارد الأساسية، لكن بعض الخصائص المتقدمة لم تفك بعد |
| **GML Runtime / Interpreter** | ~35% | مفسر بسيط للتعابير والأوامر الأساسية؛ لا يوجد مفسر Bytecode كامل مثل ويندوز |
| **DnD Actions Engine** | ~40% | دعم الأساسيات (الحركة، تغيير السبرايت، تغيير الكائن، المنبهات) |
| **Physics & Collisions** | ~30% | اصطدام مربع AABB + منصات البلاط؛ لا يوجد Precise per-pixel collision mask |
| **Graphics & Rendering** | ~35% | Software Renderer على المضيف + هيكل GLES مبدئي؛ لا توجد الشادارات والـ Surfaces المتقدمة |
| **Audio Engine** | ~25% | طابور الأوامر وفك الصوت متوفر على Host؛ تشغيل العتاد المباشر لم ينتهي بعد |
| **النسبة الكلية** | **~38%** | **أقل من 40% مقارنة بنواة ويندوز GM82 الكاملة** |

---

## ✅ ما تم إنجازه واختباره على المضيف (Host Prototype)

1. فك ملفات GMK وإعادة بناء الموارد في الذاكرة (`mario_bros`, `plataformas`, `shooter`, `zelda`).
2. Soft rendering وإظهار أول إطار بدون شاشة سوداء (`nonzero_pixels > 1000`).
3. تجربة 4 ألعاب بـ 10 خطوات بدون انهيار (Smoke 4/4 PASS).
4. محاكاة حركة ماريو 100 إطار مع الجاذبية والمنصات وتتبع الكاميرا (`MARIO_PLAYABLE_PASS_HOST`).
5. تقييم عبارات GML الأساسية (`x=`, `coins+=`, `if`, `repeat`, `with`).
6. فك أفعال DnD الشائعة وتطبيقها على الكائنات.
7. طابور تشغيل الصوت البرمجي والربط بـ JNI.

---

## ❌ المتبقي الكبير للوصول لتطابق ويندوز (REMAINING > 60%)

1. **Full GML Bytecode VM:** دعم كافة دوال ومكاتب GML وشجرات التنفيذ المعقدة.
2. **Precise Collision Masking:** اصطدام البكسل بدقة لكل سبرايت بدلاً من AABB.
3. **GLES Hardware Pipeline:** رفع التكستشرات وإطارات الرسم للـ GPU على أندرويد.
4. **OpenSL ES Audio Backend:** تشغيل الصوت الحقيقي المباشر على الجهاز.
5. **Advanced GM82 Features:** Particles, mp_grid, ds_lists, ds_maps, Surfaces, Blend modes, Paths, Timelines الكاملة.

---

## التعهد بالشفافية

عدم ادعاء "100%" أو "Complete Engine". النسبة الحالية هي **38%** حقيقية مع العمل التدريجي لتوسيع النواة.
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
