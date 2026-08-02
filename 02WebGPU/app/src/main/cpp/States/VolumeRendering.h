#pragma once

#include <webgpu.h>

#include <WebGPU/WgpData.h>
#include <WebGPU/WgpTexture.h>
#include <WebGPU/WgpBuffer.h>
#include <WebGPU/WgpModel.h>

#include <Shape/Shape.h>
#include <States/StateMachine.h>

#include <core/Camera.h>
#include <core/AssimpModel.h>
#include <core/ObjModel.h>
#include <core/TrackBall.h>

class VolumeRendering : public State {

public:

    VolumeRendering(StateMachine& machine);
    ~VolumeRendering();

    void fixedUpdate() override;
    void update() override;
    void render() override;
    void resize(int deltaW, int deltaH) override;

    void OnDraw(const WGPUCommandEncoder& commandEncoder, const WGPURenderPassDescriptor& renderPassDescriptor);
    void OnButton(const Event::MouseButtonEvent& event) override;

private:

    std::vector<WGPUBindGroupLayout> OnBindGroupLayoutsVolume();
    WGPUBindGroup createVolumeBindGroup();

    bool m_initUi = true;
    bool m_drawUi = true;
    float m_rotation = 0.0f;
    bool m_rotate = true;
    float m_near = 4.3f;
    float m_far = 4.4f;

    Camera m_camera;
    TrackBall m_trackball;

    WgpBuffer m_uniformBuffer;
    WgpTexture m_volumeTexture;
    WGPUBindGroup m_volumeBindGroup;
};