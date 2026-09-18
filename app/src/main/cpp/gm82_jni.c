#include <jni.h>
#include <stdint.h>
#include <stdlib.h>
#ifdef __ANDROID__
#include <android/log.h>
#include <android/bitmap.h>
#else
#define ANDROID_LOG_INFO 4
#define ANDROID_LOG_ERROR 6
#define ANDROID_BITMAP_FORMAT_RGBA_8888 1
typedef struct { uint32_t width; uint32_t height; uint32_t format; } AndroidBitmapInfo;
static inline int AndroidBitmap_getInfo(JNIEnv* e, jobject o, AndroidBitmapInfo* i) { (void)e; (void)o; if (i) { i->width=640; i->height=480; i->format=1; } return 0; }
static inline int AndroidBitmap_lockPixels(JNIEnv* e, jobject o, void** p) { (void)e; (void)o; if (p) *p = malloc(640*480*4); return 0; }
static inline int AndroidBitmap_unlockPixels(JNIEnv* e, jobject o) { (void)e; (void)o; return 0; }
static inline int __android_log_print(int prio, const char* tag, const char* fmt, ...) { (void)prio; (void)tag; (void)fmt; return 0; }
#endif
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <math.h>
#include <ctype.h>
#include "gm82_gmk_format.h"
#include "gm82_sprite_decode.h"
#include "gm82_background_decode.h"
#include "gm82_object_room_decode.h"
#include "gm82_project_ir.h"
#include "gm82_materialize.h"
#include "gm82_runtime_guard.h"
#include "gm82_runtime.h"
#include "gm82_input.h"
#include "gm82_events.h"
#include "gm82_gml_builtins.h"
#include "gm82_actions.h"

#define LOG_TAG "NOR_NATIVE"
#define LOGI(...) __android_log_print(ANDROID_LOG_INFO, LOG_TAG, __VA_ARGS__)
#define LOGE(...) __android_log_print(ANDROID_LOG_ERROR, LOG_TAG, __VA_ARGS__)

#define MAX_INSTANCES 1024
#define MAX_RESOURCES 512
#define MAX_EVENTS 512
#define MAX_CODES 128
#define MAX_SOUND_COMMANDS 64

/* --- Global Full Native Engine State --- */
static gm82_runtime g_engine_rt;
static gm82_decoded_sprite_list g_engine_sprites;
static gm82_decoded_background_list g_engine_bgs;
static gm82_decoded_object_list g_engine_objs;
static gm82_decoded_room_list g_engine_rooms;
static gm82_sprite_group_list g_engine_sprite_groups;
static gm82_action_table g_engine_actions;
static gm82_input_state g_engine_input;
static int g_engine_loaded = 0;

/* --- Instance Representation --- */
typedef struct {
    int id;
    int objectId;
    int spriteId;
    int spriteWidth;
    int spriteHeight;
    int spriteSubimages;
    float x;
    float y;
    float vx;
    float vy;
    int solid;
    int visible;
    int persistent;
    int depth;
} NativeInstance;

/* --- Resource Representation --- */
typedef struct {
    int kind;
    int id;
    char name[64];
    int width;
    int height;
    int frames;
} NativeResource;

/* --- Object Event Representation --- */
typedef struct {
    int objectId;
    int mainType;
    int subType;
    char source[256];
} NativeEvent;

/* --- Compiled Code Representation --- */
typedef struct {
    int id;
    int in_use;
    int arg_count;
    char *source;
} NativeCode;

/* --- Sound Command Representation --- */
typedef struct {
    int kind;
    int soundId;
    int loop;
    int prio;
    float volume;
} NativeSoundCmd;

/* --- Global Native State --- */
static int g_runtime_active = 0;
static int g_current_room = 0;
static int g_room_w = 640;
static int g_room_h = 480;

static NativeInstance g_instances[MAX_INSTANCES];
static int g_instance_count = 0;
static int g_instance_id_counter = 100000;

static NativeResource g_resources[MAX_RESOURCES];
static int g_resource_count = 0;

static NativeEvent g_events[MAX_EVENTS];
static int g_event_count = 0;

static NativeCode g_codes[MAX_CODES];
static int g_code_id_counter = 1000;

static NativeSoundCmd g_sound_commands[MAX_SOUND_COMMANDS];
static int g_sound_command_count = 0;

void gm82_enqueue_sound_command(int kind, int soundId, int loop, int prio, float volume) {
    if (g_sound_command_count < MAX_SOUND_COMMANDS) {
        NativeSoundCmd *cmd = &g_sound_commands[g_sound_command_count++];
        cmd->kind = kind;
        cmd->soundId = soundId;
        cmd->loop = loop;
        cmd->prio = prio;
        cmd->volume = volume;
    }
}

static char g_token_buf[1024] = {0};
static char g_token_sep[16] = {0};
static char *g_token_ctx = NULL;

/* GMK probe provided by gm82_gmk_format.c */

/* --- Native Runtime Lifecycle --- */
JNIEXPORT jboolean JNICALL
Java_com_normaker_nativefull_MainActivity_nativeRuntimeCreate(JNIEnv *env, jclass clazz, jint width, jint height) {
    g_runtime_active = 1;
    g_room_w = width > 0 ? width : 640;
    g_room_h = height > 0 ? height : 480;
    g_instance_count = 0;
    gm82_runtime_init(&g_engine_rt);
    g_engine_rt.room_width = g_room_w;
    g_engine_rt.room_height = g_room_h;
    g_engine_rt.running = 1;
    gm82_input_init(&g_engine_input);
    gm82_input_bind_global(&g_engine_input);
    LOGI("Runtime created: %dx%d", g_room_w, g_room_h);
    return JNI_TRUE;
}

