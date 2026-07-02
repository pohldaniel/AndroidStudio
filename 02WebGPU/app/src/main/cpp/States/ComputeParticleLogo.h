#pragma once

#include <webgpu.h>

#include <WebGpu/WgpData.h>
#include <WebGPU/WgpTexture.h>
#include <WebGPU/WgpBuffer.h>
#include <WebGPU/WgpModel.h>

#include <Shape/Shape.h>
#include <States/StateMachine.h>

#include <core/Camera.h>
#include <core/AssimpModel.h>
#include <core/ObjModel.h>
#include <core/TrackBall.h>

#define PARTICLE_NUM (50000u)

class ComputeParticleLogo : public State {

    struct RenderParams {
        glm::mat4 model_view_projection_matrix;
        glm::vec3 right;
        float pad1;
        glm::vec3 up;
        float pad2;
    };

    struct Seed {
        float x;
        float y;
        float z;
        float w;
    };

    struct ParticleData {
        float delta_time;
        float brightness_factor;
        float pad[2];
        Seed seed;
    };

public:

    ComputeParticleLogo(StateMachine& machine);
    ~ComputeParticleLogo() override;

    void fixedUpdate() override;
    void update() override;
    void render() override;
    void resize(int deltaW, int deltaH) override;

    void OnDraw(const WGPUCommandEncoder& commandEncoder, const WGPURenderPassDescriptor& renderPassDescriptor);
    void OnButton(const Event::MouseButtonEvent& event) override;

private:

    std::vector<WGPUBindGroupLayout> OnBindGroupLayoutsProbability();
    std::vector<WGPUBindGroupLayout> OnBindGroupLayoutsSimulate();
    std::vector<WGPUBindGroupLayout> OnBindGroupLayoutsParticle();

    WGPUBindGroup createComputeBindGroup();
    WGPUBindGroup createBindGroup();
    void updateSimulation();
    float randomFloat(float min, float max);
    float randomFloat();

    bool m_initUi = true;
    bool m_drawUi = false;

    Camera m_camera;
    ParticleData particleData;
    RenderParams m_renderParams;

    WgpTexture m_wgpWgpuLogo;
    WgpBuffer m_probabilityBuffer, m_bufferA, m_bufferB, m_simulationBuffer, m_particlesBuffer, m_renderParamsBuffer, m_quadVerticesBuffer;
    WGPUBindGroup m_computeBindGroup, m_bindGroup;
};