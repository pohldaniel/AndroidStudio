package com.android.webgpu;

import android.content.res.AssetManager;
import android.opengl.GLSurfaceView;
import android.view.Surface;
import android.view.SurfaceHolder;
import android.view.SurfaceView;

import javax.microedition.khronos.egl.EGLConfig;
import javax.microedition.khronos.opengles.GL10;

class WgpRenderer {

    SurfaceView surfaceView;
    AssetManager assetManager;
    WgpRenderer(SurfaceView surfaceView, AssetManager assetManager){
        this.surfaceView = surfaceView;
        this.assetManager = assetManager;
        NativeLibrary.wgpInit();
        NativeLibrary.initStates();
    }

    public void onSurfaceCreated(SurfaceHolder holder) {
       NativeLibrary.wgpConfigureSurface(holder.getSurface());
    }

    public void onSurfaceChanged(SurfaceHolder holder,  int format, int width, int height) {
        stop();
        NativeLibrary.wgpResize(holder.getSurface(), width, height);
        start(holder);
    }

    public void onSurfaceDestroyed(){
        stop();
    }

    public void start(SurfaceHolder holder) {
        NativeLibrary.start(holder.getSurface());
    }

    public void stop() {
        NativeLibrary.stop();
    }

}