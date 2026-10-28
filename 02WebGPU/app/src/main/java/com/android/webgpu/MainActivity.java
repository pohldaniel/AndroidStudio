package com.android.webgpu;

import androidx.annotation.Keep;
import androidx.annotation.NonNull;
import androidx.appcompat.app.AppCompatActivity;

import android.content.res.Configuration;
import android.hardware.input.InputManager;
import android.os.Bundle;
import android.view.InputDevice;
import android.view.KeyEvent;
import android.view.MotionEvent;
import android.widget.Button;

import android.widget.FrameLayout;
import androidx.appcompat.widget.Toolbar;

public class MainActivity extends AppCompatActivity implements InputManager.InputDeviceListener {
    private final static String[] STATES = {"Collada", "Wireframe", "Deferred", "Particle", "Volume", "Audio Decode", "Video Decode", "Cubes", "Isometric" , "Controller", "HDR Bloom"};
    private View view;
    @Keep
    private GamepadManager gamepadManager;
    private static int CurrentStateIndex = 0;


    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        //view.setPointerIcon(PointerIcon.getSystemIcon(this, PointerIcon.TYPE_NULL));
        //setRequestedOrientation(ActivityInfo.SCREEN_ORIENTATION_LANDSCAPE);
        setContentView(R.layout.activity_main);
        FrameLayout webgpuContainer = findViewById(R.id.webgpu_container);
        view = new View(this);
        view.initRenderer(getAssets());
        gamepadManager = new GamepadManager(this);
        webgpuContainer.addView(view);

        Toolbar toolbar = findViewById(R.id.toolbar);
        toolbar.setTitle(STATES[CurrentStateIndex]);
        setSupportActionBar(toolbar);

        Button buttonLeft = findViewById(R.id.button_left);
        Button buttonAction = findViewById(R.id.button_center);
        Button buttonRight = findViewById(R.id.button_right);

        if (buttonLeft != null && buttonRight != null && buttonAction != null) {
            int length = STATES.length - 1;
            buttonAction.setVisibility(View.GONE);
            buttonAction.setVisibility(CurrentStateIndex == 1 ? View.VISIBLE : View.GONE);

            buttonLeft.setOnClickListener(v -> {
                if (CurrentStateIndex > 0) {
                    CurrentStateIndex--;
                }else{
                    CurrentStateIndex = length;
                }
                toolbar.setTitle(STATES[CurrentStateIndex]);
                NativeLibrary.OnButton(0);
                buttonAction.setVisibility(CurrentStateIndex == 1 ? View.VISIBLE : View.GONE);
            });

            buttonRight.setOnClickListener(v -> {
                if (CurrentStateIndex < length) {
                    CurrentStateIndex++;
                }else{
                    CurrentStateIndex = 0;
                }

                toolbar.setTitle(STATES[CurrentStateIndex]);
                NativeLibrary.OnButton(2);
                buttonAction.setVisibility(CurrentStateIndex == 1 ? View.VISIBLE : View.GONE);
            });

            buttonAction.setOnClickListener(
                v-> NativeLibrary.OnButton(1)
            );
        }
    }

    @Override
    protected void onPause() {
        super.onPause();
    }

    @Override
    protected void onResume() {
        super.onResume();
    }

    @Override
    protected void onDestroy() {
        super.onDestroy();
        InputManager inputManager = (InputManager) getSystemService(INPUT_SERVICE);
        if (inputManager != null) {
            inputManager.unregisterInputDeviceListener(this);
        }
        view.onDestroy();
    }

    @Override
    public void onConfigurationChanged(@NonNull Configuration newConfig) {
        super.onConfigurationChanged(newConfig);
        view.post(new Runnable() {
            @Override
            public void run() {
                NativeLibrary.onResize(view.getWidth(), view.getHeight());
            }
        });
    }

    @Override
    public boolean onKeyDown(int keyCode, KeyEvent event) {
        NativeLibrary.OnKey(keyCode, event.getAction(), event.getSource(), event.getDeviceId());
        if (keyCode == KeyEvent.KEYCODE_BUTTON_B &&
                ((event.getSource() & InputDevice.SOURCE_GAMEPAD) == InputDevice.SOURCE_GAMEPAD ||
                        (event.getSource() & InputDevice.SOURCE_JOYSTICK) == InputDevice.SOURCE_JOYSTICK)) {
            return true;
        }
        return super.onKeyDown(keyCode, event);
    }

    @Override
    public boolean onKeyUp(int keyCode, KeyEvent event) {
        NativeLibrary.OnKey(keyCode, event.getAction(), event.getSource(), event.getDeviceId());
        if (keyCode == KeyEvent.KEYCODE_BUTTON_B &&
                ((event.getSource() & InputDevice.SOURCE_GAMEPAD) == InputDevice.SOURCE_GAMEPAD ||
                        (event.getSource() & InputDevice.SOURCE_JOYSTICK) == InputDevice.SOURCE_JOYSTICK)) {
            return true;
        }
        return super.onKeyUp(keyCode, event);
    }

    @Override
    public boolean onGenericMotionEvent(MotionEvent event) {
        if ((event.getSource() & InputDevice.SOURCE_JOYSTICK) == InputDevice.SOURCE_JOYSTICK ||
                (event.getSource() & InputDevice.SOURCE_GAMEPAD) == InputDevice.SOURCE_GAMEPAD) {

            float axisX = event.getAxisValue(MotionEvent.AXIS_X);
            float axisY = event.getAxisValue(MotionEvent.AXIS_Y);
            float axisZ = event.getAxisValue(MotionEvent.AXIS_Z);
            float axisRZ = event.getAxisValue(MotionEvent.AXIS_RZ);
            float axisBrake = event.getAxisValue(MotionEvent.AXIS_BRAKE);
            float axisGas = event.getAxisValue(MotionEvent.AXIS_GAS);
            float hatX = event.getAxisValue(MotionEvent.AXIS_HAT_X);
            float hatY = event.getAxisValue(MotionEvent.AXIS_HAT_Y);
            float axisRX = event.getAxisValue(MotionEvent.AXIS_RX);
            float axisRY = event.getAxisValue(MotionEvent.AXIS_RY);

            NativeLibrary.OnGenericMotionEvent(axisX, axisY, axisZ, axisRX, axisRY, axisRZ, axisBrake, axisGas, hatX, hatY, event.getSource(), event.getDeviceId());
            return true;
        }
        return super.onGenericMotionEvent(event);
    }

    @Override
    public void onInputDeviceAdded(int deviceId) {
        InputDevice device = InputDevice.getDevice(deviceId);
        GamepadManager.CheckAndRegisterGamepad(device);
    }

    @Override
    public void onInputDeviceRemoved(int deviceId) {
        NativeLibrary.OnInputDeviceRemoved(deviceId);
    }

    @Override
    public void onInputDeviceChanged(int deviceId) {

    }
}

