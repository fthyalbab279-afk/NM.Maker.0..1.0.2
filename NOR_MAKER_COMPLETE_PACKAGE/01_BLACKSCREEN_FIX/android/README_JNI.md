# Android JNI scaffold

## الحالة
- `gm82_jni.h` / `gm82_jni.c` يربطان النواة C الحالية
- `gm82_native_draw()` = **STUB** (مفيش GL لسه)
- التحميل + step + input جاهزين على مستوى C

## المطلوب لربطه بالـ APK
1. Android.mk / CMakeLists مع NDK
2. Java `native` methods تطابق gm82_native_*
3. GLSurfaceView + رفع textures من rgba في sprites
4. تمرير مفاتيح اللمس → gm82_native_key_* / touch

## ليس جاهزاً للتشغيل داخل APK الآن
