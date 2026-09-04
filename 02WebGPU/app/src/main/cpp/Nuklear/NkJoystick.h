#pragma once

struct JoystickResult {
	void reset();
	float x = 0.0f;
	float y = 0.0f;
	bool isActive = false;

};

struct RotationResult {
	float degrees;
	bool isActive;
};

struct RotationButtonResult {
	float degrees;
	bool isRotating;
	bool buttonPressed;
	bool buttonDown;
	bool isActive = false;
};

extern "C" {

	void virtual_joystick(struct nk_rect dimension, int touch_id, JoystickResult& out);
	void nk_virtual_joystick(struct nk_context* ctx, float size_px, int touch_id, JoystickResult& out);

	void action_button(struct nk_rect dimension, bool& isPressed);
	bool nk_action_button(struct nk_context* ctx, const char* label, float size_px, bool& isPressed);

	void virtual_rotation(struct nk_rect dimension, RotationResult& out);
	void nk_virtual_rotation(struct nk_context* ctx, float size_px, RotationResult& out);

	void virtual_rotation_button(struct nk_rect dimension, int touch_id, RotationButtonResult& out);
	void nk_virtual_rotation_button(struct nk_context* ctx, float size_px, int touch_id, RotationButtonResult& out);

	bool rounded_button(struct nk_rect dimension, const char* label, int touch_id, bool& isPressed);
	void nk_rounded_button_logic(struct nk_context* ctx, const char* label, int touch_id, bool& isPressed);
}