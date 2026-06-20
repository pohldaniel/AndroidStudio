package com.android.dragon;

import android.content.res.AssetManager;
import android.opengl.GLSurfaceView;

import javax.microedition.khronos.egl.EGLConfig;
import javax.microedition.khronos.opengles.GL10;

class OGLRenderer implements GLSurfaceView.Renderer {

    AssetManager assetManager;

    OGLRenderer(AssetManager assetManager){
        this.assetManager = assetManager;
    }

    @Override
    public void onDrawFrame(GL10 gl) {
        NativeLibrary.step();
    }

    @Override
    public void onSurfaceChanged(GL10 gl, int width, int height) {
        NativeLibrary.init(width, height, assetManager);
    }

    @Override
    public void onSurfaceCreated(GL10 gl, EGLConfig config) {
    }
}