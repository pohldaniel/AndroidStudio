package com.android.webgpu;



import androidx.annotation.NonNull;
import androidx.appcompat.app.AppCompatActivity;

import android.content.res.Configuration;
import android.hardware.input.InputManager;
import android.os.Build;
import android.os.Bundle;
import android.os.CombinedVibration;
import android.os.VibrationEffect;
import android.os.VibratorManager;
import android.view.InputDevice;
import android.view.KeyEvent;
import android.view.MotionEvent;
import android.view.PointerIcon;
import android.widget.Button;

import android.widget.FrameLayout;
import androidx.appcompat.widget.Toolbar;

public class MainActivity extends AppCompatActivity implements InputManager.InputDeviceListener {
    private final String[] appStates = {"Collada", "Wireframe", "Deferred", "Particle", "Volume", "Audio Decode", "Video Decode", "Cubes", "Isometric" , "Controller"};
    private static int CurrentStateIndex = 0;
    private View view;

    private static MainActivity instance;
    public static void triggerGamepadVibration(int deviceId, float leftMotor, float rightMotor) {
        if (instance == null || Build.VERSION.SDK_INT < Build.VERSION_CODES.S) {
            return;
        }

        instance.runOnUiThread(() -> {
            InputDevice device = InputDevice.getDevice(deviceId);
            if (device != null) {
                VibratorManager vm = device.getVibratorManager();

                int leftAmp = (int) (leftMotor * 255);
                int rightAmp = (int) (rightMotor * 255);

                if (leftAmp == 0 && rightAmp == 0) {
                    vm.cancel();
                    return;
                }

                try {
                    long[] timings = new long[] { 0, 1000 };

                    VibrationEffect effectLeft = VibrationEffect.createWaveform(
                            timings,
                            new int[] { 0, Math.max(1, leftAmp) },
                            0
                    );
                    VibrationEffect effectRight = VibrationEffect.createWaveform(
                            timings,
                            new int[] { 0, Math.max(1, rightAmp) },
                            0
                    );

                    CombinedVibration combined = CombinedVibration.startParallel()
                            .addVibrator(0, effectLeft)
                            .addVibrator(1, effectRight)
                            .combine();

                    vm.vibrate(combined);
                } catch (Exception e) {
                    int maxAmp = Math.max(leftAmp, rightAmp);
                    VibrationEffect fallbackEffect = VibrationEffect.createOneShot(200, Math.max(1, maxAmp));
                    CombinedVibration fallbackCombined = CombinedVibration.createParallel(fallbackEffect);
                    vm.vibrate(fallbackCombined);
                }
            }
        });
    }


    @Override
    public boolean onKeyDown(int keyCode, KeyEvent event) {
        NativeLibrary.nativeInjectKeyEvent(keyCode, event.getAction(), event.getSource(), event.getDeviceId());
        if (keyCode == KeyEvent.KEYCODE_BUTTON_B &&
                ((event.getSource() & InputDevice.SOURCE_GAMEPAD) == InputDevice.SOURCE_GAMEPAD ||
                        (event.getSource() & InputDevice.SOURCE_JOYSTICK) == InputDevice.SOURCE_JOYSTICK)) {
            return true;
        }
        return super.onKeyDown(keyCode, event);
    }

    @Override
    public boolean onKeyUp(int keyCode, KeyEvent event) {
        NativeLibrary.nativeInjectKeyEvent(keyCode, event.getAction(), event.getSource(), event.getDeviceId());
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

            NativeLibrary.nativeInjectMotionEvent(axisX, axisY, axisZ, axisRX, axisRY, axisRZ, axisBrake, axisGas, hatX, hatY, event.getSource(), event.getDeviceId());
            return true;
        }
        return super.onGenericMotionEvent(event);
    }
    private boolean isGamepad(InputDevice device) {
        int sources = device.getSources();
        return ((sources & InputDevice.SOURCE_GAMEPAD) == InputDevice.SOURCE_GAMEPAD) ||
                ((sources & InputDevice.SOURCE_JOYSTICK) == InputDevice.SOURCE_JOYSTICK);
    }

    @Override
    public void onInputDeviceAdded(int deviceId) {
        InputDevice device = InputDevice.getDevice(deviceId);
        if (device != null && isGamepad(device)) {
            String deviceName = device.getName().toUpperCase();
            int layoutType = 0;
            if (deviceName.contains("IGP") || deviceName.contains("SONY")) {
                layoutType = 1;
            }
            NativeLibrary.nativeOnDeviceConnected(deviceId, layoutType);
        }
    }

    @Override
    public void onInputDeviceRemoved(int deviceId) {
        NativeLibrary.nativeOnDeviceDisconnected(deviceId);
    }

    @Override
    public void onInputDeviceChanged(int deviceId) {

    }

    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        instance = this;
        //view.setPointerIcon(PointerIcon.getSystemIcon(this, PointerIcon.TYPE_NULL));
        //setRequestedOrientation(ActivityInfo.SCREEN_ORIENTATION_LANDSCAPE);
        setContentView(R.layout.activity_main);
        FrameLayout webgpuContainer = findViewById(R.id.webgpu_container);

        view = new View(this);
        view.initRenderer(getAssets());
        webgpuContainer.addView(view);


        InputManager inputManager = (InputManager) getSystemService(INPUT_SERVICE);
        if (inputManager != null) {
            inputManager.registerInputDeviceListener(this, null);
            for (int id : inputManager.getInputDeviceIds()) {
                InputDevice device = inputManager.getInputDevice(id);
                if (device != null && isGamepad(device)) {
                    String deviceName = device.getName().toUpperCase();
                    int layoutType = 0;
                    if (deviceName.contains("IGP") || deviceName.contains("SONY")) {
                        layoutType = 1;
                    }
                    NativeLibrary.nativeOnDeviceConnected(id, layoutType);
                    break;
                }
            }
        }

        Toolbar toolbar = findViewById(R.id.toolbar);
        toolbar.setTitle(appStates[CurrentStateIndex]);
        //setSupportActionBar(toolbar);

        Button buttonLeft = findViewById(R.id.button_left);
        Button buttonAction = findViewById(R.id.button_center);
        Button buttonRight = findViewById(R.id.button_right);

        if (buttonLeft != null && buttonRight != null && buttonAction != null) {
            int length = appStates.length - 1;
            buttonAction.setVisibility(View.GONE);
            buttonAction.setVisibility(CurrentStateIndex == 1 ? View.VISIBLE : View.GONE);

            buttonLeft.setOnClickListener(v -> {
                if (CurrentStateIndex > 0) {
                    CurrentStateIndex--;
                }else{
                    CurrentStateIndex = length;
                }
                toolbar.setTitle(appStates[CurrentStateIndex]);
                NativeLibrary.OnButton(0);
                buttonAction.setVisibility(CurrentStateIndex == 1 ? View.VISIBLE : View.GONE);
            });

            buttonRight.setOnClickListener(v -> {
                if (CurrentStateIndex < length) {
                    CurrentStateIndex++;
                }else{
                    CurrentStateIndex = 0;
                }

                toolbar.setTitle(appStates[CurrentStateIndex]);
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
}

