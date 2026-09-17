# NOR Maker – Overall Progress (Honest)

**آخر تحديث:** 2026-09-15 15:45 EEST

## النسبة الإجمالية الحالية: **≈ 34%**

### تفصيل حسب الطبقة

| الطبقة | النسبة | ملاحظة |
|--------|--------|--------|
| Resource decode (sprites/bg) | **42%** | كل العينات الأربعة بقت تطلع frames (zelda 49، plataformas 42، shooter 22، mario 157 مع تكرار) |
| Objects / rooms / instances | 50% | — |
| Runtime loop + events | 28% | groups جاهزة |
| GML interpreter | 12% | — |
| Input / View | 40% | — |
| Audio | 5% | — |
| GLES Android | 15% | — |
| gm82core | 8% | — |

**الوزن المرجح ≈ 34%**

### ما تم في هذه الجولة
- إصلاح scan_blob: بحث byte-by-byte عن ver=800 (كان السبب في فشل zelda)
- zelda / plataformas / shooter بقوا يفكوا frames
- materialize groups موجود من قبل

### المتبقي العاجل في Resources
- إزالة التكرارات (dedupe) من نتائج الـ decoder
- نفس التحسين للـ backgrounds
- اختبار materialize كامل على mario + zelda
