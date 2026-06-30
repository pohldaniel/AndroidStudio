#pragma once

#include <webgpu.h>

#include <WebGpu/WgpData.h>
#include <WebGPU/WgpTexture.h>
#include <WebGPU/WgpBuffer.h>
#include <WebGPU/WgpModel.h>

#include <Shape/Shape.h>
#include <States/StateMachine.h>

#include <core/animation/AnimatedModel.h>
#include <core/AssimpModel.h>
#include <core/Camera.h>

#define MAX_JOIN 96u

class Collada : public State {

public:
    Collada(StateMachine& machine);
    ~Collada() override;

    void fixedUpdate() override;
    void update() override;
    void render() override;
    void resize(int deltaW, int deltaH) override;

    void OnDraw(const WGPUCommandEncoder& commandEncoder, const WGPURenderPassDescriptor& renderPassDescriptor);
    void OnButton() override;

private:

    std::vector<WGPUBindGroupLayout> OnBindGroupLayouts();
    std::vector<WGPUBindGroup> OnBindGroups();

    Camera m_camera;
    Uniforms m_uniforms;
    AnimatedModel m_cowboy;
    Animation m_run;

    WgpBuffer m_wgpBuffer, m_wgpSkinBuffer;
    WgpModel m_wgpCowboy;
    WgpTexture m_wgpTexture;
};