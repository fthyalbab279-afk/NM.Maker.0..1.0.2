# تقرير استكمال نواة GMK وتهيئة Android

**الحالة:** تنفيذ جزئي مثبت باختبارات host، مع حجب أي ادعاء بالتوافق الكامل أو تصدير ROM حقيقي لم يُختبر.

## ما تم تنفيذه

تم توسيع قارئ `gm82_gmk_reader.c` ليقرأ Room views وفق ترتيب GM8.2 المرجعي، بما في ذلك حالة الظهور، أبعاد view وport، الحدود والسرعات وobject ID. كما يقرأ Room instances مع الإحداثيات وobject ID وinstance ID وcreation code وlocked، ويقرأ Room tiles مع الخلفية، المصدر، الأبعاد، العمق، tile ID وlocked. أضيفت حماية حدود العدادات والـ payload، وescaping لشفرة إنشاء instance قبل إدخالها إلى JSON.

تم أيضًا استكمال استهلاك ذيل حالة محرر Room: `remember`، أبعاد المحرر، flags الثمانية، tab، وscroll bars. هذا يمنع اعتبار payload منتهيًا قبل نهاية schema المحلية للغرفة.

## الاختبار المثبت

تم توسيع `build_test/make_gmk_payload_fixture.py` ليولد Room يحتوي view وinstance وtile بقيم غير صفرية. ويقوم `build_test/verify_room_detail.py` بتحليل JSON فعليًا، ويثبت القيم التالية:

| العنصر | النتيجة المثبتة |
|---|---|
| View | `320x240`، port `320x240`، borders `32/32`، object `-1` |
| Instance | `(32,64)`، object `0`، instance `100`، creation code محفوظ حرفيًا |
| Tile | `(16,32)`، background `-1`، الحجم `16x16`، depth `100`، tile `7` |
| Integrity | `bytesConsumed == bytesTotal` في fixture السليم |
| Truncation guard | الملف المبتور لا يُعلن `payloadStatus=decoded` |

آخر نتيجة تشغيل موثقة هي `ROOM_DETAIL_REGRESSION_PASS` و`ROOM_TRUNCATION_GUARD_PASS`.

## استعادة مشروع Android

تمت استعادة `build.gradle` الجذري، و`app/build.gradle`، و`AndroidManifest.xml`، و`styles.xml`. كما استُعيد `gm82_portable_compat.h/.c`، وأصبح CMake يشير فقط إلى ملفات موجودة. أضيف ملف `retro_rom_native.c` لربط رموز NES/GBC/GBA مع فشل مغلق؛ فهو لا يكتب ROM وهمية ولا يعلن نجاح تصدير غير حقيقي.

## القيد الحالي

تعذر بناء APK في هذه البيئة لأن Android SDK غير موجود (`SDK location not found`). أداة Gradle 8.2.1 وملفات المشروع موجودة، لكن لا يمكن إثبات APK جديد حتى يتوفر SDK وNDK وCMake. كما أن رموز ROM الحالية placeholders فاشلة عمدًا؛ تحقيق تصدير NES/GBC/GBA أصيل يتطلب backend assembler/linker حقيقيًا واختبارات emulator، ولم يتم الادعاء بأنه مكتمل.

## الملفات المهمة

| الملف | الغرض |
|---|---|
| `android_project_base/app/src/main/cpp/gm82_gmk_reader.c` | فك GMK وRoom التفصيلي |
| `build_test/make_gmk_payload_fixture.py` | توليد fixture دلالي |
| `build_test/verify_room_detail.py` | regression JSON وtruncation |
| `build_test/gmk_room_detail_regression.log` | آخر سجل نجاح |
| `android_project_base/build.gradle` | إعداد Gradle الجذري |
| `android_project_base/app/build.gradle` | إعداد Android/ABI/CMake |
| `android_project_base/app/src/main/AndroidManifest.xml` | Manifest التطبيق |
| `android_project_base/app/src/main/cpp/retro_rom_native.c` | fail-closed symbols للـ ROM غير المنفذة |

## الخطوة اللازمة لإكمال البناء

يجب توفير Android SDK يتضمن platform وbuild-tools، وNDK r26d، وCMake 3.22.1، ثم وضع `sdk.dir` في `android_project_base/local.properties` وتشغيل `../.tools/gradle-8.2.1/bin/gradle assembleDebug`. بعد نجاح ذلك يمكن ربط manifest التفصيلي والـ Project IR واختبار الاستيراد على جهاز Android فعلي. لا يمكن اعتبار parity بنسبة 100% مع Windows GM8.2 مثبتة اعتمادًا على fixture صناعي فقط؛ يلزم GMK/GMX/GMZ حقيقي واختبار فتح/حفظ/إعادة فتح على برنامج GM8.2 الأصلي.

## تحديث تقوية النواة

استُعيدت ترويسة `gml_vm.h` التي كانت مفقودة من شجرة Android، وأضيفت واجهة `gml_vm_invoke` لتغطية استدعاء السكربتات مع arguments وscope. أثناء التحقق اكتُشف خلل ذاكرة حقيقي في `gml_vm_invoke`: كان يحفظ `error` بحجم 160 بايت ثم ينسخه إلى buffer أكبر غير متطابق. تم توحيد الحجم إلى 160 بايت، وأُعيد تشغيل اختبارات `do-until` و`switch` و`invoke` باستخدام AddressSanitizer وUndefinedBehaviorSanitizer دون فشل.

أضيف اختبار `build_test/gml_invoke_test.c`، وكانت نتيجته `GML_INVOKE_SCOPE_TEST_PASS`. كما أُضيف `build_test/core_benchmark.c` لقياس مساري VM وGMK على fixture ثابت. آخر قياس محلي على 1000 تكرار كان `vm_seconds=0.018164` و`gmk_seconds=0.013343`، مع checksum `4950000`؛ هذه أرقام baseline للبيئة الحالية وليست مقارنة عادلة مع Windows أو Android.

تم أيضًا اختبار GMK decoder تحت sanitizers على fixture سليم ومبتور، وكانت النتيجة: السليم يفك Room، والمبتور يخرج `parseStatus=partial` دون إعلان Room decoded، ولم تظهر أخطاء sanitizer.
