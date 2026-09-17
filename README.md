# NOR Maker (صانع الألعاب) for Android

NOR Maker is a full-featured retro game creation studio and GameMaker 8.2 IDE ported to Android.

## Features

- **Visual Game Creation Studio**: Build 2D/retro games with sprites, backgrounds, sound effects, fonts, objects, and scripts.
- **Level & Room Editor**: Visual tile and instance placement, collision testing, grid snapping, and view configuration.
- **Sprite & Pixel Art Editor**: Custom pixel drawing tools, animations, frame management, and color palettes.
- **Script & Action System**: GML / GameMaker-compatible scripting and drag-and-drop actions for movement, physics, and gameplay logic.
- **Native Game Engine**: C/C++ native runtime executing GML VM, collision detection, and SoundPool audio synthesis.
- **Retro Theme & MDI Windows**: Retro styling, customizable skins, CRT scanline shader, and Arabic / English interface support.
- **Export & Import**: Support for GMK, GMX, GMZ, NOR package, ROM export, and project templates.

## Architecture

- **Platform**: Android SDK 34+
- **Native Layer**: C/C++ JNI runtime (`gm82_android`) with GML interpreter and GMK parser
- **UI & Studio**: Multi-document interface (MDI) with touch optimization, virtual gamepad support, and sound engine integration
