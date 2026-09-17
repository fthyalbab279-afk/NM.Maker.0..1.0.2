package com.normaker.gm82;

/**
 * JNI declarations for NOR Maker GM82 core.
 * Load with: System.loadLibrary("gm82_android");
 *
 * STATUS: declarations only – requires compiled libgm82_android.so from NDK.
 * gm82_native_draw is a STUB until GL textures are wired.
 */
public class Gm82Native {
    static {
        try {
            System.loadLibrary("gm82_android");
        } catch (UnsatisfiedLinkError e) {
            // Library not packaged yet – expected until NDK build is done
        }
    }

    public static native boolean nativeInit(int width, int height);
    public static native void nativeShutdown();
    public static native boolean nativeLoadGame(String absolutePath);
    public static native void nativeResize(int width, int height);
    public static native void nativeStep();
    public static native void nativeDraw();
    public static native void nativeKeyDown(int vk);
    public static native void nativeKeyUp(int vk);
    public static native void nativeTouch(int x, int y, int action);
    public static native boolean nativeIsRunning();
    public static native int nativeRoomWidth();
    public static native int nativeRoomHeight();

    // VK constants matching gm82_input.h
    public static final int VK_LEFT = 37;
    public static final int VK_UP = 38;
    public static final int VK_RIGHT = 39;
    public static final int VK_DOWN = 40;
    public static final int VK_SPACE = 32;
    public static final int VK_ENTER = 13;
}
