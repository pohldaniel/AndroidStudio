#pragma once

#include <webgpu.h>

#include <WebGPU/WgpData.h>
#include <WebGPU/WgpTexture.h>
#include <WebGPU/WgpBuffer.h>
#include <WebGPU/WgpModel.h>

#include <States/StateMachine.h>
#include <Nuklear/NkJoystick.h>
#include <Shape/Shape.h>

#include <core/scene/SceneNode.h>
#include <core/scene/CollisionNode.h>
#include <core/Camera.h>
#include <core/TrackBall.h>
#include <core/Transform.h>

class Controller : public State {

public:

    Controller(StateMachine& machine);
    ~Controller();

    void fixedUpdate() override;
    void update() override;
    void render() override;
    void OnDraw(const WGPUCommandEncoder& commandEncoder, const WGPURenderPassDescriptor& renderPassDescriptor);

    void OnButton(const Event::MouseButtonEvent& event) override;
    void OnReEnter(unsigned int prevState) override;
    void resize(int deltaW, int deltaH) override;

private:

    std::vector<WGPUBindGroupLayout> OnBindGroupLayouts();
    std::vector<WGPUBindGroup> OnBindGroups();

    void renderRectangle(float x, float y);
    void renderPad(float x, float y, long value);
    void renderButton(float x, float y, long value, int index);
    void renderStick(float x, float y, float stickX, float stickY);

    long m_state = 0;
    float m_RightStickX = 0.0f;
    float m_RightStickY = 0.0f;
    float m_LeftStickX = 0.0f;
    float m_LeftStickY = 0.0f;

    Camera m_camera;
    Uniforms m_uniforms;
    Shape m_disk, m_quad;

    WgpBuffer m_uniformBuffer, m_storageBuffer;
    WgpModel m_wgpDisk, m_wgpQuad;
    std::vector<std::array<float, 20>> m_instances;
};