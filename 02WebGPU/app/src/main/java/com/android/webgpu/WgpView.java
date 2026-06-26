package com.android.webgpu;

import android.content.Context;
import android.content.res.AssetManager;
import android.util.DisplayMetrics;
import android.util.TypedValue;
import android.view.SurfaceHolder;
import android.view.SurfaceView;

import androidx.annotation.NonNull;

import javax.microedition.khronos.opengles.GL10;

class WgpView extends SurfaceView implements SurfaceHolder.Callback {

    WgpRenderer wgpRenderer;

    public WgpView(Context context) {
        super(context);
        getHolder().addCallback(this);
    }
    public void initRenderer(final AssetManager assetManager){
        wgpRenderer = new WgpRenderer(assetManager);
    }

    @Override
    public void surfaceCreated(@NonNull SurfaceHolder holder) {
        wgpRenderer.onSurfaceCreated(holder);
    }

    @Override
    public void surfaceChanged(@NonNull SurfaceHolder holder, int format, int width, int height) {
        wgpRenderer.onSurfaceChanged(holder, format, width, height);
    }

    @Override
    public void surfaceDestroyed(@NonNull SurfaceHolder holder) {
        wgpRenderer.onSurfaceDestroyed();
    }
}