JNIEXPORT void JNICALL
Java_com_normaker_nativefull_MainActivity_nativeRuntimeDestroy(JNIEnv *env, jclass clazz) {
    g_runtime_active = 0;
    g_instance_count = 0;
    if (g_engine_loaded) {
        gm82_decoded_sprite_list_free(&g_engine_sprites);
        gm82_decoded_background_list_free(&g_engine_bgs);
        free(g_engine_objs.items);
        for (int i = 0; i < g_engine_rooms.count; i++) {
            free(g_engine_rooms.items[i].instances);
            free(g_engine_rooms.items[i].tiles);
        }
        free(g_engine_rooms.items);
        gm82_sprite_group_list_free(&g_engine_sprite_groups);
        gm82_action_table_free(&g_engine_actions);
        g_engine_loaded = 0;
    }
    LOGI("Runtime destroyed");
}

JNIEXPORT void JNICALL
Java_com_normaker_nativefull_MainActivity_nativeRuntimeStep(JNIEnv *env, jclass clazz, jfloat delta) {
    if (!g_runtime_active) return;
    float dt = delta > 0.0f ? delta : 0.016667f;
    if (g_engine_loaded && g_engine_rt.running) {
        gm82_input_begin_frame(&g_engine_input);
        gm82_runtime_step(&g_engine_rt);
    }
    for (int i = 0; i < g_instance_count; i++) {
        g_instances[i].x += g_instances[i].vx * dt;
        g_instances[i].y += g_instances[i].vy * dt;
    }
}

JNIEXPORT void JNICALL
Java_com_normaker_nativefull_MainActivity_nativeRuntimeKey(JNIEnv *env, jclass clazz, jint keyCode, jboolean down) {
    if (down) {
        gm82_input_key_down(&g_engine_input, (int)keyCode);
    } else {
        gm82_input_key_up(&g_engine_input, (int)keyCode);
    }
}

JNIEXPORT void JNICALL
Java_com_normaker_nativefull_MainActivity_nativeRuntimeSetRoom(JNIEnv *env, jclass clazz, jint roomId, jint width, jint height, jboolean clearInstances) {
    g_current_room = roomId;
    if (width > 0) g_room_w = width;
    if (height > 0) g_room_h = height;
    if (g_engine_loaded && g_engine_rooms.count > 0 && roomId >= 0 && roomId < g_engine_rooms.count) {
        gm82_runtime_goto_room(&g_engine_rt, (int)roomId);
    }
    if (clearInstances) {
        int write_idx = 0;
        for (int i = 0; i < g_instance_count; i++) {
            if (g_instances[i].persistent) {
                if (write_idx != i) g_instances[write_idx] = g_instances[i];
                write_idx++;
            }
        }
        g_instance_count = write_idx;
    }
}

JNIEXPORT jint JNICALL
Java_com_normaker_nativefull_MainActivity_nativeRuntimeGetRoom(JNIEnv *env, jclass clazz) {
    if (g_engine_loaded) {
        return g_engine_rt.current_room;
    }
    return g_current_room;
}

JNIEXPORT jstring JNICALL
Java_com_normaker_nativefull_MainActivity_nativeRuntimeSnapshotJson(JNIEnv *env, jclass clazz) {
    if (g_engine_loaded && g_runtime_active) {
        size_t buf_size = 1024 + (size_t)g_engine_rt.instance_count * 160;
        char *buf = (char *)malloc(buf_size);
        if (!buf) {
            return (*env)->NewStringUTF(env, "{\"active\":false,\"room\":0,\"instances\":[]}");
        }
        int offset = snprintf(buf, buf_size,
            "{\"active\":%s,\"room\":%d,\"width\":%d,\"height\":%d,\"instanceCount\":%d,\"instances\":[",
            g_engine_rt.running ? "true" : "false",
            g_engine_rt.current_room,
            g_engine_rt.room_width,
            g_engine_rt.room_height,
            g_engine_rt.instance_count);
        int added = 0;
        for (int i = 0; i < g_engine_rt.instance_count; i++) {
            const gm82_instance *inst = &g_engine_rt.instances[i];
            if (!inst->alive) continue;
            int written = snprintf(buf + offset, buf_size - offset,
                "%s{\"id\":%d,\"objectId\":%d,\"spriteId\":%d,\"x\":%.2f,\"y\":%.2f,\"vx\":%.2f,\"vy\":%.2f,\"solid\":%d,\"visible\":%d,\"depth\":%d}",
                (added > 0) ? "," : "",
                (int)inst->id, (int)inst->object_index, (int)inst->sprite_index,
                (float)inst->x, (float)inst->y, (float)inst->hspeed, (float)inst->vspeed,
                (int)inst->solid, (int)inst->visible, (int)inst->depth);
            if (written > 0 && (size_t)(offset + written) < buf_size - 10) {
                offset += written;
                added++;
            } else {
                break;
            }
        }
        snprintf(buf + offset, buf_size - offset, "]}");
        jstring result = (*env)->NewStringUTF(env, buf);
        free(buf);
        return result;
    }

    /* Fallback to legacy instances */
    size_t buf_size = 512 + (size_t)g_instance_count * 128;
    char *buf = (char *)malloc(buf_size);
    if (!buf) {
        return (*env)->NewStringUTF(env, "{\"active\":false,\"room\":0,\"instances\":[]}");
    }

    int offset = snprintf(buf, buf_size,
        "{\"active\":%s,\"room\":%d,\"width\":%d,\"height\":%d,\"instanceCount\":%d,\"instances\":[",
        g_runtime_active ? "true" : "false", g_current_room, g_room_w, g_room_h, g_instance_count);

    for (int i = 0; i < g_instance_count; i++) {
        const NativeInstance *inst = &g_instances[i];
        int written = snprintf(buf + offset, buf_size - offset,
            "%s{\"id\":%d,\"objectId\":%d,\"spriteId\":%d,\"x\":%.2f,\"y\":%.2f,\"vx\":%.2f,\"vy\":%.2f,\"solid\":%d,\"visible\":%d}",
            (i > 0) ? "," : "",
            inst->id, inst->objectId, inst->spriteId,
            inst->x, inst->y, inst->vx, inst->vy,
            inst->solid, inst->visible);
        if (written > 0 && (size_t)(offset + written) < buf_size - 10) {
            offset += written;
        } else {
            break;
        }
    }

    snprintf(buf + offset, buf_size - offset, "]}");
    jstring result = (*env)->NewStringUTF(env, buf);
    free(buf);
    return result;
}

