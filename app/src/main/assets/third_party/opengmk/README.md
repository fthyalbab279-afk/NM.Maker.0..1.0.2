# Third-party Beta Components

هذا المجلد يحتوي نسخة مراجعة من OpenGMK لاستخدامها كمكوّن تجريبي ومرجع parser/runtime في NOR Maker Beta.

## OpenGMK

Source: https://github.com/OpenGMK/OpenGMK
Pinned review commit: `ca910f100ab7f3719a7b57c4ea5f0f3f74f20031`
License: GNU GPL v2.0, see `opengmk-review/LICENCE.md`.

تم إبقاء المصدر منفصلاً عن C11/Java الحالي في هذه المرحلة حتى لا يتم استبدال أو حذف أي من ميزات NOR Maker الأصلية. المسارات المرشحة للمراجعة هي `gm8exe/src/gamedata` و`gm8emulator/src/gml` و`gml-parser`. أي دمج ثنائي لاحق يجب أن يحافظ على notices ونسخة المصدر وشروط GPL، وأن يمر باختبارات عدم فقد الميزات.

هذا المجلد ليس ادعاءً بأن OpenGMK أصبح runtime Android مدمجاً بالكامل. الدمج الفعلي يجب أن يتم خلف طبقة توافق NOR Maker وبخطوات قابلة للرجوع.
