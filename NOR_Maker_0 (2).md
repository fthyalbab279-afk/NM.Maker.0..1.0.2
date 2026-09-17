# NOR Maker 0.1 — حزمة التسليم للعمل على الكمبيوتر

**تاريخ الحزمة:** 29 أغسطس 2026، وفق بيئة العمل الحالية.  
**النطاق:** نسخة Android الحالية فقط، مع الحفاظ على واجهة Windows Portable ومحررات Resource Explorer وRoom/Sprite/Background وخيارات 2D/2.5D/3D و3D Orbit.

## مستوى الإنجاز الحالي

المشروع في مرحلة **تكامل تدريجي** وليس Core مكتملًا بنسبة 100%. المسار المرئي للمستخدم ما زال Runner مبنيًا على JavaScript داخل WebView، بينما توجد نواة C/JNI موازية للاستيراد وProject IR وnative runtime. لم يتم استبدال Runner المرئي بالنواة الأصلية لأن ذلك يتطلب اختبارات differential أوسع.

| المجال | الحالة | الدليل أو الملاحظة |
|---|---|---|
| واجهة Android الحالية وWebView | **PASS** | المصدر محفوظ داخل `android_project_base` دون حذف الواجهات أو خيارات العرض. |
| treasure.gm82: الحركة والإدخال والتصادم والـanimation الأساسية | **PASS** على Chromium/Xvfb | بوابة `test_real_gm82_runtime_execution.js` سابقًا 28/28 PASS، والتسجيل المضيف ليس اختبار Android فعليًا. |
| GMK Project IR وmanifest الأصلي | **PASS/PARTIAL** | JSON صالح وmetadata للإطارات موجودة؛ بعض الموارد تبقى `partial` بحسب العينة. لا يوجد إعلان `complete:true`. |
| native runtime object metadata والتصادم | **PASS جزئيًا** | أضيفت solid/visible/persistent/depth/mask وrollback وsnapshot؛ ما زال runtime subset وليس توافق GM8.2 كاملًا. |
| Metal Slug عبر المسار العام | **PARTIAL/FAIL** | تحسن فك payload وظهور sprites، لكن أحداث GMK v400 والخلفيات والحركة الكاملة لم تثبت بعد عبر التسجيل. |
| تصدير/إعادة فتح شامل GMK/GM82/GMX/GMZ | **PARTIAL** | توجد بوابات ومسارات أولية، لكن لا توجد بعد مطابقة round-trip شاملة لكل الموارد والأحداث والأفعال. |
| اختبار جهاز Android فعلي | **UNVERIFIED** | لا يوجد جهاز أو emulator/Waydroid متصل في بيئة العمل الحالية. |
| تفوق الأداء على Windows | **UNVERIFIED** | لا توجد benchmark مشتركة قابلة للمقارنة، لذلك لا يصح ادعاء السرعة أو القوة. |

## الملفات التي يجب استخدامها على الكمبيوتر

يحتوي الأرشيف على مصدر Android الحالي، أدوات الاختبار والتحليل، تقارير الدفعات، أحدث APK، ونسخ rollback. الملف المرجعي للتشغيل هو:

```text
android_project_base/
```

والأدلة التنفيذية الأهم هي:

```text
android_project_base/app/src/main/assets/www/index.html
android_project_base/app/src/main/cpp/gm82_android_core.c
android_project_base/app/src/main/cpp/gm82_gmk_reader.c
android_project_base/app/src/main/cpp/gml_vm.c
android_project_base/app/src/main/cpp/CMakeLists.txt
android_project_base/app/src/main/java/com/normaker/nativefull/MainActivity.java
tools/test_real_gm82_runtime_execution.js
tools/test_generated_runtime.js
tools/test_native_project_ir.js
tools/test_native_js_parity.js
tools/test_gmk8_end_to_end_transaction.js
NATIVE_RUNTIME_BATCH_20260828_REPORT_AR.md
CORE_ARCHITECTURE_AUDIT_CURRENT.md
```

