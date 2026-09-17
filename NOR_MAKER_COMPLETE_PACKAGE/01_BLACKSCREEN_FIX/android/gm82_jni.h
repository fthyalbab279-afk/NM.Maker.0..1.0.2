#ifndef GM82_JNI_H
#define GM82_JNI_H

/*
 * Android JNI bridge – scaffold only.
 * Real .so build requires NDK; this header defines the C API the Java side will call.
 *
 * NOT claimed as integrated into the APK yet.
 */

#include <stdint.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

/* Lifecycle */
bool gm82_native_init(int surface_width, int surface_height);
void gm82_native_shutdown(void);

/* Load a game from absolute path (.gmk / .gm82) */
bool gm82_native_load_game(const char *path, char *err, int err_len);

/* Frame */
void gm82_native_resize(int w, int h);
void gm82_native_step(void);          /* one game frame */
void gm82_native_draw(void);          /* render to current GL context – STUB until GL wired */

/* Input from Java */
void gm82_native_key_down(int vk);
void gm82_native_key_up(int vk);
void gm82_native_touch(int x, int y, int action); /* 0=down 1=up 2=move */

/* Query */
int  gm82_native_is_running(void);
int  gm82_native_room_width(void);
int  gm82_native_room_height(void);

#ifdef __cplusplus
}
#endif

#endif
