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

#define MAX_NUM_LIGHTS 1024u

class DeferredRendering : public State {

public:

    DeferredRendering(StateMachine& machine);
    ~DeferredRendering() override;

    void fixedUpdate() override;
    void update() override;
    void render() override;
    void resize(int deltaW, int deltaH) override;

    void OnDraw(const WGPUCommandEncoder& commandEncoder, const WGPURenderPassDescriptor& renderPassDescriptor);
    void OnButton(const Event::MouseButtonEvent& event) override;

private:

    std::vector<WGPUBindGroupLayout> OnBindGroupLayoutsGBuffer();
    std::vector<WGPUBindGroupLayout> OnBindGroupLayoutsCompute();
    std::vector<WGPUBindGroupLayout> OnBindGroupLayoutsDeferred();
    std::vector<WGPUBindGroupLayout> OnBindGroupLayoutsDeferredDebug();

    std::vector<WGPUBindGroup> OnBindGroupsGBuffer();
    WGPUBindGroup createDeferredBindGroup();
    WGPUBindGroup createLightBindGroup();
    WGPUBindGroup createComputeBindGroup();

    float randomFloat(float min, float max);

    bool m_debug = false;
    int m_numLights = 256;

    Camera m_camera;
    TrackBall m_trackball;
    AssimpModel m_dragon;
    Shape m_quad;

    WgpBuffer m_uniformBuffer, m_cameraBuffer, m_lightBuffer, m_configBuffer, m_extentBuffer;
    WgpModel m_wgpDragon, m_wgpQuad;
    WgpTexture m_normalTexture, m_albedoTexture, m_depthTexture;
    WGPUBindGroup m_deferredBindGroup, m_lightBindGroup, m_computeBindGroup;

    std::vector<WGPURenderPassColorAttachment> renderPassColorAttachments;
    WGPURenderPassDepthStencilAttachment renderPassDepthStencilAttachment;

    static glm::vec3& RotateY(glm::vec3& p, float rad, const glm::vec3& centerOfRotation = glm::vec3(0.0f, 0.0f, 0.0f));
};