package com.android.webgpu;

import android.content.Context;
import android.content.res.AssetManager;
import android.view.SurfaceHolder;
import android.view.SurfaceView;

import androidx.annotation.NonNull;


class View extends SurfaceView implements SurfaceHolder.Callback {

    Renderer renderer;

    public View(Context context) {
        super(context);
        getHolder().addCallback(this);
    }
    public void initRenderer(final AssetManager assetManager){
        renderer = new Renderer(assetManager);
    }

    @Override
    public void surfaceCreated(@NonNull SurfaceHolder holder) {
        renderer.onSurfaceCreated(holder);
    }

    @Override
    public void surfaceChanged(@NonNull SurfaceHolder holder, int format, int width, int height) {
        renderer.onSurfaceChanged(holder, format, width, height);
    }

    @Override
    public void surfaceDestroyed(@NonNull SurfaceHolder holder) {
        renderer.onSurfaceDestroyed();
    }

    public void onDestroy(){
        renderer.onDestroy();
    }

    public void onButton(){
        renderer.onButton();
    }

    public void onAction(){
        renderer.onAction();
    }
}