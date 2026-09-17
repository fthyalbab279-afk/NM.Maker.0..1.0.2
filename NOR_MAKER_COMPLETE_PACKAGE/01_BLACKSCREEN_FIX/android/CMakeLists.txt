# NDK CMake for libgm82_android.so – requires Android SDK/NDK to build
cmake_minimum_required(VERSION 3.18)
project(gm82_android)

add_library(gm82_android SHARED
  gm82_jni.c
  gm82_jni_java_bridge.c
  gm82_gl_textures.c
  ../src/gm82_runtime.c
  ../src/gm82_events.c
  ../src/gm82_gml_builtins.c
  ../src/gm82_input.c
  ../src/gm82_view.c
  ../src/gm82_actions.c
  ../src/gm82_object_room_decode.c
  ../src/gm82_sprite_decode.c
  ../src/gm82_background_decode.c
  ../src/gm82_project_ir.c
  ../src/gm82_materialize.c
  ../src/gm82_runtime_guard.c
  ../src/gm82_loader.c
  ../src/gm82_gmk_reader.c
  ../src/gm82_gmk_format.c
  ../src/gm82_text_reader.c
  ../src/gm82_game_start.c
)

target_include_directories(gm82_android PRIVATE ../include ${CMAKE_CURRENT_SOURCE_DIR})
target_compile_definitions(gm82_android PRIVATE GM82_HAVE_JNI)
find_library(log-lib log)
find_library(z-lib z)
target_link_libraries(gm82_android ${log-lib} ${z-lib} GLESv2 EGL android)
