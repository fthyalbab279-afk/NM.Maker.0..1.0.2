#define _POSIX_C_SOURCE 200809L
/*
 * JNI bridge matching Gm82Native.java
 * Compile with NDK + -I$JAVA_HOME/include when building libgm82_android.so
 *
 * Without jni.h in this environment, we provide a portable shim so the file
 * still documents the exact entry points. Real build defines GM82_HAVE_JNI.
 */
#include "gm82_jni.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>

#ifdef GM82_HAVE_JNI
#include <jni.h>

static char *jstring_to_utf8(JNIEnv *env, jstring js) {
    if (!js) return NULL;
    const char *s = (*env)->GetStringUTFChars(env, js, NULL);
    if (!s) return NULL;
    char *out = strdup(s);
    (*env)->ReleaseStringUTFChars(env, js, s);
    return out;
}

JNIEXPORT jboolean JNICALL
Java_com_normaker_gm82_Gm82Native_nativeInit(JNIEnv *env, jclass cls, jint w, jint h) {
    (void)env; (void)cls;
    return gm82_native_init((int)w, (int)h) ? JNI_TRUE : JNI_FALSE;
}

JNIEXPORT void JNICALL
Java_com_normaker_gm82_Gm82Native_nativeShutdown(JNIEnv *env, jclass cls) {
    (void)env; (void)cls;
    gm82_native_shutdown();
}

JNIEXPORT jboolean JNICALL
Java_com_normaker_gm82_Gm82Native_nativeLoadGame(JNIEnv *env, jclass cls, jstring path) {
    (void)cls;
    char *p = jstring_to_utf8(env, path);
    char err[256];
    jboolean ok = JNI_FALSE;
    if (p) {
        ok = gm82_native_load_game(p, err, sizeof(err)) ? JNI_TRUE : JNI_FALSE;
        free(p);
    }
    return ok;
}

JNIEXPORT void JNICALL
Java_com_normaker_gm82_Gm82Native_nativeResize(JNIEnv *env, jclass cls, jint w, jint h) {
    (void)env; (void)cls;
    gm82_native_resize((int)w, (int)h);
}

JNIEXPORT void JNICALL
Java_com_normaker_gm82_Gm82Native_nativeStep(JNIEnv *env, jclass cls) {
    (void)env; (void)cls;
    gm82_native_step();
}

JNIEXPORT void JNICALL
Java_com_normaker_gm82_Gm82Native_nativeDraw(JNIEnv *env, jclass cls) {
    (void)env; (void)cls;
    gm82_native_draw(); /* STUB until GLES */
}

JNIEXPORT void JNICALL
Java_com_normaker_gm82_Gm82Native_nativeKeyDown(JNIEnv *env, jclass cls, jint vk) {
    (void)env; (void)cls;
    gm82_native_key_down((int)vk);
}

JNIEXPORT void JNICALL
Java_com_normaker_gm82_Gm82Native_nativeKeyUp(JNIEnv *env, jclass cls, jint vk) {
    (void)env; (void)cls;
    gm82_native_key_up((int)vk);
}

JNIEXPORT void JNICALL
Java_com_normaker_gm82_Gm82Native_nativeTouch(JNIEnv *env, jclass cls, jint x, jint y, jint action) {
    (void)env; (void)cls;
    gm82_native_touch((int)x, (int)y, (int)action);
}

JNIEXPORT jboolean JNICALL
Java_com_normaker_gm82_Gm82Native_nativeIsRunning(JNIEnv *env, jclass cls) {
    (void)env; (void)cls;
    return gm82_native_is_running() ? JNI_TRUE : JNI_FALSE;
}

JNIEXPORT jint JNICALL
Java_com_normaker_gm82_Gm82Native_nativeRoomWidth(JNIEnv *env, jclass cls) {
    (void)env; (void)cls;
    return (jint)gm82_native_room_width();
}

JNIEXPORT jint JNICALL
Java_com_normaker_gm82_Gm82Native_nativeRoomHeight(JNIEnv *env, jclass cls) {
    (void)env; (void)cls;
    return (jint)gm82_native_room_height();
}

#endif /* GM82_HAVE_JNI */
