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
