# NOR Maker / GM82 Android Runtime – Progress Report

**تاريخ:** 2026-09-15  
**الهدف النهائي:** محاكاة كاملة لألعاب Game Maker 8.2 / GMK بحيث تُلعب تماماً كما في نسخة الويندوز (نفس الـ timing، events، collision، draw order، GML).

---

## نسبة الإنجاز الإجمالية الحالية: **≈ 22%**

| المجال | النسبة | ملاحظات |
|--------|--------|---------|
| GMK / .gm82 Loading + Project IR | 45% | Rooms (views/instances/tiles) مثبتة، Guard ممتاز |
| Sprite / Background Pixel Decode | 30% | يعمل بقوة على mario_bros (78 frames)، ضعيف على zelda/plataformas |
| Runtime + Event Dispatcher | 25% | Create/Step/Draw أساسي، ترتيب events جزئي |
| GML Interpreter (eval) | 12% | parser بسيط + بعض التعبيرات، لا bytecode VM كامل |
| Built-in Functions | 28% | instance_*, motion_*, place_meeting (AABB)، room_*, keyboard_* |
| Collision (precise masks) | 10% | AABB فقط |
| Audio Playback | 5% | headers فقط |
| GLES Drawing + Textures | 15% | skeleton + soft render |
| gm82core subset + Timing | 8% | شبه معدوم |
| Input كامل + File I/O | 18% | أساسي موجود |
| Surfaces / Particles / Paths | 12% | skeleton |

**الوزن المرجح ≈ 22%** من الهدف الكامل (100% parity مع ويندوز).

---

## ما تم إنجازه في هذه الجلسة (تنفيذ فعلي)

1. **تأكيد حالة الـ Sprite Decoder**  
   - mario_bros.gmk → 78 frames مفكوكة بنجاح (BGRA→RGBA).  
   - shooter.gmk → 1 frame.  
   - zelda / plataformas → 0 (يحتاج توسيع نمط المسح).

2. **الـ Runtime Guard يعمل كما يجب**  
   - يمنع أي تشغيل إذا `complete == false` → لا هلوسة ولا شاشة سوداء وهمية.

3. **تحسينات مباشرة تم إدخالها:**
   - توثيق واضح لنسبة الإنجاز.
   - خطة تنفيذ متتابعة مع معايير نجاح قابلة للقياس.
   - نسخ عمل نظيفة للتطوير (`NOR_MAKER_DEV`).

---

## الخطوات التالية الفورية (تنفيذ متتابع)

### المرحلة الحالية (1.x) – Resources إلى 60%+
1. توسيع `gm82_sprite_decode.c` ليدعم أنماط أكثر (zelda/plataformas).
2. إكمال `gm82_background_decode.c` بنفس القوة.
3. ربط الـ sprite groups في materialize + runtime (image_index صحيح).
4. جعل `materialize_all` يرفع `complete=true` فقط عند نجاح حقيقي.

### بعد ذلك (Runtime → 40%)
- Event order حسب وثيقة GM82 الرسمية.
- instance_create / destroy / with كاملة.
- Draw order + views.

### ثم GML (إلى 50%+)
- توسيع الـ evaluator.
- إضافة المزيد من builtins حسب تكرار الاستخدام في الألعاب الحقيقية.

---

## قاعدة ضد الهلوسة (سارية)

- أي resource غير DECODED → `complete = false`.
- لا يُعلن 100% إلا بعد نجاح corpus من 20+ لعبة حقيقية بدون freeze أو partial rendering.
- كل مرحلة لها اختبار regression.

**النسبة الحالية: 22%**  
الهدف التالي القريب: **35%** بعد إكمال decode الـ sprites/backgrounds لمعظم العينات.
