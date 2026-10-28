package com.android.webgpu;

import android.hardware.input.InputManager;
import android.os.Build;
import android.os.CombinedVibration;
import android.os.VibrationEffect;
import android.os.VibratorManager;
import android.view.InputDevice;

public class GamepadManager {
    private final static String[] VENDORS = {"IGP", "SONY"};

    GamepadManager(MainActivity instance){
        InputManager inputManager = (InputManager) instance.getSystemService(android.content.Context.INPUT_SERVICE);
        if (inputManager != null) {
            inputManager.registerInputDeviceListener(instance, null);
            for (int id : inputManager.getInputDeviceIds()) {
                InputDevice device = inputManager.getInputDevice(id);
                CheckAndRegisterGamepad(device);
            }
        }
    }

    public static void callVibration(int deviceId, float leftMotor, float rightMotor) {
        if (Build.VERSION.SDK_INT < Build.VERSION_CODES.S) {
            return;
        }

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
    }

    private static boolean IsGamepad(InputDevice device) {
        int sources = device.getSources();
        return ((sources & InputDevice.SOURCE_GAMEPAD) == InputDevice.SOURCE_GAMEPAD) ||
                ((sources & InputDevice.SOURCE_JOYSTICK) == InputDevice.SOURCE_JOYSTICK);
    }

    public static void CheckAndRegisterGamepad(InputDevice device) {
        if (device != null && IsGamepad(device)) {
            String deviceName = device.getName().toUpperCase();
            int layoutType = 0;
            for (String vendor : VENDORS) {
                if (deviceName.contains(vendor)) {
                    layoutType = 1;
                    break;
                }
            }
            NativeLibrary.OnInputDeviceAdded(device.getId(), layoutType);
        }
    }
}