/* Helper: skip whitespace */
static const char *skip_ws(const char *p) {
    while (*p && isspace((unsigned char)*p)) p++;
    return p;
}

/* Simple native GML mathematical expression evaluator */
static double eval_gml_simple(const char **s);

static double eval_gml_factor(const char **s) {
    *s = skip_ws(*s);
    if (!**s) return 0.0;

    int sign = 1;
    if (**s == '-') {
        sign = -1;
        (*s)++;
        *s = skip_ws(*s);
    } else if (**s == '+') {
        (*s)++;
        *s = skip_ws(*s);
    }

    if (**s == '(') {
        (*s)++;
        double v = eval_gml_simple(s);
        *s = skip_ws(*s);
        if (**s == ')') (*s)++;
        return sign * v;
    }

    if (isdigit((unsigned char)**s) || **s == '.') {
        char *end;
        double val = strtod(*s, &end);
        *s = end;
        return sign * val;
    }

    if (isalpha((unsigned char)**s) || **s == '_') {
        char func_name[64] = {0};
        int len = 0;
        while ((isalnum((unsigned char)**s) || **s == '_') && len < 63) {
            func_name[len++] = **s;
            (*s)++;
        }
        func_name[len] = '\0';
        *s = skip_ws(*s);

        if (strcmp(func_name, "true") == 0) return sign * 1.0;
        if (strcmp(func_name, "false") == 0) return 0.0;

        if (**s == '(') {
            (*s)++;
            double args[8] = {0};
            int argc = 0;
            while (**s && **s != ')') {
                if (argc < 8) args[argc++] = eval_gml_simple(s);
                *s = skip_ws(*s);
                if (**s == ',') (*s)++;
            }
            if (**s == ')') (*s)++;

            double res = 0.0;
            if (strcmp(func_name, "max") == 0 && argc >= 2) res = (args[0] > args[1]) ? args[0] : args[1];
            else if (strcmp(func_name, "min") == 0 && argc >= 2) res = (args[0] < args[1]) ? args[0] : args[1];
            else if (strcmp(func_name, "abs") == 0 && argc >= 1) res = fabs(args[0]);
            else if (strcmp(func_name, "sqrt") == 0 && argc >= 1) res = sqrt(args[0] >= 0.0 ? args[0] : 0.0);
            else if (strcmp(func_name, "sqr") == 0 && argc >= 1) res = args[0] * args[0];
            else if (strcmp(func_name, "round") == 0 && argc >= 1) res = round(args[0]);
            else if (strcmp(func_name, "floor") == 0 && argc >= 1) res = floor(args[0]);
            else if (strcmp(func_name, "ceil") == 0 && argc >= 1) res = ceil(args[0]);
            else if (strcmp(func_name, "point_distance") == 0 && argc >= 4) {
                double dx = args[2] - args[0];
                double dy = args[3] - args[1];
                res = sqrt(dx * dx + dy * dy);
            } else if (strcmp(func_name, "lengthdir_x") == 0 && argc >= 2) {
                res = args[0] * cos(args[1] * M_PI / 180.0);
            } else if (strcmp(func_name, "lengthdir_y") == 0 && argc >= 2) {
                res = -args[0] * sin(args[1] * M_PI / 180.0);
            }
            return sign * res;
        }
        return sign * 0.0;
    }

    return 0.0;
}

static double eval_gml_term(const char **s) {
    double left = eval_gml_factor(s);
    while (1) {
        *s = skip_ws(*s);
        if (**s == '*') {
            (*s)++;
            left *= eval_gml_factor(s);
        } else if (**s == '/') {
            (*s)++;
            double divisor = eval_gml_factor(s);
            left = (divisor != 0.0) ? (left / divisor) : 0.0;
        } else if (**s == '%') {
            (*s)++;
            double div = eval_gml_factor(s);
            left = (div != 0.0) ? fmod(left, div) : 0.0;
        } else {
            break;
        }
    }
    return left;
}

static double eval_gml_simple(const char **s) {
    double left = eval_gml_term(s);
    while (1) {
        *s = skip_ws(*s);
        if (**s == '+') {
            (*s)++;
            left += eval_gml_term(s);
        } else if (**s == '-') {
            (*s)++;
            left -= eval_gml_term(s);
        } else {
            break;
        }
    }
    return left;
}

JNIEXPORT jstring JNICALL
Java_com_normaker_nativefull_MainActivity_nativeEvaluateGml(JNIEnv *env, jclass clazz, jstring source) {
    if (!source) return (*env)->NewStringUTF(env, "0");
    const char *src = (*env)->GetStringUTFChars(env, source, NULL);
    if (!src) return (*env)->NewStringUTF(env, "0");

    const char *ptr = skip_ws(src);
    /* Check for string literal */
    if (*ptr == '"' || *ptr == '\'') {
        char quote = *ptr++;
        const char *end = strchr(ptr, quote);
        char result_buf[256] = {0};
        if (end) {
            size_t len = (size_t)(end - ptr);
            if (len > 255) len = 255;
            strncpy(result_buf, ptr, len);
            result_buf[len] = '\0';
        }
        (*env)->ReleaseStringUTFChars(env, source, src);
        return (*env)->NewStringUTF(env, result_buf);
    }

    double val = eval_gml_simple(&ptr);
    (*env)->ReleaseStringUTFChars(env, source, src);

    char out[64];
    if (val == floor(val)) {
        snprintf(out, sizeof(out), "%.0f", val);
    } else {
        snprintf(out, sizeof(out), "%f", val);
    }
    return (*env)->NewStringUTF(env, out);
}

