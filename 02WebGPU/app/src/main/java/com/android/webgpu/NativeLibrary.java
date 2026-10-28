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
    public static native void onResize(int width, int height);
    public static native void start(Surface surface);
    public static native void stop();
    public static native void destroy();
    public static native void OnButton(int button);

    public static native void nativeSendTouch(int pointerId, float x, float y, int actionType);

    public static native void nativeInjectMotionEvent(float axisX, float axisY, float axisZ, float axisRX, float axisRY, float axisRZ, float axisBrake, float axisGas, float hatX, float hatY, int source, int deviceId);
    public static native void nativeInjectKeyEvent(int keyCode, int action, int source, int deviceId);
    public static native void nativeOnDeviceConnected(int deviceId, int layoutType);
    public static native void nativeOnDeviceDisconnected(int deviceId);
}
