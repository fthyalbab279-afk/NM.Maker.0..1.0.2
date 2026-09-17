# NOR Maker – GM82 Dual Support Package (honest status)

تاريخ: 2026-09-13

## الهدف
تشغيل ألعاب **Game Maker 8.2** (GMK binary + .gm82 text) مع الحفاظ على نفس الـ design:
load → Project IR → materialize → runtime guard → play

## الحالة الحقيقية الآن

| المكوّن | الحالة |
|---------|--------|
| GMK 800 probe + load | موجود (partial IR) |
| .gm82 text / directory load | موجود (minimal, partial IR) |
| Unified loader + auto-detect | موجود |
| Runtime Guard (منع الشاشة السوداء) | موجود ويشتغل |
| Materialize scaffolding | موجود |
| فك pixels كامل للـ sprites | **ناقص** |
| فك pixels كامل للـ backgrounds | **ناقص** |
| Objects / Events / Collision / Audio | **ناقص** |
| محاكاة كاملة زي ويندوز GM82 | **ناقص** |

الاختبار الحالي على mario_bros.gmk:
```
ok=false (expected), complete=0
message: Resources incomplete – materialize failed (black screen prevented)
ALL_DUAL_LOAD_TESTS_PASS
```

هذا هو السلوك الصحيح: اللعبة لا تبدأ لأن الـ resources لسه مش DECODED.

## هيكل الحزمة
01_BLACKSCREEN_FIX/  → الكود الجديد (GMK + .gm82 + Guard)
00_ORIGINAL_FROM_APK/ → ملفات الـ validation الأصلية
02_SAMPLES/ → العينات الحقيقية

## الخطوة الجاية الحقيقية
كتابة decoder الـ sprite frames (zlib + BGRA حسب صيغة GMK 800) جوه gm82_materialize_sprites.

مفيش ادعاء إن اللعبة بتشتغل كاملة.
