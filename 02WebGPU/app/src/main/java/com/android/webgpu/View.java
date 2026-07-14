package com.android.webgpu;

import android.content.Context;
import android.content.res.AssetManager;
import android.view.MotionEvent;
import android.view.SurfaceHolder;
import android.view.SurfaceView;

import androidx.annotation.NonNull;

class View extends SurfaceView implements SurfaceHolder.Callback {

    Renderer renderer;
    private static int activeJoystickPointerId = -1;
    private static int activeActionPointerId = -1;

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
        renderer.onContextDestroyed();
    }

    @Override
    public boolean onTouchEvent(MotionEvent event) {
        int actionEncoded = event.getAction();
        int action = actionEncoded & MotionEvent.ACTION_MASK;
        int pointerIndex = (actionEncoded & MotionEvent.ACTION_POINTER_INDEX_MASK) >> MotionEvent.ACTION_POINTER_INDEX_SHIFT;

        int pointerId = event.getPointerId(pointerIndex);
        float x = event.getX(pointerIndex);
        float y = event.getY(pointerIndex);
        int viewWidth = getWidth();

        switch (action) {
            case MotionEvent.ACTION_DOWN:
            case MotionEvent.ACTION_POINTER_DOWN:
                if (x < (viewWidth / 2)) {
                    // Linke Hälfte -> Joystick fangen
                    if (activeJoystickPointerId == -1) {
                        activeJoystickPointerId = pointerId;
                        NativeLibrary.nativeSendTouch(pointerId, x, y, 0); // DOWN
                    }
                } else {
                    // Rechte Hälfte -> Action-Button fangen
                    if (activeActionPointerId == -1) {
                        activeActionPointerId = pointerId;
                        // TIPP: Wenn du mehrere Buttons hast, kannst du entweder einen zweiten
                        // JNI-Kanal aufmachen (z.B. nativeSendActionTouch) oder die Daten
                        // in ein Array in C++ schreiben. Für den Anfang reicht es,
                        // die Koordinaten an C++ weiterzugeben, da C++ prüft, welcher Button getroffen wurde!
                        NativeLibrary.nativeSendTouch(pointerId, x, y, 0);
                    }
                }
                break;

            case MotionEvent.ACTION_MOVE:
                // Alle aktiven Finger updaten
                for (int i = 0; i < event.getPointerCount(); i++) {
                    int pId = event.getPointerId(i);
                    if (pId == activeJoystickPointerId || pId == activeActionPointerId) {
                        NativeLibrary.nativeSendTouch(pId, event.getX(i), event.getY(i), 1); // MOVE
                    }
                }
                break;

            case MotionEvent.ACTION_UP:
            case MotionEvent.ACTION_POINTER_UP:
            case MotionEvent.ACTION_CANCEL:
                if (pointerId == activeJoystickPointerId) {
                    activeJoystickPointerId = -1;
                    NativeLibrary.nativeSendTouch(pointerId, x, y, 2); // UP
                } else if (pointerId == activeActionPointerId) {
                    activeActionPointerId = -1;
                    NativeLibrary.nativeSendTouch(pointerId, x, y, 2); // UP
                    performClick();
                }
                break;
        }
        return true;
    }

    @Override
    public boolean performClick() {
        return super.performClick();
    }
}