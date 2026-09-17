# NOR Maker → GM82 100% Parity Master Plan

**تاريخ:** 2026-09-13  
**الهدف:** محاكاة كاملة لألعاب Game Maker 8.2 على Android بنفس سلوك النسخة الأصلية على ويندوز.

---

## 0. الحقيقة الصريحة

| البند | الواقع |
|-------|--------|
| هل يمكن إنهاء 100% في جلسة واحدة؟ | **لا** |
| حجم العمل المتبقي | مشروع متوسط/كبير (أشهر لفريق صغير) |
| المرجع الرسمي | https://github.com/GM82Project + https://gm82.cherry-treehouse.com |
| المرجع التقني للـ format | LateralGM + OpenGMK + strings داخل libgm82_android.so |

أي ادعاء "خلص 100%" الآن = هلوسة.  
الخطة دي متتابعة وقابلة للقياس.

---

## 1. تعريف "100% مكتمل"

اللعبة تُعتبر مدعومة بالكامل عندما:

1. تُحمّل من `.gmk` أو `.gm82` بدون partial
2. كل الـ sprites / backgrounds / objects تظهر بشكل صحيح
3. الـ instances تُنشأ وتنفذ أحداث Create / Step / Draw / Collision / Alarm
4. الـ collision (bbox + precise) يطابق سلوك GM82
5. الصوت يشتغل
6. الـ timing يطابق (30/60 FPS + delta من gm82core)
7. أهم دوال gm82core الشائعة تعمل
8. 20+ لعبة حقيقية من المجتمع تشتغل بدون شاشة سوداء أو freeze

---

## 2. المراحل المتتابعة

### المرحلة 1 – Resources (الحالية)
**الهدف:** `complete = true` بعد materialize حقيقي

| # | المهمة | معيار النجاح |
|---|--------|---------------|
| 1.1 | فك قائمة الـ Sprites من GMK 800 | sprite_count > 0 + framesDecoded |
| 1.2 | فك pixels كل frame (zlib/BGRA) | gl_texture صالح أو RGBA buffer |
| 1.3 | فك Backgrounds | نفس المعيار |
| 1.4 | فك Objects (events/actions headers) | object_count > 0 |
| 1.5 | فك Sounds headers | sound_count > 0 |
| 1.6 | ربط materialize → complete=true فقط عند نجاح الكل | Guard يفتح |

### المرحلة 2 – Runtime الأساسي
| # | المهمة | معيار النجاح |
|---|--------|---------------|
| 2.1 | إنشاء instances من Room data | instances تظهر في الـ room |
| 2.2 | Event dispatcher (Create/Step/Draw) | كود الـ Create يتنفذ |
| 2.3 | Draw order صحيح | خلفية + tiles + sprites مرئية |
| 2.4 | View system | الكاميرا تتحرك |
| 2.5 | room_goto / room_restart | التنقل بين الغرف يشتغل |

### المرحلة 3 – GML Built-ins الأساسية
| # | المجموعة | أمثلة |
|---|----------|--------|
| 3.1 | Drawing | draw_sprite, draw_background, draw_set_color, draw_text |
| 3.2 | Instances | instance_create, instance_destroy, with, other, instance_number |
| 3.3 | Collision | place_meeting, instance_place, collision_rectangle, ... |
| 3.4 | Movement | motion_set, move_towards_point, speed/direction/hspeed/vspeed |
| 3.5 | Built-in vars | x,y,sprite_index,image_index,image_speed,depth,visible,solid |
| 3.6 | Alarms + Timelines | alarm[0..11], timeline_ |

### المرحلة 4 – gm82core subset
تنفيذ الدوال الأكثر استخداماً من gm82core (من الـ .gej الرسمي):
- timing: get_timer, delta_time equivalents
- math: approach, lerp helpers, angle_*
- string helpers
- ds_ آمنة (إن أمكن)
- input helpers

### المرحلة 5 – Audio + Input + File
- wav/ogg playback
- keyboard / mouse / virtual pad كامل
- file_text / ini أساسي

### المرحلة 6 – Surfaces + Advanced
- surface_create / surface_set_target
- basic blend modes
- (shaders لاحقاً عبر GLES)

### المرحلة 7 – Validation
- corpus من 20+ لعبة GM82
- مقارنة screenshots + logs مع ويندوز
- لا يُعلن 100% إلا بعد نجاح الـ corpus

---

## 3. قواعد صارمة ضد الهلوسة

1. أي resource مش DECODED → `complete = false`
2. أي built-in مش منفّذ → يرجع خطأ واضح أو no-op موثق، مش سلوك عشوائي
3. كل مرحلة لها اختبار regression
4. مفيش `complete=true` إلا بعد نجاح اختبارات المرحلة

---

## 4. الحالة الحالية (بعد كل الشغل السابق)

```
[x] Runtime Guard
[x] Project IR v5
[x] Dual loader skeleton (GMK + .gm82)
[x] zlib inflate helper
[ ] Sprite pixel decode   ← المرحلة 1.1 / 1.2 (قيد التنفيذ)
[ ] Background pixel decode
[ ] Objects / Events
[ ] Instance system
[ ] GML built-ins الكاملة
[ ] gm82core compatibility
[ ] Audio
```

---

## 5. ترتيب التنفيذ الفعلي من الآن

1. تحسين GMK reader لاستخراج قوائم الـ resources الحقيقية من العينات
2. Sprite materialize حقيقي (pixels)
3. Background materialize
4. Object stubs + Create event
5. Minimal draw loop
6. توسيع GML built-ins تدريجياً حسب ما تحتاجه العينات

كل خطوة هتتوثق بنتيجة اختبار على الملفات الموجودة (mario_bros / zelda / shooter).
