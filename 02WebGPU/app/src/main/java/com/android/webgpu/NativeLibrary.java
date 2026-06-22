package com.android.webgpu;

import android.content.res.AssetManager;
import android.view.Surface;

public class NativeLibrary {

    static {
        System.loadLibrary("native");
    }

    public static native void init(int width, int height, AssetManager assetManager);
    public static native void step();
    public static native void initWindow(Surface surface);
}