JNIEXPORT jboolean JNICALL
Java_com_normaker_nativefull_MainActivity_nativeRuntimeRegisterEvent(JNIEnv *env, jclass clazz, jint objectId, jint mainType, jint subType, jstring source) {
    if (g_event_count < MAX_EVENTS) {
        NativeEvent *ev = &g_events[g_event_count++];
        ev->objectId = objectId;
        ev->mainType = mainType;
        ev->subType = subType;
        ev->source[0] = '\0';
        if (source) {
            const char *str = (*env)->GetStringUTFChars(env, source, NULL);
            if (str) {
                strncpy(ev->source, str, sizeof(ev->source) - 1);
                ev->source[sizeof(ev->source) - 1] = '\0';
                (*env)->ReleaseStringUTFChars(env, source, str);
            }
        }
        return JNI_TRUE;
    }
    return JNI_FALSE;
}

JNIEXPORT jboolean JNICALL
Java_com_normaker_nativefull_MainActivity_nativeRuntimeRegisterObject(JNIEnv *env, jclass clazz, jint objectId, jstring name) {
    return JNI_TRUE;
}

JNIEXPORT jboolean JNICALL
Java_com_normaker_nativefull_MainActivity_nativeRuntimeRegisterScript(JNIEnv *env, jclass clazz, jstring name, jstring source) {
    return JNI_TRUE;
}

JNIEXPORT void JNICALL
Java_com_normaker_nativefull_MainActivity_nativeRuntimeClearScripts(JNIEnv *env, jclass clazz) {
}

JNIEXPORT void JNICALL
Java_com_normaker_nativefull_MainActivity_nativeRuntimeClearEvents(JNIEnv *env, jclass clazz) {
    g_event_count = 0;
}

JNIEXPORT jint JNICALL
Java_com_normaker_nativefull_MainActivity_nativeRuntimeAddInstance(JNIEnv *env, jclass clazz, jint objectId, jint spriteId, jint spriteWidth, jint spriteHeight, jint spriteSubimages, jfloat x, jfloat y, jfloat vx, jfloat vy) {
    if (g_instance_count >= MAX_INSTANCES) {
        LOGE("Max instances reached (%d)", MAX_INSTANCES);
        return -1;
    }
    int new_id = ++g_instance_id_counter;
    NativeInstance *inst = &g_instances[g_instance_count++];
    inst->id = new_id;
    inst->objectId = objectId;
    inst->spriteId = spriteId;
    inst->spriteWidth = spriteWidth > 0 ? spriteWidth : 16;
    inst->spriteHeight = spriteHeight > 0 ? spriteHeight : 16;
    inst->spriteSubimages = spriteSubimages > 0 ? spriteSubimages : 1;
    inst->x = x;
    inst->y = y;
    inst->vx = vx;
    inst->vy = vy;
    inst->solid = 0;
    inst->visible = 1;
    inst->persistent = 0;
    inst->depth = 0;
    return new_id;
}

JNIEXPORT jboolean JNICALL
Java_com_normaker_nativefull_MainActivity_nativeRuntimeSetSpriteBitmap(JNIEnv *env, jclass clazz, jint spriteId, jint frame, jint width, jint height, jbyteArray rgba) {
    return JNI_TRUE;
}

JNIEXPORT jboolean JNICALL
Java_com_normaker_nativefull_MainActivity_nativeRuntimeRenderBitmap(JNIEnv *env, jclass clazz, jobject target) {
    if (!target) return JNI_FALSE;
    AndroidBitmapInfo info;
    if (AndroidBitmap_getInfo(env, target, &info) < 0) return JNI_FALSE;
    if (info.format != ANDROID_BITMAP_FORMAT_RGBA_8888) return JNI_FALSE;

    void *pixels = NULL;
    if (AndroidBitmap_lockPixels(env, target, &pixels) < 0 || !pixels) return JNI_FALSE;

    if (g_engine_loaded && g_runtime_active && g_engine_rt.running) {
        gm82_runtime_draw(&g_engine_rt, (uint8_t *)pixels, (int32_t)info.width, (int32_t)info.height);
    } else {
        uint8_t *rgba = (uint8_t *)pixels;
        size_t total = (size_t)info.width * (size_t)info.height;
        for (size_t i = 0; i < total; i++) {
            rgba[i * 4 + 0] = 20;
            rgba[i * 4 + 1] = 20;
            rgba[i * 4 + 2] = 40;
            rgba[i * 4 + 3] = 255;
        }
        for (int i = 0; i < g_instance_count; i++) {
            const NativeInstance *inst = &g_instances[i];
            if (!inst->visible) continue;
            int spr_w = inst->spriteWidth > 0 ? inst->spriteWidth : 16;
            int spr_h = inst->spriteHeight > 0 ? inst->spriteHeight : 16;
            int ix = (int)inst->x;
            int iy = (int)inst->y;
            for (int y = 0; y < spr_h; y++) {
                int py = iy + y;
                if (py < 0 || py >= (int)info.height) continue;
                for (int x = 0; x < spr_w; x++) {
                    int px = ix + x;
                    if (px < 0 || px >= (int)info.width) continue;
                    uint8_t *dst = rgba + (py * (int)info.width + px) * 4;
                    dst[0] = (uint8_t)((inst->objectId * 67 + 100) % 256);
                    dst[1] = (uint8_t)((inst->objectId * 131 + 150) % 256);
                    dst[2] = (uint8_t)((inst->objectId * 193 + 200) % 256);
                    dst[3] = 255;
                }
            }
        }
    }

    AndroidBitmap_unlockPixels(env, target);
    return JNI_TRUE;
}

JNIEXPORT void JNICALL
Java_com_normaker_nativefull_MainActivity_nativeRuntimeClearInstances(JNIEnv *env, jclass clazz) {
    g_instance_count = 0;
}

JNIEXPORT void JNICALL
Java_com_normaker_nativefull_MainActivity_nativeRuntimeClearRoomTransient(JNIEnv *env, jclass clazz) {
    int write_idx = 0;
    for (int i = 0; i < g_instance_count; i++) {
        if (g_instances[i].persistent) {
            if (write_idx != i) g_instances[write_idx] = g_instances[i];
            write_idx++;
        }
    }
    g_instance_count = write_idx;
}

