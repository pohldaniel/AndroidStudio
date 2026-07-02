package com.android.webgpu;

import android.content.res.AssetManager;
import android.view.SurfaceHolder;

class Renderer {

    Renderer(AssetManager assetManager){
        NativeLibrary.wgpInit(assetManager);
    }

    public void onSurfaceCreated(SurfaceHolder holder) {
       NativeLibrary.wgpConfigureSurface(holder.getSurface());
       NativeLibrary.resize(holder.getSurface(), holder.getSurfaceFrame().width(), holder.getSurfaceFrame().height());
       NativeLibrary.initStates();
    }

    public void onSurfaceChanged(SurfaceHolder holder,  int format, int width, int height) {
        NativeLibrary.stop();
        NativeLibrary.resize(holder.getSurface(), width, height);
        NativeLibrary.start(holder.getSurface());
    }

    public void onSurfaceDestroyed(){
        NativeLibrary.stop();
    }

    public void onContextDestroyed(){
        NativeLibrary.destroy();
    }

}