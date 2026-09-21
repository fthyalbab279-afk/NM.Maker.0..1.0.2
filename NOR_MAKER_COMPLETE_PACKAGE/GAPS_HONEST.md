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
| Networking | غير موجود |
| APK Android UI integration لهذه النواة | لم يُدمج في الـ APK بعد |

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
