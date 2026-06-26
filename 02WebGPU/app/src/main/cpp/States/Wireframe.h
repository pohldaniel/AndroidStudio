#pragma once

#include <webgpu.h>

#include <WebGpu/WgpData.h>
#include <WebGPU/WgpTexture.h>
#include <WebGPU/WgpBuffer.h>
#include <WebGPU/WgpModel.h>

#include <Shape/Shape.h>
#include <States/StateMachine.h>

#include <core/Camera.h>

class Wireframe : public State {

public:
    Wireframe(StateMachine& machine);
    ~Wireframe() override;

    void fixedUpdate() override;
    void update() override;
    void render() override;
    void OnDraw(const WGPUCommandEncoder& commandEncoder, const WGPURenderPassDescriptor& renderPassDescriptor);

private:

    std::vector<WGPUBindGroupLayout> OnBindGroupLayouts();
    std::vector<WGPUBindGroup> OnBindGroups();

    Shape m_quad;
    Uniforms m_uniforms;

    WgpTexture m_wgpTexture;
    WgpModel m_wgpQuad;
    WgpBuffer m_wgpBuffer;

    WGPUBindGroup m_bindgroup;
};