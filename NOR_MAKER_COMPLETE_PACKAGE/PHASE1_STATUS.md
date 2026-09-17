# Phase 1 – Resources Execution Status

**تاريخ:** 2026-09-15

## ما تم تنفيذه في هذه الجولة

1. **تحسين `gm82_materialize_sprites`**
   - أصبح يبني `sprite groups` من الإطارات المفكوكة.
   - `subimage_count` بقى صحيح للـ sprites متعددة الإطارات.
   - أول frame يُنقل ملكيته للـ IR (جاهز للرسم).

2. **تأكيد حالة الـ Decoder على العينات**
   | العينة | frames مفكوكة |
   |--------|----------------|
   | mario_bros.gmk | 78 |
   | shooter.gmk | 1 |
   | zelda.gmk | 0 |
   | plataformas.gmk | 0 |

3. **تحليل سبب فشل zelda / plataformas**
   - الـ zlib streams موجودة وكبيرة.
   - نمط `ver=800 + w + h + dlen` الموجود في mario مش موجود بنفس الشكل.
   - يحتاج مسح أعمق لبنية الـ resource chunk (مخطط المرحلة التالية).

## الناقص المتبقي في المرحلة 1

- توسيع `scan_blob` / resource walker لصيغ zelda و plataformas.
- إكمال materialize_backgrounds بنفس أسلوب الـ groups.
- اختبار materialize كامل على mario (complete flag).

## الخطوة التالية مباشرة

تحليل أعمق لبنية zlib في zelda واستخراج نمط الإطارات البديل.