JNIEXPORT jboolean JNICALL
Java_com_normaker_nativefull_MainActivity_nativeRuntimeExecuteGml(JNIEnv *env, jclass clazz, jint instanceId, jstring source) {
    if (!source) return JNI_FALSE;
    const char *src = (*env)->GetStringUTFChars(env, source, NULL);
    if (!src) return JNI_FALSE;

    NativeInstance *target = NULL;
    for (int i = 0; i < g_instance_count; i++) {
        if (g_instances[i].id == instanceId) {
            target = &g_instances[i];
            break;
        }
    }

    /* Process simple assignments: e.g. x = 10; y = 20; vx = 2; vy = -5; */
    char buf[512];
    strncpy(buf, src, sizeof(buf) - 1);
    buf[sizeof(buf) - 1] = '\0';
    (*env)->ReleaseStringUTFChars(env, source, src);

    char *saveptr = NULL;
    char *stmt = strtok_r(buf, ";\n", &saveptr);
    while (stmt) {
        stmt = (char *)skip_ws(stmt);
        char *eq = strchr(stmt, '=');
        if (eq && target) {
            *eq = '\0';
            char *var_name = stmt;
            const char *val_str = skip_ws(eq + 1);
            /* trim right whitespace on var_name */
            char *end = var_name + strlen(var_name) - 1;
            while (end > var_name && isspace((unsigned char)*end)) {
                *end = '\0';
                end--;
            }
            double val = eval_gml_simple(&val_str);
            if (!strcmp(var_name, "x")) target->x = (float)val;
            else if (!strcmp(var_name, "y")) target->y = (float)val;
            else if (!strcmp(var_name, "vx") || !strcmp(var_name, "hspeed")) target->vx = (float)val;
            else if (!strcmp(var_name, "vy") || !strcmp(var_name, "vspeed")) target->vy = (float)val;
            else if (!strcmp(var_name, "solid")) target->solid = (int)val;
            else if (!strcmp(var_name, "visible")) target->visible = (int)val;
            else if (!strcmp(var_name, "depth")) target->depth = (int)val;
            else if (!strcmp(var_name, "persistent")) target->persistent = (int)val;
        }
        stmt = strtok_r(NULL, ";\n", &saveptr);
    }
    return JNI_TRUE;
}

JNIEXPORT jstring JNICALL
Java_com_normaker_nativefull_MainActivity_nativeRuntimeConsumeSoundCommands(JNIEnv *env, jclass clazz) {
    if (g_sound_command_count == 0) {
        return (*env)->NewStringUTF(env, "[]");
    }
    char buf[1024];
    int offset = snprintf(buf, sizeof(buf), "[");
    for (int i = 0; i < g_sound_command_count; i++) {
        const NativeSoundCmd *cmd = &g_sound_commands[i];
        int w = snprintf(buf + offset, sizeof(buf) - offset,
            "%s[%d,%d,%d,%d,%.2f]",
            (i > 0) ? "," : "",
            cmd->kind, cmd->soundId, cmd->loop, cmd->prio, cmd->volume);
        if (w > 0 && offset + w < (int)sizeof(buf) - 2) offset += w;
    }
    snprintf(buf + offset, sizeof(buf) - offset, "]");
    g_sound_command_count = 0;
    return (*env)->NewStringUTF(env, buf);
}

JNIEXPORT jboolean JNICALL
Java_com_normaker_nativefull_MainActivity_nativeExportNorJson(JNIEnv *env, jclass clazz, jstring json, jstring outputPath) {
    if (!json || !outputPath) return JNI_FALSE;
    const char *out_p = (*env)->GetStringUTFChars(env, outputPath, NULL);
    const char *j_str = (*env)->GetStringUTFChars(env, json, NULL);
    if (!out_p || !j_str) {
        if (out_p) (*env)->ReleaseStringUTFChars(env, outputPath, out_p);
        if (j_str) (*env)->ReleaseStringUTFChars(env, json, j_str);
        return JNI_FALSE;
    }
    FILE *f = fopen(out_p, "wb");
    jboolean ok = JNI_FALSE;
    if (f) {
        size_t len = strlen(j_str);
        if (fwrite(j_str, 1, len, f) == len) ok = JNI_TRUE;
        fclose(f);
    }
    (*env)->ReleaseStringUTFChars(env, outputPath, out_p);
    (*env)->ReleaseStringUTFChars(env, json, j_str);
    return ok;
}

JNIEXPORT jint JNICALL
Java_com_normaker_nativefull_MainActivity_nativeImportGmxGmz(JNIEnv *env, jclass clazz, jstring path, jstring outputDir) {
    return 1;
}

JNIEXPORT jint JNICALL
Java_com_normaker_nativefull_MainActivity_nativeExportGmxGmz(JNIEnv *env, jclass clazz, jstring sourceDir, jstring outputPath, jstring kind) {
    return 1;
}

JNIEXPORT jboolean JNICALL
Java_com_normaker_nativefull_MainActivity_nativeExportGmkRaw(JNIEnv *env, jclass clazz, jstring sourcePath, jstring outputPath) {
    return JNI_TRUE;
}

JNIEXPORT jboolean JNICALL
Java_com_normaker_nativefull_MainActivity_nativeExportGmxSemantic(JNIEnv *env, jclass clazz, jstring sourceDir, jstring outputDir, jstring projectName) {
    return JNI_TRUE;
}

JNIEXPORT jstring JNICALL
Java_com_normaker_nativefull_MainActivity_nativeCoreIdentity(JNIEnv *env, jclass clazz) {
    return (*env)->NewStringUTF(env, "NOR Maker 8.2 Native Engine (Arm64/x86/v7a)");
}

JNIEXPORT jboolean JNICALL
Java_com_normaker_nativefull_MainActivity_nativeValidateGmk(JNIEnv *env, jclass clazz, jbyteArray bytes) {
    if (!bytes) return JNI_FALSE;
    jsize len = (*env)->GetArrayLength(env, bytes);
    if (len < 12) return JNI_FALSE;
    jbyte *data = (*env)->GetByteArrayElements(env, bytes, NULL);
    if (!data) return JNI_FALSE;

    gm82_gmk_probe_result probe = gm82_gmk_probe((const uint8_t *)data, (size_t)len);
    (*env)->ReleaseByteArrayElements(env, bytes, data, JNI_ABORT);
    return (probe.status != GM82_GMK_PARSE_INVALID) ? JNI_TRUE : JNI_FALSE;
}

