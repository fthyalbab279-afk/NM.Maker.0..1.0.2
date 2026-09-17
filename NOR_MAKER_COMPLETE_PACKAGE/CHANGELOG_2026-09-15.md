# Changelog – 2026-09-15

## تعديلات حقيقية في هذه الجولة

### 1. gm82_sprite_decode.c
- تغيير `scan_blob` من مسح كل 4 بايت إلى مسح byte-by-byte
- السبب: بعد أسماء ذات طول غير محاذي، علامة ver=800 مش بتكون aligned
- النتيجة المقاسة:
  - mario_bros: frames تُفك (مع تكرار يحتاج dedupe لاحقاً)
  - zelda: 49 frame (كانت 0)
  - plataformas: 42 frame (كانت 0)
  - shooter: 22 frame (كانت 1)

### 2. gm82_materialize.c
- إضافة بناء sprite groups
- `subimage_count` بقى يعكس عدد الإطارات الحقيقي للمجموعة

### 3. ملفات التوثيق
- OVERALL_PROGRESS.md (النسبة ≈ 34%)
- PHASE1_STATUS.md
- PROGRESS_REPORT.md

## ما لم يُغيّر
- باقي ملفات الـ runtime / events / GML / audio / GLES كما هي في الأرشيف الأصلي
- مفيش ادعاء إن المحرك بقى 100% أو إن الشاشة السوداء اتحلت من جديد (كانت متشالة من الأولويات حسب الطلب)
