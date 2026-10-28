#include <iostream>
#include <fstream>
#include <sstream>
#include <algorithm>
#include <AssetIO.h>

#include "GamePad.h"

JavaVM* AndroidGamepad::Vm;
jclass AndroidGamepad::MainActivityClass;
jmethodID AndroidGamepad::VibrateMethodId;
const float AndroidGamepad::Deadzone = 0.12f;
AndroidGamepad* AndroidGamepad::s_Players[4] = { nullptr, nullptr, nullptr, nullptr };

AndroidGamepad::AndroidGamepad() {
    m_playerIndex = -1;
    m_deviceId = -1;
    m_pCurrButtonStates = m_buttonStates[0];
    m_pPrevButtonStates = m_buttonStates[1];

    memset(m_pCurrButtonStates, false, 21);
    memset(m_pPrevButtonStates, false, 21);

    m_connected = false;
    m_leftStickX = 0.0f;
    m_leftStickY = 0.0f;
    m_rightStickX = 0.0f;
    m_rightStickY = 0.0f;
    m_leftTrigger = 0.0f;
    m_rightTrigger = 0.0f;
    m_layoutType = GamepadLayout::STANDARD;
}

void AndroidGamepad::update(){
    bool* pTempGamePadStates = m_pPrevButtonStates;
    m_pPrevButtonStates = m_pCurrButtonStates;
    m_pCurrButtonStates = pTempGamePadStates;
    memcpy(m_pCurrButtonStates, m_pPrevButtonStates, 21);
}

void AndroidGamepad::setVibration(float leftMotor, float rightMotor) {
    if (m_deviceId == -1 || Vm == nullptr || VibrateMethodId == nullptr) return;

    JNIEnv* env = nullptr;
    bool isAttached = false;
    int res = Vm->GetEnv((void**)&env, JNI_VERSION_1_6);

    if (res == JNI_EDETACHED) {
        if (Vm->AttachCurrentThread(&env, nullptr) == 0) {
         isAttached = true;
        }
    }

    if (env != nullptr) {
        env->CallStaticVoidMethod(MainActivityClass, VibrateMethodId, m_deviceId, leftMotor, rightMotor);
        if (isAttached) {
            Vm->DetachCurrentThread();
        }
    }
}

IGamePad* AndroidGamepad::GetPlayer(int playerIndex) {
    if (playerIndex >= 0 && playerIndex < 4) {
        return AndroidGamepad::s_Players[playerIndex];
    }
    return nullptr;
}

IGamePad* AndroidGamepad::GetPlayer(){
    for (int i = 0; i < 4; ++i) {
        if (AndroidGamepad::s_Players[i]->isConnected()) {
            return AndroidGamepad::s_Players[i];
        }
    }
    return nullptr;
}

void AndroidGamepad::Update() {
    for (int i = 0; i < 4; ++i) {
        IGamePad* player = AndroidGamepad::GetPlayer(i);
        if (player) {
            player->update();
        }
    }
}

void AndroidGamepad::Initialize(JNIEnv* env)  {
    for (int i = 0; i < 4; ++i) {
        AndroidGamepad::s_Players[i] = new AndroidGamepad();
        AndroidGamepad::s_Players[i]->m_playerIndex = i;
    }

    env->GetJavaVM(&Vm);
    jclass localClass = env->FindClass("com/android/webgpu/MainActivity");
    MainActivityClass = (jclass)env->NewGlobalRef(localClass);
    VibrateMethodId = env->GetStaticMethodID(MainActivityClass, "triggerGamepadVibration", "(IFF)V");
}

void AndroidGamepad::Shutdown() {
    for (int i = 0; i < 4; ++i) {
        if (s_Players[i]) {
            delete s_Players[i];
            s_Players[i] = nullptr;
        }
    }
}

float AndroidGamepad::getLeftStickX() const  {
    return m_leftStickX;
}

float AndroidGamepad::getLeftStickY() const  {
    return m_leftStickY;
}

float AndroidGamepad::getRightStickX() const  {
    return m_rightStickX;
}

float AndroidGamepad::getRightStickY() const {
    return m_rightStickY;
}

float AndroidGamepad::getLeftTrigger() const  {
    return m_leftTrigger;
}

float AndroidGamepad::getRightTrigger() const {
    return m_rightTrigger;
}

bool AndroidGamepad::buttonPressed(GamepadButton button) const {
    return m_pCurrButtonStates[button] && !m_pPrevButtonStates[button];
}

bool AndroidGamepad::buttonDown(GamepadButton button) const {
    return m_pCurrButtonStates[button];
}

bool AndroidGamepad::buttonUp(GamepadButton button) const {
    return !m_pCurrButtonStates[button] && m_pPrevButtonStates[button];
}

bool AndroidGamepad::isConnected() {
    return m_connected;
}

int AndroidGamepad::getDeviceId() const {
    return m_deviceId;
}

GamepadLayout AndroidGamepad::getLayoutType() const{
    return  m_layoutType;
}

void AndroidGamepad::setLeftStickX(float leftStickX){
    m_leftStickX = leftStickX;
}

void AndroidGamepad::setLeftStickY(float leftStickY){
    m_leftStickY = leftStickY;
}

void AndroidGamepad::setRightStickX(float rightStickX){
    m_rightStickX = rightStickX;
}