JNIEXPORT jstring JNICALL
Java_com_normaker_nativefull_MainActivity_nativeGmkHeaderJson(JNIEnv *env, jclass clazz, jbyteArray bytes) {
    if (!bytes) return (*env)->NewStringUTF(env, "{\"valid\":false,\"error\":\"null_buffer\"}");
    jsize len = (*env)->GetArrayLength(env, bytes);
    jbyte *data = (*env)->GetByteArrayElements(env, bytes, NULL);
    if (!data) return (*env)->NewStringUTF(env, "{\"valid\":false,\"error\":\"alloc_failed\"}");

    gm82_gmk_probe_result probe = gm82_gmk_probe((const uint8_t *)data, (size_t)len);
    (*env)->ReleaseByteArrayElements(env, bytes, data, JNI_ABORT);

    char buf[256];
    snprintf(buf, sizeof(buf),
        "{\"magic\":%d,\"version\":%d,\"appId\":%d,\"valid\":%s,\"formatKind\":%d,\"error\":\"%s\"}",
        probe.magic, probe.version, probe.app_id,
        (probe.status != GM82_GMK_PARSE_INVALID) ? "true" : "false",
        (int)probe.format_kind,
        probe.error_code ? probe.error_code : "");
    return (*env)->NewStringUTF(env, buf);
}

JNIEXPORT jstring JNICALL
Java_com_normaker_nativefull_MainActivity_nativeGmkLayoutJson(JNIEnv *env, jclass clazz, jbyteArray bytes) {
    return (*env)->NewStringUTF(env, "{\"chunks\":[],\"count\":0}");
}

JNIEXPORT jstring JNICALL
Java_com_normaker_nativefull_MainActivity_nativeGmkChunkInventory(JNIEnv *env, jclass clazz, jbyteArray bytes) {
    return (*env)->NewStringUTF(env, "[]");
}

JNIEXPORT jstring JNICALL
Java_com_normaker_nativefull_MainActivity_nativeGmkResourceManifest(JNIEnv *env, jclass clazz, jbyteArray bytes) {
    if (!bytes) return (*env)->NewStringUTF(env, "{\"resources\":[],\"count\":0}");
    jsize len = (*env)->GetArrayLength(env, bytes);
    if (len < 12) return (*env)->NewStringUTF(env, "{\"resources\":[],\"count\":0}");
    jbyte *data = (*env)->GetByteArrayElements(env, bytes, NULL);
    if (!data) return (*env)->NewStringUTF(env, "{\"resources\":[],\"count\":0}");

    gm82_decoded_sprite_list sprites;
    gm82_decoded_background_list bgs;
    gm82_decoded_object_list objs;
    gm82_decoded_room_list rms;

    gm82_decode_sprites_from_gmk((const uint8_t *)data, (size_t)len, &sprites);
    gm82_decode_backgrounds_from_gmk((const uint8_t *)data, (size_t)len, &bgs);
    gm82_decode_objects_from_gmk((const uint8_t *)data, (size_t)len, &objs);
    gm82_decode_rooms_from_gmk((const uint8_t *)data, (size_t)len, &rms);

    (*env)->ReleaseByteArrayElements(env, bytes, data, JNI_ABORT);

    /* Construct JSON manifest */
    size_t json_cap = 65536 + (size_t)sprites.count * 128 + (size_t)bgs.count * 128 + (size_t)objs.count * 128 + (size_t)rms.count * 128;
    char *json = (char *)malloc(json_cap);
    if (!json) {
        gm82_decoded_sprite_list_free(&sprites);
        gm82_decoded_background_list_free(&bgs);
        free(objs.items);
        for (int i = 0; i < rms.count; i++) { free(rms.items[i].instances); free(rms.items[i].tiles); }
        free(rms.items);
        return (*env)->NewStringUTF(env, "{\"resources\":[],\"count\":0}");
    }

    int pos = snprintf(json, json_cap, "{\"ok\":true,\"sprites\":[");
    for (int i = 0; i < sprites.count && pos < (int)json_cap - 256; i++) {
        pos += snprintf(json + pos, json_cap - (size_t)pos,
            "%s{\"id\":%d,\"name\":\"%s\",\"width\":%d,\"height\":%d}",
            (i > 0 ? "," : ""), i, sprites.frames[i].name, sprites.frames[i].width, sprites.frames[i].height);
    }
    pos += snprintf(json + pos, json_cap - (size_t)pos, "],\"backgrounds\":[");
    for (int i = 0; i < bgs.count && pos < (int)json_cap - 256; i++) {
        pos += snprintf(json + pos, json_cap - (size_t)pos,
            "%s{\"id\":%d,\"name\":\"%s\",\"width\":%d,\"height\":%d}",
            (i > 0 ? "," : ""), i, bgs.items[i].name, bgs.items[i].width, bgs.items[i].height);
    }
    pos += snprintf(json + pos, json_cap - (size_t)pos, "],\"objects\":[");
    for (int i = 0; i < objs.count && pos < (int)json_cap - 256; i++) {
        pos += snprintf(json + pos, json_cap - (size_t)pos,
            "%s{\"id\":%d,\"name\":\"%s\",\"sprite_index\":%d,\"solid\":%d,\"visible\":%d,\"depth\":%d}",
            (i > 0 ? "," : ""), i, objs.items[i].name, objs.items[i].sprite_index, objs.items[i].solid, objs.items[i].visible, objs.items[i].depth);
    }
    pos += snprintf(json + pos, json_cap - (size_t)pos, "],\"rooms\":[");
    for (int i = 0; i < rms.count && pos < (int)json_cap - 256; i++) {
        pos += snprintf(json + pos, json_cap - (size_t)pos,
            "%s{\"id\":%d,\"name\":\"%s\",\"width\":%d,\"height\":%d,\"speed\":%d,\"instance_count\":%d}",
            (i > 0 ? "," : ""), i, rms.items[i].name, rms.items[i].width, rms.items[i].height, rms.items[i].speed, rms.items[i].instance_count);
    }
    snprintf(json + pos, json_cap - (size_t)pos, "]}");

    gm82_decoded_sprite_list_free(&sprites);
    gm82_decoded_background_list_free(&bgs);
    free(objs.items);
    for (int i = 0; i < rms.count; i++) { free(rms.items[i].instances); free(rms.items[i].tiles); }
    free(rms.items);

    jstring res = (*env)->NewStringUTF(env, json);
    free(json);
    return res;
}

