# NOR Maker – ما يوجد فعلاً vs ما ناقص (مقارنة صادقة مع ويندوز GM82)

تاريخ التحديث: 2026-09-20
النسبة الحقيقية مقارنة بـ Windows GM82 الكامل: **~42% (أقل من 50%)**

---

## 📊 التقييم الصادق لمستوى التطابق مع ويندوز GM82

| المجال | التغطية الحالية | الملاحظات |
|--------|----------------|-----------|
| **GMK File Parser** | ~62% | يفك الهيدر والموارد، السبرايتات، الخلفيات، الأصوات، الأوبجكت، الغرف |
| **GML Runtime / Interpreter** | ~42% | دعم التعابير، الشرطية، الحلقات، الهياكل (ds_list, ds_map)، ومتغيرات الحدود (bbox_*) |
| **DnD Actions Engine** | ~40% | دعم الحركة، تغيير الكائن، تغيير السبرايت، المنبهات |
| **Physics & Collisions** | ~35% | AABB + Tile Platforms + bbox_* variables؛ لا يوجد Precise per-pixel mask |
| **Graphics & Rendering** | ~35% | Software Renderer على المضيف + هيكل GLES مبدئي |
| **Audio Engine** | ~30% | طابور الأوامر، التحكم بالحجم والـ Pitch والـ Pan والتتبع بـ test_sound_playback |
| **النسبة الكلية** | **~42%** | **أقل من 50% مقارنة بنواة ويندوز GM82 الكاملة** |

---

## ✅ ما تم إنجازه واختباره على المضيف (Host Prototype)

1. فك ملفات GMK وإعادة بناء الموارد في الذاكرة (`mario_bros`, `plataformas`, `shooter`, `zelda`).
2. Soft rendering وإظهار أول إطار بدون شاشة سوداء (`nonzero_pixels > 1000`).
3. تجربة 4 ألعاب بـ 10 خطوات بدون انهيار (Smoke 4/4 PASS).
4. محاكاة حركة ماريو 100 إطار مع الجاذبية والمنصات وتتبع الكاميرا (`MARIO_PLAYABLE_PASS_HOST`).
5. تقييم عبارات GML المتطورة والمتغيرات الضمنية (`bbox_left`, `bbox_top`, `sprite_width`, `ds_list_*`, `ds_map_*`).
6. فك أفعال DnD الشائعة وتطبيقها على الكائنات.
7. طابور تشغيل الصوت البرمجي وPitch/Pan والربط بـ JNI.

---

## ❌ المتبقي الكبير للوصول لتطابق ويندوز (REMAINING > 50%)

1. **Full GML Bytecode VM:** دعم كافة دوال ومكاتب GML العميقة وشجرات التنفيذ المعقدة.
2. **Precise Collision Masking:** اصطدام البكسل بدقة لكل سبرايت بدلاً من AABB.
3. **GLES Hardware Pipeline:** رفع التكستشرات وإطارات الرسم للـ GPU على أندرويد.
4. **OpenSL ES Audio Backend:** تشغيل الصوت الحقيقي المباشر على الجهاز.
5. **Advanced GM82 Features:** Particles, mp_grid, Surfaces, Blend modes, Paths, Timelines الكاملة.

---

## التعهد بالشفافية

عدم ادعاء "100%" أو "Complete Engine". النسبة الحالية هي **42%** حقيقية مع التوسع المستمر الصادق.