void AndroidGamepad::setRightStickY(float rightStickY){
    m_rightStickY = rightStickY;
}

void AndroidGamepad::setLeftTrigger(float leftTrigger){
    m_leftTrigger = leftTrigger;
}

void AndroidGamepad::setRightTrigger(float rightTrigger){
    m_rightTrigger = rightTrigger;
}

void AndroidGamepad::setConnected(bool connected){
    m_connected = connected;
}

void AndroidGamepad::setLayoutType(GamepadLayout layoutType){
    m_layoutType = layoutType;
}

void AndroidGamepad::setDeviceId(int deviceId){
    m_deviceId = deviceId;
}

void AndroidGamepad::setButtonState(GamepadButton button, bool isPressed) {
    m_pCurrButtonStates[button] = isPressed;
}

GamepadButton AndroidGamepad::mapKeyCode(int32_t keyCode) const {
    //if(m_layoutType == GamepadLayout::STANDARD){
        switch (keyCode) {
            case AKEYCODE_BUTTON_A:      return GamepadButton::GAMEPAD_A;
            case AKEYCODE_BUTTON_B:      return GamepadButton::GAMEPAD_B;
            case AKEYCODE_BUTTON_X:      return GamepadButton::GAMEPAD_X;
            case AKEYCODE_BUTTON_Y:      return GamepadButton::GAMEPAD_Y;
            case AKEYCODE_BUTTON_L1:     return GamepadButton::GAMEPAD_LEFT_SHOULDER;
            case AKEYCODE_BUTTON_R1:     return GamepadButton::GAMEPAD_RIGHT_SHOULDER;
            case AKEYCODE_BUTTON_THUMBL: return GamepadButton::GAMEPAD_LEFT_THUMB;
            case AKEYCODE_BUTTON_THUMBR: return GamepadButton::GAMEPAD_RIGHT_THUMB;
            case AKEYCODE_BUTTON_START:  return GamepadButton::GAMEPAD_START;
            case AKEYCODE_BUTTON_SELECT: return GamepadButton::GAMEPAD_BACK;
            case AKEYCODE_DPAD_UP:       return GamepadButton::DPAD_UP;
            case AKEYCODE_DPAD_DOWN:     return GamepadButton::DPAD_DOWN;
            case AKEYCODE_DPAD_LEFT:     return GamepadButton::DPAD_LEFT;
            case AKEYCODE_DPAD_RIGHT:    return GamepadButton::DPAD_RIGHT;
            case AKEYCODE_BUTTON_L2:    return GamepadButton::GAMEPAD_LEFT_TRIGGER;
            case AKEYCODE_BUTTON_R2:    return GamepadButton::GAMEPAD_RIGHT_TRIGGER;
            default:  return GamepadButton::NONE;
        }
    /*}else{
        switch (keyCode) {
            case AKEYCODE_BUTTON_A:      return GamepadButton::GAMEPAD_X;
            case AKEYCODE_BUTTON_B:      return GamepadButton::GAMEPAD_A;
            case AKEYCODE_BUTTON_C:      return GamepadButton::GAMEPAD_B;
            case AKEYCODE_BUTTON_X:      return GamepadButton::GAMEPAD_Y;

            case AKEYCODE_BUTTON_Y:      return GamepadButton::GAMEPAD_LEFT_SHOULDER;
            case AKEYCODE_BUTTON_Z:    return GamepadButton::GAMEPAD_RIGHT_SHOULDER;

            case AKEYCODE_DPAD_UP:       return GamepadButton::DPAD_UP;
            case AKEYCODE_DPAD_DOWN:     return GamepadButton::DPAD_DOWN;
            case AKEYCODE_DPAD_LEFT:     return GamepadButton::DPAD_LEFT;
            case AKEYCODE_DPAD_RIGHT:    return GamepadButton::DPAD_RIGHT;
            case AKEYCODE_BUTTON_L1:    return GamepadButton::GAMEPAD_LEFT_TRIGGER;
            case AKEYCODE_BUTTON_R1:    return GamepadButton::GAMEPAD_RIGHT_TRIGGER;


            case AKEYCODE_BUTTON_START: return GamepadButton::GAMEPAD_RIGHT_THUMB;
            case AKEYCODE_BUTTON_SELECT: return GamepadButton::GAMEPAD_LEFT_THUMB;
            case AKEYCODE_BUTTON_R2:  return GamepadButton::GAMEPAD_START;
            case AKEYCODE_BUTTON_L2: return GamepadButton::GAMEPAD_BACK;

            default:  return GamepadButton::NONE;
        }
    }*/
}

void AndroidGamepad::reset() {
    m_deviceId = -1;
    m_connected = false;
    memset(m_pCurrButtonStates, false, 21);
    memset(m_pPrevButtonStates, false, 21);
    m_leftStickX = m_leftStickY = m_rightStickX = m_rightStickY = 0.0f;
    m_leftTrigger = m_rightTrigger = 0.0f;
}

float AndroidGamepad::ApplyDeadzone(float value) {
    if (std::fabs(value) < Deadzone) return 0.0f;
    return (value > 0.0f) ? (value - Deadzone) / (1.0f - Deadzone)
                          : (value + Deadzone) / (1.0f - Deadzone);
}