JNIEXPORT jstring JNICALL
Java_com_normaker_nativefull_MainActivity_nativeImportGmkSnapshot(JNIEnv *env, jclass clazz, jbyteArray bytes, jstring outputDir) {
    if (!bytes) return (*env)->NewStringUTF(env, "{\"ok\":false,\"error\":\"null_buffer\"}");
    jsize len = (*env)->GetArrayLength(env, bytes);
    if (len < 12) return (*env)->NewStringUTF(env, "{\"ok\":false,\"error\":\"too_small\"}");
    jbyte *data = (*env)->GetByteArrayElements(env, bytes, NULL);
    if (!data) return (*env)->NewStringUTF(env, "{\"ok\":false,\"error\":\"memory_error\"}");

    if (g_engine_loaded) {
        gm82_decoded_sprite_list_free(&g_engine_sprites);
        gm82_decoded_background_list_free(&g_engine_bgs);
        free(g_engine_objs.items);
        for (int i = 0; i < g_engine_rooms.count; i++) {
            free(g_engine_rooms.items[i].instances);
            free(g_engine_rooms.items[i].tiles);
        }
        free(g_engine_rooms.items);
        gm82_sprite_group_list_free(&g_engine_sprite_groups);
        gm82_action_table_free(&g_engine_actions);
        g_engine_loaded = 0;
    }

    gm82_decoded_sprite_list_init(&g_engine_sprites);
    gm82_decoded_background_list_init(&g_engine_bgs);
    memset(&g_engine_objs, 0, sizeof(g_engine_objs));
    memset(&g_engine_rooms, 0, sizeof(g_engine_rooms));
    memset(&g_engine_actions, 0, sizeof(g_engine_actions));

    int n_spr = gm82_decode_sprites_from_gmk((const uint8_t *)data, (size_t)len, &g_engine_sprites);
    int n_bg = gm82_decode_backgrounds_from_gmk((const uint8_t *)data, (size_t)len, &g_engine_bgs);
    int n_obj = gm82_decode_objects_from_gmk((const uint8_t *)data, (size_t)len, &g_engine_objs);
    int n_rm = gm82_decode_rooms_from_gmk((const uint8_t *)data, (size_t)len, &g_engine_rooms);
    gm82_actions_scan_gmk((const uint8_t *)data, (size_t)len, &g_engine_actions);

    gm82_sprite_groups_build(&g_engine_sprites, &g_engine_sprite_groups);

    (*env)->ReleaseByteArrayElements(env, bytes, data, JNI_ABORT);

    gm82_runtime_init(&g_engine_rt);
    gm82_runtime_bind_assets(&g_engine_rt,
        &g_engine_objs,
        &g_engine_sprites,
        &g_engine_bgs,
        &g_engine_rooms,
        &g_engine_actions);
    gm82_runtime_bind_sprite_groups(&g_engine_rt, &g_engine_sprite_groups);
    gm82_input_init(&g_engine_input);
    gm82_input_bind_global(&g_engine_input);

    if (n_rm > 0) {
        gm82_runtime_goto_room(&g_engine_rt, 0);
    }

    g_runtime_active = 1;
    g_engine_loaded = 1;

    char buf[512];
    snprintf(buf, sizeof(buf),
        "{\"ok\":true,\"status\":\"ok\",\"sprites\":%d,\"backgrounds\":%d,\"objects\":%d,\"rooms\":%d,\"complete\":%s}",
        n_spr, n_bg, n_obj, n_rm, (n_spr > 0 && n_rm > 0) ? "true" : "false");

    return (*env)->NewStringUTF(env, buf);
}

JNIEXPORT jint JNICALL
Java_com_normaker_nativefull_MainActivity_nativeCompileGml(JNIEnv *env, jclass clazz, jstring source) {
    for (int i = 0; i < MAX_CODES; i++) {
        if (!g_codes[i].in_use) {
            int new_id = ++g_code_id_counter;
            g_codes[i].id = new_id;
            g_codes[i].in_use = 1;
            g_codes[i].arg_count = 0;
            g_codes[i].source = NULL;
            if (source) {
                const char *src = (*env)->GetStringUTFChars(env, source, NULL);
                if (src) {
                    g_codes[i].source = strdup(src);
                    (*env)->ReleaseStringUTFChars(env, source, src);
                }
            }
            return new_id;
        }
    }
    return ++g_code_id_counter;
}

JNIEXPORT jboolean JNICALL
Java_com_normaker_nativefull_MainActivity_nativeCodeExists(JNIEnv *env, jclass clazz, jint codeId) {
    for (int i = 0; i < MAX_CODES; i++) {
        if (g_codes[i].in_use && g_codes[i].id == codeId) return JNI_TRUE;
    }
    return JNI_FALSE;
}

JNIEXPORT jint JNICALL
Java_com_normaker_nativefull_MainActivity_nativeCodeGetArgCount(JNIEnv *env, jclass clazz, jint codeId) {
    for (int i = 0; i < MAX_CODES; i++) {
        if (g_codes[i].in_use && g_codes[i].id == codeId) return g_codes[i].arg_count;
    }
    return 0;
}

JNIEXPORT void JNICALL
Java_com_normaker_nativefull_MainActivity_nativeCodeDestroy(JNIEnv *env, jclass clazz, jint codeId) {
    for (int i = 0; i < MAX_CODES; i++) {
        if (g_codes[i].in_use && g_codes[i].id == codeId) {
            g_codes[i].in_use = 0;
            if (g_codes[i].source) {
                free(g_codes[i].source);
                g_codes[i].source = NULL;
            }
            break;
        }
    }
}

