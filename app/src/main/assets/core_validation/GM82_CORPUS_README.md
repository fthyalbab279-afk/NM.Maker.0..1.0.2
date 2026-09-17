# GM8.2 Corpus المتدرج

هذا المجلد مخصص لبناء مجموعة اختبارات حقيقية لصيغة GMK 8.0/8.2 داخل مشروع NOR Maker. الهدف هو عزل فرق واحد في كل Fixture حتى يمكن استنتاج حدود الـ payload الداخلية بواسطة binary differential analysis، ثم قياس round-trip إلى Project IR.

## قاعدة المصدر

الملفات التي تحمل `source=gm82_export` يجب أن تُنشأ من GameMaker 8.2 فعلياً. الملفات الاصطناعية الموجودة في `build_test/` مفيدة لاختبار سلامة parser وRoom decoder، لكنها لا تثبت layout الأصلي ولا توافق L2. الملفات الواقعية الحالية `IA.gmk` و`shotgun.gmk` تُنسخ إلى `gm82/90_real_projects/` بواسطة المستخدم أو أداة تجهيز corpus ولا تُعد fixtures معزولة.

## البنية

| المسار | الغرض |
|---|---|
| `gm82/00_empty` | baseline مشروع فارغ |
| `gm82/10_sprite` | Sprite وsubimages وorigin وmask والخيارات |
| `gm82/20_object` | Object properties وEvents وActions وGML |
| `gm82/30_room` | Room settings وviews وinstances وtiles |
| `gm82/90_real_projects` | المشاريع الكبيرة الواقعية المقدمة |
| `gm82/99_negative` | ملفات مبتورة أو معدلة لاختبار الرفض الآمن |
| `manifests/` | وصف التغيير المقصود ومعايير القبول |
| `tools/` | أدوات التحقق والتشغيل |

## طريقة تسمية الملفات

يجب أن يطابق اسم الملف في `gm82_corpus_manifest.json`. لا تستبدل fixture مفقوداً بملف مولد يدوياً وتضع عليه علامة `gm82_export`. إذا لم يتوفر الملف، يجب أن يظل الاختبار `MISSING` حتى لا يتحول غياب البيانات إلى نجاح زائف.

## ترتيب الاستخدام

ابدأ بـ `SPR-01-basic` ثم `SPR-02-origin`، وبعدها multiframe وmask. انتقل إلى Object basic ثم Create GML ثم الأحداث المتعددة والأفعال. أخيراً اختبر Room الفارغة ثم view وinstance وtile ثم combined. لكل انتقال يجب حفظ `before.gmk` و`after.gmk`، وحساب diff، وتسجيل offsets المرشحة في تقرير منفصل.

## بوابة L2

لا تعلن L2 إلا عندما تكون جميع fixtures المطلوبة موجودة، ويعيد reader نفس الـ canonical IR بعد writer ثم reader، وتكون الحقول غير المفهومة محفوظة كـ raw bytes، وتنجح اختبارات ASan وUBSan. نجاح probe وحده أو إنتاج manifest جزئي لا يكفي.