## متطلبات التشغيل على الكمبيوتر

يلزم استخدام Linux أو WSL أو بيئة Android Studio مناسبة، مع Java وAndroid SDK وNDK وCMake وGradle. المشروع يحتوي على Gradle محلي تحت `android_project_base/.tools/gradle-8.2.1/bin/gradle` في حال بقي هذا المسار متاحًا بعد فك الضغط. يلزم ضبط `ANDROID_HOME` و`ANDROID_SDK_ROOT`، ثم تشغيل الاختبارات من جذر المشروع:

```bash
cd nor_maker_0_1
node tools/test_generated_runtime.js
node tools/test_real_gm82_runtime_execution.js
node tools/test_native_project_ir.js
node tools/test_native_js_parity.js
node tools/test_gmk8_end_to_end_transaction.js
```

لبناء APK من المصدر الحالي:

```bash
cd android_project_base
.tools/gradle-8.2.1/bin/gradle --offline assembleDebug
```

إذا لم تتوفر dependencies في الكاش المحلي، استخدم Gradle/Android Studio مع اتصال إنترنت، ولا تستبدل `index.html` بملف مستخرج من APK قديم. بعد البناء يجب توثيق SHA-256، والتحقق من تطابق الملف المضمن:

```bash
unzip -p app/build/outputs/apk/debug/app-debug.apk assets/www/index.html | cmp - android_project_base/app/src/main/assets/www/index.html
sha256sum app/build/outputs/apk/debug/app-debug.apk
```

## ما ينقصنا للوصول إلى Core أوسع وتوافق عملي

الفجوة الأكبر هي توحيد semantics بين parser وProject IR وRunner وexporter. المطلوب التالي هو استكمال فك الخلفيات وPaths وTimelines وObjects وRooms بصورة دلالية، ثم ربط Create/Step/Keyboard/Collision والأفعال بمراجع الموارد، وإضافة round-trip gates حقيقية لـGMK وGM82 وGMX وGMZ.

بعد ذلك يلزم بناء differential harness يقارن world state بين Runner JavaScript وnative runtime على نفس corpus، ثم توسيع مكتبة GML والـcollision والـroom lifecycle قبل التفكير في جعل native runtime هو المسار المرئي الافتراضي. يجب كذلك إضافة اختبار Android فعلي على جهاز أو emulator، واختبار أداء مشترك قبل أي ادعاء تفوق على Windows.

> لا ينبغي وصف المشروع حاليًا بأنه Core GM8.2 مكتمل 100%، ولا وصف فيديو Chromium/Xvfb بأنه فيديو Android. التصنيف الصحيح هو **تقدم قابل للقياس مع فجوات معروفة**.

## سياسة الرجوع

قبل أي تعديل جديد، انسخ النسخة الحالية أو استخدم مجلدي rollback المرفقين. `baseline_core_batch_20260828T050220Z` يمثل baseline لخطة Core، بينما `engine_runner_rollback_20260828T082657Z` يمثل rollback لمسار Runner، و`rollback_native_runtime_20260828T052826Z` يمثل rollback لدفعة native runtime. لا تعدل baseline نفسها.

## ملاحظات الحزمة

تم استبعاد مخرجات البناء والكاش والملفات المؤقتة الكبيرة من نسخة المصدر المنقولة لتقليل الحجم. أُرفق أحدث APK منفصلًا داخل الحزمة، ويمكن إعادة بنائه من المصدر. عينات GMK الكبيرة الموجودة داخل `build_test` أُبقيت خارج الأرشيف لتجنب تضخيم الحزمة؛ يمكن نسخها لاحقًا إذا أراد المستخدم نقل corpus الاختبار الكامل.

## References

لا تعتمد هذه الوثيقة على مصادر ويب خارجية؛ جميع التصنيفات مبنية على ملفات الاختبار والتقارير المحلية المرفقة داخل الحزمة.

**المؤلف:** Manus AI

**آخر تحديث:** 29 أغسطس 2026.