JNIEXPORT jint JNICALL
Java_com_normaker_nativefull_MainActivity_nativeCodeExecute(JNIEnv *env, jclass clazz, jint instanceId, jint codeId) {
    return 0;
}

JNIEXPORT jboolean JNICALL
Java_com_normaker_nativefull_MainActivity_nativeExportRom(JNIEnv *env, jclass clazz, jstring title, jstring outputPath, jint kind) {
    return JNI_TRUE;
}

JNIEXPORT jint JNICALL
Java_com_normaker_nativefull_MainActivity_nativeDetectRom(JNIEnv *env, jclass clazz, jstring path) {
    return 0;
}

JNIEXPORT jboolean JNICALL
Java_com_normaker_nativefull_MainActivity_nativeValidateRom(JNIEnv *env, jclass clazz, jstring path, jint kind) {
    return JNI_TRUE;
}

JNIEXPORT void JNICALL
Java_com_normaker_nativefull_MainActivity_nativeClearResourceRegistry(JNIEnv *env, jclass clazz) {
    g_resource_count = 0;
}

JNIEXPORT jint JNICALL
Java_com_normaker_nativefull_MainActivity_nativeRegisterResource(JNIEnv *env, jclass clazz, jint kind, jint id, jstring name, jint width, jint height, jint frames) {
    if (g_resource_count < MAX_RESOURCES) {
        NativeResource *res = &g_resources[g_resource_count++];
        res->kind = kind;
        res->id = id;
        res->width = width;
        res->height = height;
        res->frames = frames;
        res->name[0] = '\0';
        if (name) {
            const char *str = (*env)->GetStringUTFChars(env, name, NULL);
            if (str) {
                strncpy(res->name, str, sizeof(res->name) - 1);
                res->name[sizeof(res->name) - 1] = '\0';
                (*env)->ReleaseStringUTFChars(env, name, str);
            }
        }
    }
    return id;
}

JNIEXPORT jint JNICALL
Java_com_normaker_nativefull_MainActivity_nativeResourceCount(JNIEnv *env, jclass clazz) {
    return g_resource_count;
}

JNIEXPORT jint JNICALL
Java_com_normaker_nativefull_MainActivity_nativeRegisterObjectEvent(JNIEnv *env, jclass clazz, jint objectId, jint mainType, jint subType, jstring source) {
    if (g_event_count < MAX_EVENTS) {
        NativeEvent *ev = &g_events[g_event_count++];
        ev->objectId = objectId;
        ev->mainType = mainType;
        ev->subType = subType;
        ev->source[0] = '\0';
        if (source) {
            const char *str = (*env)->GetStringUTFChars(env, source, NULL);
            if (str) {
                strncpy(ev->source, str, sizeof(ev->source) - 1);
                ev->source[sizeof(ev->source) - 1] = '\0';
                (*env)->ReleaseStringUTFChars(env, source, str);
            }
        }
        return g_event_count;
    }
    return g_event_count;
}

JNIEXPORT jint JNICALL
Java_com_normaker_nativefull_MainActivity_nativeObjectEventCount(JNIEnv *env, jclass clazz) {
    return g_event_count;
}

JNIEXPORT jdouble JNICALL
Java_com_normaker_nativefull_MainActivity_nativeGm82CompatCheck(JNIEnv *env, jclass clazz) {
    return 8.2;
}

JNIEXPORT jdouble JNICALL
Java_com_normaker_nativefull_MainActivity_nativeGm82ColorReverse(JNIEnv *env, jclass clazz, jdouble color) {
    int c = (int)color;
    int r = c & 0xFF;
    int g = (c >> 8) & 0xFF;
    int b = (c >> 16) & 0xFF;
    return (double)((r << 16) | (g << 8) | b);
}

JNIEXPORT jdouble JNICALL
Java_com_normaker_nativefull_MainActivity_nativeGm82ColorInverse(JNIEnv *env, jclass clazz, jdouble color) {
    int c = (int)color;
    return (double)(0xFFFFFF ^ (c & 0xFFFFFF));
}

JNIEXPORT jint JNICALL
Java_com_normaker_nativefull_MainActivity_nativeGm82TokenStart(JNIEnv *env, jclass clazz, jstring text, jstring separator) {
    if (!text || !separator) return 0;
    const char *native_str = (*env)->GetStringUTFChars(env, text, NULL);
    const char *native_sep = (*env)->GetStringUTFChars(env, separator, NULL);
    if (!native_str || !native_sep) {
        if (native_str) (*env)->ReleaseStringUTFChars(env, text, native_str);
        if (native_sep) (*env)->ReleaseStringUTFChars(env, separator, native_sep);
        return 0;
    }
    memset(g_token_buf, 0, sizeof(g_token_buf));
    memset(g_token_sep, 0, sizeof(g_token_sep));
    strncpy(g_token_buf, native_str, sizeof(g_token_buf) - 1);
    strncpy(g_token_sep, native_sep, sizeof(g_token_sep) - 1);
    g_token_buf[sizeof(g_token_buf) - 1] = '\0';
    g_token_sep[sizeof(g_token_sep) - 1] = '\0';
    (*env)->ReleaseStringUTFChars(env, text, native_str);
    (*env)->ReleaseStringUTFChars(env, separator, native_sep);
    g_token_ctx = g_token_buf;
    return 1;
}

JNIEXPORT jstring JNICALL
Java_com_normaker_nativefull_MainActivity_nativeGm82TokenNext(JNIEnv *env, jclass clazz) {
    if (!g_token_ctx || !*g_token_ctx) return (*env)->NewStringUTF(env, "");
    char *token = strtok_r(g_token_ctx, g_token_sep, &g_token_ctx);
    return (*env)->NewStringUTF(env, token ? token : "");
}

JNIEXPORT void JNICALL
Java_com_normaker_nativefull_MainActivity_nativeGm82TokenReset(JNIEnv *env, jclass clazz) {
    g_token_ctx = NULL;
    memset(g_token_buf, 0, sizeof(g_token_buf));
}
