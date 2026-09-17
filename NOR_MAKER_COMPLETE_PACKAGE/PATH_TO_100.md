# الطريق الحقيقي إلى 100% – بدون وهم

## تعريف 100%

اللعبة تشتغل على Android **مثل ويندوز GM82** في:
- تحميل GMK/GM82 كامل
- كل الـ sprites/backgrounds/objects/rooms/sounds
- تنفيذ GML/DnD من الملف
- collision دقيق
- صوت
- رسم OpenGL
- gm82core الأساسي
- 20+ لعبة مجتمع بدون شاشة سوداء

## التقدم الحالي (تقديري)

| طبقة | نسبة تقريبية | ملاحظة |
|------|----------------|--------|
| Resource decode (sprites/bg) | ~40% | mario_bros يعمل؛ صيغ أخرى جزئية |
| Objects / rooms / instances | ~50% | instances تُستخرج؛ actions لا |
| Runtime loop | ~35% | Create/Step/Draw + AABB |
| GML | ~15% | builtins C فقط، بدون مفسّر سكربت |
| Input / View | ~40% | state جاهز؛ مش مربوط بـ Android UI |
| Audio | 0% | |
| GPU Android | 0% | soft PPM فقط |
| gm82core | 0% | |
| **الإجمالي التقريبي** | **~25–30%** | ليس 100% |

## العمل المتبقي (تقدير جهد)

| # | مهمة | تقدير |
|---|------|--------|
| 1 | Action/DnD + GML script parser كامل | أسابيع |
| 2 | JNI + GLSurfaceView + texture upload | أسابيع |
| 3 | Audio (OpenSL/AAudio) | أيام–أسبوع |
| 4 | Precise collision masks | أيام |
| 5 | كل صيغ GMK 800 variants | أسابيع |
| 6 | gm82core subset | أسابيع |
| 7 | Validation corpus 20+ لعبة | مستمر |

**الإجمالي الواقعي: أشهر لفريق صغير، ليس ساعات.**

## سياسة ضد الهلوسة

1. لا يُعلن complete إلا بعد اختبار على ملف حقيقي
2. كل فجوة موثقة في GAPS_HONEST.md
3. Behaviors بالاسم ≠ تنفيذ أحداث الملف
4. PPM ≠ محرك Android كامل
