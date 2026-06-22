package com.android.collada;

import android.content.res.AssetManager;

public class NativeLibrary {

    static {
        System.loadLibrary("native");
    }

    public static native void init(int width, int height, AssetManager assetManager);
    public static native void step();
}
