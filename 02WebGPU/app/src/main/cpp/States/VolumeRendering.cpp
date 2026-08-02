#include <WebGPU/WgpContext.h>
#include <WebGPU/WgpRenderer.h>
#include <States/ComputeParticleLogo.h>
#include <States/Isometric.h>

#include "AssetIO.h"
#include "VolumeRendering.h"

VolumeRendering::VolumeRendering(StateMachine& machine) : State(machine, States::VOLUME_RENDERING) {

    wgpSetMSAASampleCount(4u);

    m_camera.perspective(glm::radians(72.0f), static_cast<float>(wgpWidth) / static_cast<float>(wgpHeight), m_near, m_far);
    m_camera.orthographic(0.0f, static_cast<float>(wgpWidth), 0.0f, static_cast<float>(wgpHeight), -1.0f, 1.0f);
    m_camera.lookAt(4.0f, 0.0f, 0.0f);
    m_camera.setMovingSpeed(20.0f);
    m_camera.setRotationSpeed(1.0f);

    wgpContext.setClearColor({ 0.0f, 1.0f, 0.0f, 1.0f });
    wgpContext.addSampler(wgpCreateSampler(WGPUFilterMode_Linear, WGPUAddressMode_ClampToEdge, 16u, WGPUMipmapFilterMode_Linear), SS_0);

    m_uniformBuffer.createBuffer(sizeof(glm::mat4), WGPUBufferUsage_CopyDst | WGPUBufferUsage_Uniform);
    m_volumeTexture.loadFromFile("textures/t1_icbm_normal_1mm_pn0_rf0_180x216x180_uint8_1x1.bin", 180u, 216u, 180u);

    wgpContext.addSahderModule("VOLUME", "shader/volume.wgsl");
    wgpContext.createRenderPipeline("VOLUME", "RP_VOLUME", VL_NONE, std::bind(&VolumeRendering::OnBindGroupLayoutsVolume, this), 4u);

    wgpContext.OnDraw = std::bind(&VolumeRendering::OnDraw, this, std::placeholders::_1, std::placeholders::_2);
    m_volumeBindGroup = createVolumeBindGroup();
}

VolumeRendering::~VolumeRendering() {
    m_uniformBuffer.markForDelete();
    m_volumeTexture.markForDelete();

    wgpuBindGroupRelease(m_volumeBindGroup);
}

void VolumeRendering::fixedUpdate() {

}

void VolumeRendering::update() {
    m_trackball.idle();

    if (m_rotate) {
        m_rotation += m_dt;
        m_camera.lookAt(4.0f, glm::degrees(sinf(m_rotation)), glm::degrees(cosf(m_rotation)));
    }
    glm::mat4 invVP = m_camera.getInvViewMatrix() * m_camera.getInvPerspectiveMatrix();
    wgpuQueueWriteBuffer(wgpContext.queue, m_uniformBuffer.getBuffer(), 0u, &invVP, sizeof(glm::mat4));
}

void VolumeRendering::render() {
    wgpDraw();
}

void VolumeRendering::OnDraw(const WGPUCommandEncoder& commandEncoder, const WGPURenderPassDescriptor& renderPassDescriptor) {
    {
        WGPURenderPassEncoder renderPassEncoder = wgpuCommandEncoderBeginRenderPass(commandEncoder, &renderPassDescriptor);
        wgpuRenderPassEncoderSetPipeline(renderPassEncoder, wgpContext.renderPipelines.at("RP_VOLUME"));
        wgpuRenderPassEncoderSetBindGroup(renderPassEncoder, 0u, m_volumeBindGroup, 0u, 0u);
        wgpuRenderPassEncoderDraw(renderPassEncoder, 3u, 1u, 0u, 0u);
        wgpuRenderPassEncoderEnd(renderPassEncoder);
        wgpuRenderPassEncoderRelease(renderPassEncoder);
    }
}

void VolumeRendering::resize(int deltaW, int deltaH) {
    m_camera.perspective(glm::radians(72.0f), static_cast<float>(wgpWidth) / static_cast<float>(wgpHeight), m_near, m_far);
    m_camera.orthographic(0.0f, static_cast<float>(wgpWidth), 0.0f, static_cast<float>(wgpHeight), -1.0f, 1.0f);
    m_trackball.reshape(wgpWidth, wgpHeight);
}

void VolumeRendering::OnButton(const Event::MouseButtonEvent& event) {
    wgpCleanState();
    m_isRunning = false;

    if(event.button == Event::MouseButtonEvent::BUTTON_LEFT){
        m_machine.addStateAtBottom(new ComputeParticleLogo(m_machine));
    }

    if(event.button == Event::MouseButtonEvent::BUTTON_RIGHT){
        m_machine.addStateAtBottom(new Isometric(m_machine));
    }
}

std::vector<WGPUBindGroupLayout> VolumeRendering::OnBindGroupLayoutsVolume() {
    std::vector<WGPUBindGroupLayout> bindingLayouts(1);

    std::vector<WGPUBindGroupLayoutEntry> bindingLayoutEntries(3);
    bindingLayoutEntries[0].binding = 0u;
    bindingLayoutEntries[0].visibility = WGPUShaderStage_Vertex;
    bindingLayoutEntries[0].buffer.type = WGPUBufferBindingType_Uniform;
    bindingLayoutEntries[0].buffer.minBindingSize = sizeof(glm::mat4);

    bindingLayoutEntries[1].binding = 1u;
    bindingLayoutEntries[1].visibility = WGPUShaderStage_Fragment;
    bindingLayoutEntries[1].sampler.type = WGPUSamplerBindingType_Filtering;

    bindingLayoutEntries[2].binding = 2u;
    bindingLayoutEntries[2].visibility = WGPUShaderStage_Fragment;
    bindingLayoutEntries[2].texture.viewDimension = WGPUTextureViewDimension_3D;
    bindingLayoutEntries[2].texture.sampleType = WGPUTextureSampleType_Float;

    WGPUBindGroupLayoutDescriptor bindGroupLayoutDescriptor = {};
    bindGroupLayoutDescriptor.entryCount = (uint32_t)bindingLayoutEntries.size();
    bindGroupLayoutDescriptor.entries = bindingLayoutEntries.data();

    bindingLayouts[0] = wgpuDeviceCreateBindGroupLayout(wgpContext.device, &bindGroupLayoutDescriptor);

    return bindingLayouts;
}

WGPUBindGroup VolumeRendering::createVolumeBindGroup() {
    std::vector<WGPUBindGroupEntry> bindGroupEntries(3);

    bindGroupEntries[0].binding = 0u;
    bindGroupEntries[0].buffer = m_uniformBuffer.getBuffer();
    bindGroupEntries[0].size = wgpuBufferGetSize(m_uniformBuffer.getBuffer());

    bindGroupEntries[1].binding = 1u;
    bindGroupEntries[1].sampler = wgpContext.getSampler(SS_0);

    bindGroupEntries[2].binding = 2u;
    bindGroupEntries[2].textureView = m_volumeTexture.getTextureView();

    WGPUBindGroupDescriptor bindGroupDesc = {};
    bindGroupDesc.layout = wgpuRenderPipelineGetBindGroupLayout(wgpContext.renderPipelines.at("RP_VOLUME"), 0u);
    bindGroupDesc.entryCount = (uint32_t)bindGroupEntries.size();
    bindGroupDesc.entries = bindGroupEntries.data();

    return wgpuDeviceCreateBindGroup(wgpContext.device, &bindGroupDesc);
}