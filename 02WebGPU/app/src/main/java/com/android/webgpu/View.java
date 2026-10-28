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
                    if (activeJoystickPointerId == -1) {
                        activeJoystickPointerId = pointerId;
                        NativeLibrary.OnTouchEvent(pointerId, x, y, 0); // DOWN
                    }
                } else {
                    if (activeActionPointerId == -1) {
                        NativeLibrary.OnTouchEvent(pointerId, x, y, 0);
                    }
                }
                break;

            case MotionEvent.ACTION_MOVE:
                for (int i = 0; i < event.getPointerCount(); i++) {
                    int pId = event.getPointerId(i);
                    if (pId == activeJoystickPointerId || pId == activeActionPointerId) {
                        NativeLibrary.OnTouchEvent(pId, event.getX(i), event.getY(i), 1); // MOVE
                    }
                }
                break;

            case MotionEvent.ACTION_UP:
            case MotionEvent.ACTION_POINTER_UP:
            case MotionEvent.ACTION_CANCEL:
                if (pointerId == activeJoystickPointerId) {
                    activeJoystickPointerId = -1;
                    NativeLibrary.OnTouchEvent(pointerId, x, y, 2); // UP
                } else if (pointerId == activeActionPointerId) {
                    activeActionPointerId = -1;
                    NativeLibrary.OnTouchEvent(pointerId, x, y, 2); // UP
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

    /*@Override
    public boolean onKeyDown(int keyCode, KeyEvent event) {
        if ((event.getSource() & InputDevice.SOURCE_GAMEPAD) == InputDevice.SOURCE_GAMEPAD ||
                (event.getSource() & InputDevice.SOURCE_JOYSTICK) == InputDevice.SOURCE_JOYSTICK) {

            NativeLibrary.handleKeyEvent(keyCode, true);
            return true;
        }
        return super.onKeyDown(keyCode, event);
    }

    @Override
    public boolean onKeyUp(int keyCode, KeyEvent event) {
        if ((event.getSource() & InputDevice.SOURCE_GAMEPAD) == InputDevice.SOURCE_GAMEPAD ||
                (event.getSource() & InputDevice.SOURCE_JOYSTICK) == InputDevice.SOURCE_JOYSTICK) {

            NativeLibrary.handleKeyEvent(keyCode, false);
            return true;
        }
        return super.onKeyUp(keyCode, event);
    }

    @Override
    public boolean onGenericMotionEvent(MotionEvent event) {
        if ((event.getSource() & InputDevice.SOURCE_JOYSTICK) == InputDevice.SOURCE_JOYSTICK &&
                event.getAction() == MotionEvent.ACTION_MOVE) {

            // Relevante Achsen auslesen und an C++ senden
            NativeLibrary.handleMotionEvent(MotionEvent.AXIS_X, event.getAxisValue(MotionEvent.AXIS_X));
            NativeLibrary.handleMotionEvent(MotionEvent.AXIS_Y, event.getAxisValue(MotionEvent.AXIS_Y));
            NativeLibrary.handleMotionEvent(MotionEvent.AXIS_Z, event.getAxisValue(MotionEvent.AXIS_Z));
            NativeLibrary.handleMotionEvent(MotionEvent.AXIS_RZ, event.getAxisValue(MotionEvent.AXIS_RZ));
            NativeLibrary.handleMotionEvent(MotionEvent.AXIS_BRAKE, event.getAxisValue(MotionEvent.AXIS_BRAKE));
            NativeLibrary.handleMotionEvent(MotionEvent.AXIS_GAS, event.getAxisValue(MotionEvent.AXIS_GAS));

            return true;
        }
        return super.onGenericMotionEvent(event);
    }*/
}