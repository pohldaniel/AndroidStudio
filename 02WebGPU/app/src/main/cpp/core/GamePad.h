#pragma once
#include <android/input.h>
#include "Globals.h"

enum GamepadButton {
	NONE = 0,
	DPAD_UP = 1,
	DPAD_DOWN = 2,
	DPAD_LEFT = 3,
	DPAD_RIGHT = 4,
	DPAD_UP_RIGHT = 5,
	DPAD_DOWN_RIGHT = 6,
	DPAD_UP_LEFT = 7,
	DPAD_DOWN_LEFT = 8,
	GAMEPAD_START = 9,
	GAMEPAD_BACK = 10,
	GAMEPAD_LEFT_THUMB = 11,
	GAMEPAD_RIGHT_THUMB = 12,

	GAMEPAD_LEFT_SHOULDER = 13,
	GAMEPAD_RIGHT_SHOULDER = 14,
	GAMEPAD_A = 15,
	GAMEPAD_B = 16,
	GAMEPAD_X = 17,
	GAMEPAD_Y = 18,
	GAMEPAD_LEFT_TRIGGER = 19,
	GAMEPAD_RIGHT_TRIGGER = 20,
};

enum class GamepadLayout {
    STANDARD = 0,
    DINPUT = 1,
    Count
};

class IGamePad {

public:
	virtual void update() = 0;
	virtual void setVibration(float leftMotor, float rightMotor) = 0;

	virtual float getRightStickX() const = 0;
	virtual float getRightStickY() const = 0;
	virtual float getLeftStickX() const = 0;
	virtual float getLeftStickY() const = 0;
	virtual float getLeftTrigger() const = 0;
	virtual float getRightTrigger() const = 0;

	virtual bool buttonPressed(GamepadButton button) const = 0;
	virtual bool buttonDown(GamepadButton button) const = 0;
	virtual bool buttonUp(GamepadButton button) const = 0;
	virtual bool isConnected() = 0;
};

class AndroidGamepad : public IGamePad {

public:

	AndroidGamepad();
	virtual ~AndroidGamepad() = default;

	void update() override;
	void setVibration(float leftMotor, float rightMotor) override;

	float getLeftStickX() const override;
	float getLeftStickY() const override;
	float getRightStickX() const override;
	float getRightStickY() const override;
	float getLeftTrigger() const override;
	float getRightTrigger() const override;

	bool buttonDown(GamepadButton button) const override;
	bool buttonPressed(GamepadButton button) const override;
	bool buttonUp(GamepadButton button) const override;
	bool isConnected() override;
    int getDeviceId() const;
	GamepadLayout getLayoutType() const;

    void setLeftStickX(float leftStickX);
    void setLeftStickY(float leftStickY);
    void setRightStickX(float rightStickX);
    void setRightStickY(float rightStickY);
    void setLeftTrigger(float leftTrigger);
    void setRightTrigger(float rightTrigger);

    void setConnected(bool connected);
    void setLayoutType(GamepadLayout layoutType);
    void setDeviceId(int deviceId);
    void setButtonState(GamepadButton button, bool isPressed);
	void reset();

	static void Update();
	static void Initialize(JNIEnv* env);
	static void Shutdown();
	static IGamePad* GetPlayer(int playerIndex);
	static IGamePad* GetPlayer();
	static GamepadButton MapKeyCode(int32_t keyCode);
    static void  ApplyRadialDeadzone(float rawX, float rawY, float& outX, float& outY);
	static AndroidGamepad* s_Players[4];

private:

    GamepadLayout m_layoutType;
	float m_leftStickX;
	float m_leftStickY;
	float m_rightStickX;
	float m_rightStickY;
	float m_leftTrigger;
	float m_rightTrigger;

	bool m_buttonStates[2][21];
    bool* m_pCurrButtonStates;
	bool* m_pPrevButtonStates;
	int m_playerIndex;
    bool m_connected;
    int m_deviceId;

	static const float DEADZONE;
	static JavaVM* Vm;
	static jclass VibrationClass;
	static jmethodID VibrateMethodId;
};