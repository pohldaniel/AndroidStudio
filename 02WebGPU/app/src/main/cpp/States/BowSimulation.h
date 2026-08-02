#pragma once

#include <webgpu.h>

#include <WebGPU/WgpData.h>
#include <WebGPU/WgpTexture.h>
#include <WebGPU/WgpBuffer.h>
#include <WebGPU/WgpModel.h>

#include <Nuklear/NkContext.h>

#include <Shape/Shape.h>
#include <States/StateMachine.h>

#include <core/Camera.h>
#include <core/AssimpModel.h>
#include <core/ObjModel.h>
#include <core/TrackBall.h>

class BowSimulation : public State {

	struct JoystickResult {
		float x = 0.0f;
		float y = 0.0f;
		bool is_active = false;
	};

public:

	struct Reticle {
		float x = 0.0f;          // Aktuelle X-Position auf dem Screen
		float y = 0.0f;          // Aktuelle Y-Position auf dem Screen
		float speed = 400.0f;    // Geschwindigkeit der Fadenkreuz-Bewegung (Pixel/Sekunde)

		// Zittern/Schwanken (Sway)
		float sway_time = 0.0f;
		float sway_intensity = 15.0f; // Wie weit das Fadenkreuz von alleine abdriftet

		// Bogen-Spann-Status
		float focus_radius = 40.0f;   // Aktueller Radius des äußeren Rings
		float min_focus = 15.0f;      // Maximaler Fokus (kleiner Kreis)
		float max_focus = 50.0f;      // Entspannter Zustand
	};

	BowSimulation(StateMachine& machine);
    ~BowSimulation();

    void fixedUpdate() override;
    void update() override;
    void render() override;
    void resize(int deltaW, int deltaH) override;

    void OnDraw(const WGPUCommandEncoder& commandEncoder, const WGPURenderPassDescriptor& renderPassDescriptor);
	void OnFillBuffer(nk_context& nkCntxt);
    void OnButton(const Event::MouseButtonEvent& event) override;

private:

    std::vector<WGPUBindGroupLayout> OnBindGroupLayouts();
	void renderUi(const WGPURenderPassEncoder& renderPassEncoder);

	bool m_initUi = true;
	bool m_drawUi = false;
	bool m_isHovered = false;
	float m_uiScale = 1.0f;
	float m_scrollDelta = 0.0f;
	bool m_wasHovered1 = false;
	bool m_wasHovered2 = false;

	Camera m_camera;
	TrackBall m_trackball;

	WgpBuffer m_vertexBuffer, m_indexBuffer, m_uniformBuffer;
	WgpTexture m_texture, m_textureFont, m_textureIcon;

	WGPURenderBundle m_renderBundle;
	WGPUBindGroup m_bindgroup, m_bindgroupFont, m_bindgroupIcon;

	WGPUBindGroup createBindGroup();
	WGPUBindGroup createBindGroupFont();
	WGPUBindGroup createBindGroupIcon();
	JoystickResult nk_virtual_joystick(struct nk_context* ctx, float size_px, int touch_id);
	bool nk_circular_action_button(struct nk_context* ctx, const char* label, float size_px, int touch_id);
	void init_reticle(Reticle& r, float screen_w, float screen_h);
	void update_reticle(Reticle& r, float joystick_x, float joystick_y, bool is_drawing_bow, float dt, float screen_w, float screen_h);
	void draw_reticle(struct nk_context* ctx, const Reticle& r, float screen_w, float screen_h);

	struct nk_image playIcon;
	struct nk_vec2 current_pos;

	const float BASE_ROW_DYN = 30.0f;
	const float BASE_ROW_STAT = 32.0f;
};