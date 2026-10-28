#pragma once

#include <webgpu.h>

#include <WebGPU/WgpData.h>
#include <WebGPU/WgpTexture.h>
#include <WebGPU/WgpBuffer.h>
#include <WebGPU/WgpModel.h>

#include <States/StateMachine.h>
#include <Shape/Shape.h>

#include <core/Camera.h>


struct BloomMip {
    WGPUTextureView renderTargetView;
    WGPUBuffer downbuffer;
    WGPUBindGroup downsampleBindGroup;
    WGPUBindGroup upsampleBindGroup;
};

struct BloomFBO {

    struct DownsampleUniforms {
        std::array<float, 2> srcResolution;
        uint32_t mipLevel;
        uint32_t padding;
    };

    struct UpsampleUniforms {
        float filterRadius;
        float padding[3];
    };

    void init(WGPUDevice device, uint32_t width, uint32_t height, uint32_t mipChainLength, float filterRadius = 0.005f);
    void createBindGroups(WGPUDevice device, WGPUBindGroupLayout layout, WGPUSampler sampler, WGPUTextureView sceneColorView);
    void resize(uint32_t width, uint32_t height);
    void cleanup();
    bool m_karisAverageOnDownsample = true;
    std::vector<BloomMip> mMipChain;
    WgpTexture m_texture;
    WGPUBuffer upbuffer = nullptr;
};

class HDRBloom : public State {
    struct Instance {
        glm::mat4 model;
        glm::vec4 color;
    };
    struct CompositeUniforms {
        float exposure = 1.0f;
        float bloomStrength = 0.04f;
        float padding[2] = { 0.0f, 0.0f };
    };

    struct Light {
        glm::vec3 position;
        float pad;
        glm::vec4 color;
    };
public:

    HDRBloom(StateMachine& machine);
    ~HDRBloom() override;

    void fixedUpdate() override;
    void update() override;
    void render() override;
    void OnDraw(const WGPUCommandEncoder& commandEncoder, const WGPURenderPassDescriptor& renderPassDescriptor);
    void OnPostDraw();

    void OnButton(const Event::MouseButtonEvent& event) override;
    void resize(int deltaW, int deltaH) override;

private:

    std::vector<WGPUBindGroupLayout> OnBindGroupLayouts();
    std::vector<WGPUBindGroupLayout> OnBindGroupLayoutsSampler();
    std::vector<WGPUBindGroupLayout> OnBindGroupLayoutsComposite();

    WGPUBindGroup createBindGroup(const WgpTexture& texture);
    WGPUBindGroup createBindGroupComposite();

    void renderUi(const WGPURenderPassEncoder& renderPassEncoder);

    bool m_initUi = true;
    bool m_drawUi = false;
    bool m_wantResize = false;
    Camera m_camera;
    Uniforms m_uniforms;
    CompositeUniforms m_compositeUniforms;
    Shape m_cube, m_cubeEmessive;

    WgpBuffer m_uniformBuffer, m_storageBuffer, m_compositeBuffer, m_lightBuffer;
    WgpModel m_wgpCube, m_wgpCubeEmessive;
    WgpTexture m_wgpSceneTarget, m_wgpBrightnessTarget, m_wgpSceneDepth;
    WgpTexture m_wgpTextureEmpty, m_wgpTextureWood, m_wgpTextureContainer;
    WGPUBindGroup m_bindGroupComposite;

    std::vector<Instance> m_instances;
    std::vector<Light> m_lights;

    BloomFBO m_bloomFBO;
    std::vector<WGPURenderPassColorAttachment> m_renderPassColorAttachments;
};