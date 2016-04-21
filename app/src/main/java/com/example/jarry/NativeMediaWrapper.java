package com.example.jarry;

import android.graphics.SurfaceTexture;

/**
 * Created by Administrator on 2016/4/17 0017.
 */
public class NativeMediaWrapper {
    static {
        System.loadLibrary("native_media");
    }

    public static native void nativeOnCreate();
    public static native void nativeOnDestroy();
    public static native void nativeSurfaceCreated();
    public static native void nativeSurfaceChanged(int width, int height);
    public static native void nativeDrawFrame();
    public static native void nativeFrameAailable();
    public static native SurfaceTexture nativeGetSurfaceTexture();
}
