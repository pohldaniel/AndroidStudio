package com.android.webgpu;

import android.content.res.AssetManager;
import android.view.Surface;

public class NativeLibrary {

    static {
        System.loadLibrary("native");
    }

    public static native void wgpInit(AssetManager assetManager);
    public static native void initStates();
    public static native void wgpConfigureSurface(Surface surface);
    public static native void resize(Surface surface, int width, int height);

    public static native void start(Surface surface);
    public static native void stop();
    public static native void destroy();
    public static native void OnButton();
    public static native void OnAction();
}
