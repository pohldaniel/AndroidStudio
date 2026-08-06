#include <WebGPU/WgpContext.h>
#include <States/Wireframe.h>
#include <States/ComputeParticleLogo.h>

#include "DeferredRendering.h"

DeferredRendering::DeferredRendering(StateMachine& machine) : State(machine, States::DEFERRED_RENDERING) {

    m_camera.perspective(glm::radians(72.0f), static_cast<float>(wgpWidth) / static_cast<float>(wgpHeight), 0.1f, 1000.0f);
    m_camera.orthographic(0.0f, static_cast<float>(wgpWidth), 0.0f, static_cast<float>(wgpHeight), -1.0f, 1.0f);
    m_camera.lookAt(glm::vec3(1.0f, 2.0f, 4.0f), glm::vec3(0.2f, 0.2f, 1.5f) + glm::vec3(0.0f, 0.0f, -1.0f), glm::vec3(0.0f, 1.0f, 0.0f));
    m_camera.lookAt(glm::vec3(0.0f, 75.0f, -150.0f), glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(0.0f, 1.0f, 0.0f));
    m_camera.setMovingSpeed(20.0f);
    m_camera.setRotationSpeed(0.1f);

    m_dragon.loadModel("models/dragon_vrip_res4.ply");
	m_dragon.scale(500.0f);
	m_dragon.translate(0.0f, -45.0, 0.0f);
    m_dragon.generateNormals();
    m_dragon.generateUVs();

    m_quad.buildQuadXZ({ -100.0f, -25.0f, -100.0f }, { 200.0f, 200.0f }, 1u, 1u, true, true);

    m_trackball.reshape(wgpWidth, wgpHeight);

    wgpContext.setClearColor({ 0.0f, 0.0f, 0.0f, 1.0f });

    m_uniformBuffer.createBuffer(2u * sizeof(glm::mat4), WGPUBufferUsage_CopyDst | WGPUBufferUsage_Uniform);
    wgpuQueueWriteBuffer(wgpContext.queue, m_uniformBuffer.getBuffer(), 0u, &Camera::IDENTITY, sizeof(glm::mat4));
    wgpuQueueWriteBuffer(wgpContext.queue, m_uniformBuffer.getBuffer(), 16u * sizeof(float), &Camera::IDENTITY, sizeof(glm::mat4));

    m_cameraBuffer.createBuffer(2u * sizeof(glm::mat4), WGPUBufferUsage_CopyDst | WGPUBufferUsage_Uniform);

    glm::mat4 VP = m_camera.getPerspectiveMatrix() * m_camera.getViewMatrix();
    glm::mat4 invVP = m_camera.getInvViewMatrix() * m_camera.getInvPerspectiveMatrix();

    wgpuQueueWriteBuffer(wgpContext.queue, m_cameraBuffer.getBuffer(), 0u, &VP, sizeof(glm::mat4));
    wgpuQueueWriteBuffer(wgpContext.queue, m_cameraBuffer.getBuffer(), 64u, &invVP, sizeof(glm::mat4));

    m_configBuffer.createBuffer(sizeof(uint32_t), WGPUBufferUsage_CopyDst | WGPUBufferUsage_Uniform);
    wgpuQueueWriteBuffer(wgpContext.queue, m_configBuffer.getBuffer(), 0u, &m_numLights, sizeof(uint32_t));

    m_lightBuffer.createBuffer(sizeof(float) * 8u * MAX_NUM_LIGHTS, WGPUBufferUsage_Storage, true);

    glm::vec3 light_extent_min = { -50.f, -30.f, -50.f };
    glm::vec3 light_extent_max = { 50.f, 100.f, 50.f };
    glm::vec3 extent = light_extent_max - light_extent_min;

    float* light_data = (float*)wgpuBufferGetMappedRange(m_lightBuffer.getBuffer(), 0u, sizeof(float) * 8u * MAX_NUM_LIGHTS);
    glm::vec4 tmp_vec4 = {0.0f, 0.0f, 0.0f, 0.0f};

    for (uint32_t i = 0, offset = 0; i < MAX_NUM_LIGHTS; ++i) {
        offset = 8u * i;
        // position
        for (uint8_t j = 0; j < 3; j++) {
            tmp_vec4[j] = randomFloat(0.0f, 1.0f) * extent[j] + light_extent_min[j];
        }
        tmp_vec4[3] = 1.0f;
        memcpy(&light_data[offset], &tmp_vec4[0], sizeof(glm::vec4));
        // color
        tmp_vec4[0] = randomFloat(0.0f, 1.0f) * 2.0f;
        tmp_vec4[1] = randomFloat(0.0f, 1.0f) * 2.0f;
        tmp_vec4[2] = randomFloat(0.0f, 1.0f) * 2.0f;
        // radius
        tmp_vec4[3] = 20.0f;
        memcpy(&light_data[offset + 4], &tmp_vec4[0], sizeof(glm::vec4));
    }
    wgpuBufferUnmap(m_lightBuffer.getBuffer());

    m_extentBuffer.createBuffer(4u * 8u, WGPUBufferUsage_CopyDst | WGPUBufferUsage_Uniform);
    float light_extent_data[8] = { 0 };
    memcpy(&light_extent_data[0], &light_extent_min, sizeof(glm::vec3));
    memcpy(&light_extent_data[4], &light_extent_max, sizeof(glm::vec3));
    wgpuQueueWriteBuffer(wgpContext.queue, m_extentBuffer.getBuffer(), 0u, &light_extent_data, 32u);

    wgpContext.addSahderModule("DEFERRED", "shader/deferred.wgsl");
    wgpContext.createRenderPipeline("DEFERRED", "RP_DEFERRED", VL_NONE, std::bind(&DeferredRendering::OnBindGroupLayoutsDeferred, this));

    wgpContext.addSahderModule("DEFERRED_DEBUG", "shader/deferred_debug.wgsl");
    wgpContext.createRenderPipeline("DEFERRED_DEBUG", "RP_DEFERRED_DEBUG", VL_NONE, std::bind(&DeferredRendering::OnBindGroupLayoutsDeferredDebug, this),
                                    1u,
                                    WGPUPrimitiveTopology_TriangleList,
                                    WGPUTextureFormat_Undefined,
                                    WGPUTextureFormat_Undefined,
                                    WGPUCompareFunction_Less,
                                    { WRITE_DEPTH | DEPTH_STENCIL_STATE | BLEND_STATE | FRAGMENT_STATE, BlendMode::ALPHA_BLENDING, WGPUTextureFormat_Undefined , WGPUCullMode_Undefined,  DEFAULT ,
                                      {
                                              { NULL, STRVIEW("canvasSizeWidth"), static_cast<double>(wgpWidth)   },
                                              { NULL, STRVIEW("canvasSizeHeight"), static_cast<double>(wgpHeight) }
                                      }
                                    }
    );

    wgpContext.addSahderModule("GBUFFER", "shader/deferred_gbuffer.wgsl");
    wgpContext.createRenderPipeline("GBUFFER", "RP_GBUFFER", VL_PTN, std::bind(&DeferredRendering::OnBindGroupLayoutsGBuffer, this),
                                    1u,
                                    WGPUPrimitiveTopology_TriangleList,
                                    WGPUTextureFormat_BGRA8Unorm,
                                    WGPUTextureFormat_Depth24Plus,
                                    WGPUCompareFunction_Less,
                                    { WRITE_DEPTH | DEPTH_STENCIL_STATE | BLEND_STATE | FRAGMENT_STATE, BlendMode::ALPHA_BLENDING, WGPUTextureFormat_RGBA16Float , WGPUCullMode_Undefined,  DEFAULT }
    );

    wgpContext.addSahderModule("COMPUTE", "shader/deferred_compute.wgsl");
    wgpContext.createComputePipeline("COMPUTE", "main", "CP_DEFFERED", std::bind(&DeferredRendering::OnBindGroupLayoutsCompute, this));

    wgpContext.OnDraw = std::bind(&DeferredRendering::OnDraw, this, std::placeholders::_1, std::placeholders::_2);

    m_wgpDragon.create(m_dragon);
    m_wgpDragon.setBindGroups("BG", std::bind(&DeferredRendering::OnBindGroupsGBuffer, this));

    m_wgpQuad.create(m_quad);
    m_wgpQuad.setBindGroups("BG", std::bind(&DeferredRendering::OnBindGroupsGBuffer, this));

    m_normalTexture.createEmpty(wgpWidth, wgpHeight, 1u, WGPUTextureUsage_TextureBinding | WGPUTextureUsage_RenderAttachment, WGPUTextureFormat_RGBA16Float);
    m_albedoTexture.createEmpty(wgpWidth, wgpHeight, 1u, WGPUTextureUsage_TextureBinding | WGPUTextureUsage_RenderAttachment, WGPUTextureFormat_BGRA8Unorm);
    m_depthTexture.createEmpty(wgpWidth, wgpHeight, 1u, WGPUTextureUsage_TextureBinding | WGPUTextureUsage_RenderAttachment, WGPUTextureFormat_Depth24Plus);

    WGPURenderPassColorAttachment renderPassColorAttachmentNormal = {};
    renderPassColorAttachmentNormal.view = m_normalTexture.getTextureView();
    renderPassColorAttachmentNormal.resolveTarget = NULL;
    renderPassColorAttachmentNormal.loadOp = WGPULoadOp::WGPULoadOp_Clear;
    renderPassColorAttachmentNormal.storeOp = WGPUStoreOp::WGPUStoreOp_Store;
    renderPassColorAttachmentNormal.clearValue = { 0.0f, 0.0f, 1.0f, 1.0f };
    renderPassColorAttachmentNormal.depthSlice = WGPU_DEPTH_SLICE_UNDEFINED;
    renderPassColorAttachments.push_back(renderPassColorAttachmentNormal);

    WGPURenderPassColorAttachment renderPassColorAttachmentAlbedo = {};
    renderPassColorAttachmentAlbedo.view = m_albedoTexture.getTextureView();
    renderPassColorAttachmentAlbedo.resolveTarget = NULL;
    renderPassColorAttachmentAlbedo.loadOp = WGPULoadOp::WGPULoadOp_Clear;
    renderPassColorAttachmentAlbedo.storeOp = WGPUStoreOp::WGPUStoreOp_Store;
    renderPassColorAttachmentAlbedo.clearValue = { 0.0f, 0.0f, 0.0f, 1.0f };
    renderPassColorAttachmentAlbedo.depthSlice = WGPU_DEPTH_SLICE_UNDEFINED;
    renderPassColorAttachments.push_back(renderPassColorAttachmentAlbedo);

    renderPassDepthStencilAttachment = {};
    renderPassDepthStencilAttachment.view = m_depthTexture.getTextureView();
    renderPassDepthStencilAttachment.depthClearValue = 1.0f;
    renderPassDepthStencilAttachment.depthLoadOp = WGPULoadOp::WGPULoadOp_Clear;
    renderPassDepthStencilAttachment.depthStoreOp = WGPUStoreOp::WGPUStoreOp_Store;
    renderPassDepthStencilAttachment.depthReadOnly = WGPUOptionalBool::WGPUOptionalBool_False;
    renderPassDepthStencilAttachment.stencilClearValue = 0u;
    renderPassDepthStencilAttachment.stencilLoadOp = WGPULoadOp::WGPULoadOp_Undefined;
    renderPassDepthStencilAttachment.stencilStoreOp = WGPUStoreOp::WGPUStoreOp_Undefined;
    renderPassDepthStencilAttachment.stencilReadOnly = WGPUOptionalBool::WGPUOptionalBool_True;

    m_deferredBindGroup = createDeferredBindGroup();
    m_lightBindGroup = createLightBindGroup();
    m_computeBindGroup = createComputeBindGroup();
}

DeferredRendering::~DeferredRendering() {
    m_uniformBuffer.markForDelete();
    m_cameraBuffer.markForDelete();
    m_lightBuffer.markForDelete();
    m_configBuffer.markForDelete();
    m_extentBuffer.markForDelete();

    m_normalTexture.markForDelete();
    m_albedoTexture.markForDelete();
    m_depthTexture.markForDelete();

    wgpuBindGroupRelease(m_deferredBindGroup);
    wgpuBindGroupRelease(m_lightBindGroup);
    wgpuBindGroupRelease(m_computeBindGroup);
}

void DeferredRendering::fixedUpdate() {

}

void DeferredRendering::update() {
    m_trackball.idle();

    glm::vec3 position = m_camera.getPosition();
    RotateY(position, m_dt * 0.75f);
    m_camera.setPosition(position, true);

    glm::mat4 model = m_trackball.getTransform();
    wgpuQueueWriteBuffer(wgpContext.queue, m_uniformBuffer.getBuffer(), 0, &model, sizeof(glm::mat4));
    wgpuQueueWriteBuffer(wgpContext.queue, m_uniformBuffer.getBuffer(), 64u, &Camera::IDENTITY, sizeof(glm::mat4));

    glm::mat4 VP = m_camera.getPerspectiveMatrix() * m_camera.getViewMatrix();
    glm::mat4 invVP = m_camera.getInvViewMatrix() * m_camera.getInvPerspectiveMatrix();

    wgpuQueueWriteBuffer(wgpContext.queue, m_cameraBuffer.getBuffer(), 0, &VP, sizeof(glm::mat4));
    wgpuQueueWriteBuffer(wgpContext.queue, m_cameraBuffer.getBuffer(), 64u, &invVP, sizeof(glm::mat4));
}

void DeferredRendering::render() {
    wgpDraw();
}

void DeferredRendering::OnDraw(const WGPUCommandEncoder& commandEncoder, const WGPURenderPassDescriptor& renderPassDescriptor) {
    {
        WGPURenderPassDescriptor rndrPssDscrptor = renderPassDescriptor;
        rndrPssDscrptor.colorAttachments = renderPassColorAttachments.data();
        rndrPssDscrptor.colorAttachmentCount = renderPassColorAttachments.size();
        rndrPssDscrptor.depthStencilAttachment = &renderPassDepthStencilAttachment;

        WGPURenderPassEncoder renderPassEncoder = wgpuCommandEncoderBeginRenderPass(commandEncoder, &rndrPssDscrptor);
        wgpuRenderPassEncoderSetViewport(renderPassEncoder, 0.0f, 0.0f, static_cast<float>(wgpWidth), static_cast<float>(wgpHeight), 0.0f, 1.0f);
        wgpuRenderPassEncoderSetPipeline(renderPassEncoder, wgpContext.renderPipelines.at("RP_GBUFFER"));

        m_wgpDragon.draw(renderPassEncoder);
        m_wgpQuad.draw(renderPassEncoder);

        wgpuRenderPassEncoderEnd(renderPassEncoder);
        wgpuRenderPassEncoderRelease(renderPassEncoder);
    }

    {
        WGPUComputePassEncoder computePassEncoder = wgpuCommandEncoderBeginComputePass(commandEncoder, NULL);
        wgpuComputePassEncoderSetPipeline(computePassEncoder, wgpContext.computePipelines.at("CP_DEFFERED"));
        wgpuComputePassEncoderSetBindGroup(computePassEncoder, 0, m_computeBindGroup, 0, NULL);
        wgpuComputePassEncoderDispatchWorkgroups(computePassEncoder, (uint32_t)ceilf(m_numLights / 64.f), 1u, 1u);
        wgpuComputePassEncoderEnd(computePassEncoder);
        wgpuComputePassEncoderRelease(computePassEncoder);
    }

    if (m_debug) {
        WGPURenderPassEncoder renderPassEncoder = wgpuCommandEncoderBeginRenderPass(commandEncoder, &renderPassDescriptor);
        wgpuRenderPassEncoderSetViewport(renderPassEncoder, 0.0f, 0.0f, static_cast<float>(wgpWidth), static_cast<float>(wgpHeight), 0.0f, 1.0f);
        wgpuRenderPassEncoderSetPipeline(renderPassEncoder, wgpContext.renderPipelines.at("RP_DEFERRED_DEBUG"));
        wgpuRenderPassEncoderSetBindGroup(renderPassEncoder, 0u, m_deferredBindGroup, 0u, 0u);

        wgpuRenderPassEncoderDraw(renderPassEncoder, 6u, 1u, 0u, 0u);
        wgpuRenderPassEncoderEnd(renderPassEncoder);

        wgpuRenderPassEncoderRelease(renderPassEncoder);
    }else {
        WGPURenderPassEncoder renderPassEncoder = wgpuCommandEncoderBeginRenderPass(commandEncoder, &renderPassDescriptor);
        wgpuRenderPassEncoderSetViewport(renderPassEncoder, 0.0f, 0.0f, static_cast<float>(wgpWidth), static_cast<float>(wgpHeight), 0.0f, 1.0f);
        wgpuRenderPassEncoderSetPipeline(renderPassEncoder, wgpContext.renderPipelines.at("RP_DEFERRED"));
        wgpuRenderPassEncoderSetBindGroup(renderPassEncoder, 0u, m_deferredBindGroup, 0u, NULL);
        wgpuRenderPassEncoderSetBindGroup(renderPassEncoder, 1u, m_lightBindGroup, 0u, NULL);

        wgpuRenderPassEncoderDraw(renderPassEncoder, 6u, 1u, 0u, 0u);

        wgpuRenderPassEncoderEnd(renderPassEncoder);
        wgpuRenderPassEncoderRelease(renderPassEncoder);
    }
}

void DeferredRendering::resize(int deltaW, int deltaH) {
    m_camera.perspective(glm::radians(72.0f), static_cast<float>(wgpWidth) / static_cast<float>(wgpHeight), 1.0f, 2000.0f);
    m_camera.orthographic(0.0f, static_cast<float>(wgpWidth), 0.0f, static_cast<float>(wgpHeight), -1.0f, 1.0f);
    m_trackball.reshape(wgpWidth, wgpHeight);

    m_normalTexture.resize(wgpWidth, wgpHeight);
    m_albedoTexture.resize(wgpWidth, wgpHeight);
    m_depthTexture.resize(wgpWidth, wgpHeight);

    wgpuBindGroupRelease(m_deferredBindGroup);
    m_deferredBindGroup = createDeferredBindGroup();

    renderPassColorAttachments[0].view = m_normalTexture.getTextureView();
    renderPassColorAttachments[1].view = m_albedoTexture.getTextureView();
    renderPassDepthStencilAttachment.view = m_depthTexture.getTextureView();
}

void DeferredRendering::OnButton(const Event::MouseButtonEvent& event) {
    wgpCleanState();
    m_isRunning = false;

    if(event.button == Event::MouseButtonEvent::BUTTON_LEFT){
        m_machine.addStateAtBottom(new Wireframe(m_machine));
    }

    if(event.button == Event::MouseButtonEvent::BUTTON_RIGHT){
        m_machine.addStateAtBottom(new ComputeParticleLogo(m_machine));
    }
}

std::vector<WGPUBindGroupLayout> DeferredRendering::OnBindGroupLayoutsGBuffer() {
    std::vector<WGPUBindGroupLayout> bindingLayouts(1);

    std::vector<WGPUBindGroupLayoutEntry> bindingLayoutEntries(2);
    bindingLayoutEntries[0].binding = 0u;
    bindingLayoutEntries[0].visibility = WGPUShaderStage_Vertex;
    bindingLayoutEntries[0].buffer.type = WGPUBufferBindingType_Uniform;
    bindingLayoutEntries[0].buffer.minBindingSize = sizeof(glm::mat4) * 2u;

    bindingLayoutEntries[1].binding = 1u;
    bindingLayoutEntries[1].visibility = WGPUShaderStage_Vertex;
    bindingLayoutEntries[1].buffer.type = WGPUBufferBindingType_Uniform;
    bindingLayoutEntries[1].buffer.minBindingSize = sizeof(glm::mat4) * 2u;

    WGPUBindGroupLayoutDescriptor bindGroupLayoutDescriptor = {};
    bindGroupLayoutDescriptor.entryCount = (uint32_t)bindingLayoutEntries.size();
    bindGroupLayoutDescriptor.entries = bindingLayoutEntries.data();

    bindingLayouts[0] = wgpuDeviceCreateBindGroupLayout(wgpContext.device, &bindGroupLayoutDescriptor);

    return bindingLayouts;
}

std::vector<WGPUBindGroupLayout> DeferredRendering::OnBindGroupLayoutsCompute() {
    std::vector<WGPUBindGroupLayout> bindingLayouts(1);

    std::vector<WGPUBindGroupLayoutEntry> bindingLayoutEntries(3);
    bindingLayoutEntries[0].binding = 0u;
    bindingLayoutEntries[0].visibility = WGPUShaderStage_Compute;
    bindingLayoutEntries[0].buffer.type = WGPUBufferBindingType_Storage;
    bindingLayoutEntries[0].buffer.minBindingSize = sizeof(float) * 8u * MAX_NUM_LIGHTS;

    bindingLayoutEntries[1].binding = 1u;
    bindingLayoutEntries[1].visibility = WGPUShaderStage_Compute;
    bindingLayoutEntries[1].buffer.type = WGPUBufferBindingType_Uniform;
    bindingLayoutEntries[1].buffer.minBindingSize = sizeof(uint32_t);

    bindingLayoutEntries[2].binding = 2u;
    bindingLayoutEntries[2].visibility = WGPUShaderStage_Compute;
    bindingLayoutEntries[2].buffer.type = WGPUBufferBindingType_Uniform;
    bindingLayoutEntries[2].buffer.minBindingSize = 0u;

    WGPUBindGroupLayoutDescriptor bindGroupLayoutDescriptor = {};
    bindGroupLayoutDescriptor.entryCount = (uint32_t)bindingLayoutEntries.size();
    bindGroupLayoutDescriptor.entries = bindingLayoutEntries.data();

    bindingLayouts[0] = wgpuDeviceCreateBindGroupLayout(wgpContext.device, &bindGroupLayoutDescriptor);

    return bindingLayouts;
}

std::vector<WGPUBindGroupLayout> DeferredRendering::OnBindGroupLayoutsDeferred() {
    std::vector<WGPUBindGroupLayout> bindingLayouts(2);

    std::vector<WGPUBindGroupLayoutEntry> bindingLayoutEntries0(3);
    bindingLayoutEntries0[0].binding = 0u;
    bindingLayoutEntries0[0].visibility = WGPUShaderStage_Fragment;
    bindingLayoutEntries0[0].texture.viewDimension = WGPUTextureViewDimension_2D;
    bindingLayoutEntries0[0].texture.sampleType = WGPUTextureSampleType_UnfilterableFloat;

    bindingLayoutEntries0[1].binding = 1u;
    bindingLayoutEntries0[1].visibility = WGPUShaderStage_Fragment;
    bindingLayoutEntries0[1].texture.viewDimension = WGPUTextureViewDimension_2D;
    bindingLayoutEntries0[1].texture.sampleType = WGPUTextureSampleType_UnfilterableFloat;

    bindingLayoutEntries0[2].binding = 2u;
    bindingLayoutEntries0[2].visibility = WGPUShaderStage_Fragment;
    bindingLayoutEntries0[2].texture.viewDimension = WGPUTextureViewDimension_2D;
    bindingLayoutEntries0[2].texture.sampleType = WGPUTextureSampleType_Depth;

    WGPUBindGroupLayoutDescriptor bindGroupLayoutDescriptor0 = {};
    bindGroupLayoutDescriptor0.entryCount = (uint32_t)bindingLayoutEntries0.size();
    bindGroupLayoutDescriptor0.entries = bindingLayoutEntries0.data();

    bindingLayouts[0] = wgpuDeviceCreateBindGroupLayout(wgpContext.device, &bindGroupLayoutDescriptor0);

    std::vector<WGPUBindGroupLayoutEntry> bindingLayoutEntries1(3);

    bindingLayoutEntries1[0].binding = 0u;
    bindingLayoutEntries1[0].visibility = WGPUShaderStage_Fragment | WGPUShaderStage_Compute;
    bindingLayoutEntries1[0].buffer.type = WGPUBufferBindingType_ReadOnlyStorage;
    bindingLayoutEntries1[0].buffer.minBindingSize = sizeof(float) * 8u * MAX_NUM_LIGHTS;

    bindingLayoutEntries1[1].binding = 1u;
    bindingLayoutEntries1[1].visibility = WGPUShaderStage_Fragment | WGPUShaderStage_Compute;
    bindingLayoutEntries1[1].buffer.type = WGPUBufferBindingType_Uniform;
    bindingLayoutEntries1[1].buffer.minBindingSize = sizeof(uint32_t);

    bindingLayoutEntries1[2].binding = 2u;
    bindingLayoutEntries1[2].visibility = WGPUShaderStage_Fragment;
    bindingLayoutEntries1[2].buffer.type = WGPUBufferBindingType_Uniform;
    bindingLayoutEntries1[2].buffer.minBindingSize = sizeof(glm::mat4) * 2u;

    WGPUBindGroupLayoutDescriptor bindGroupLayoutDescriptor1 = {};
    bindGroupLayoutDescriptor1.entryCount = (uint32_t)bindingLayoutEntries1.size();
    bindGroupLayoutDescriptor1.entries = bindingLayoutEntries1.data();

    bindingLayouts[1] = wgpuDeviceCreateBindGroupLayout(wgpContext.device, &bindGroupLayoutDescriptor1);

    return bindingLayouts;
}

std::vector<WGPUBindGroupLayout> DeferredRendering::OnBindGroupLayoutsDeferredDebug() {
    std::vector<WGPUBindGroupLayout> bindingLayouts(1);

    std::vector<WGPUBindGroupLayoutEntry> bindingLayoutEntries(3);
    bindingLayoutEntries[0].binding = 0u;
    bindingLayoutEntries[0].visibility = WGPUShaderStage_Fragment;
    bindingLayoutEntries[0].texture.viewDimension = WGPUTextureViewDimension_2D;
    bindingLayoutEntries[0].texture.sampleType = WGPUTextureSampleType_UnfilterableFloat;

    bindingLayoutEntries[1].binding = 1u;
    bindingLayoutEntries[1].visibility = WGPUShaderStage_Fragment;
    bindingLayoutEntries[1].texture.viewDimension = WGPUTextureViewDimension_2D;
    bindingLayoutEntries[1].texture.sampleType = WGPUTextureSampleType_UnfilterableFloat;

    bindingLayoutEntries[2].binding = 2u;
    bindingLayoutEntries[2].visibility = WGPUShaderStage_Fragment;
    bindingLayoutEntries[2].texture.viewDimension = WGPUTextureViewDimension_2D;
    bindingLayoutEntries[2].texture.sampleType = WGPUTextureSampleType_Depth;

    WGPUBindGroupLayoutDescriptor bindGroupLayoutDescriptor = {};
    bindGroupLayoutDescriptor.entryCount = (uint32_t)bindingLayoutEntries.size();
    bindGroupLayoutDescriptor.entries = bindingLayoutEntries.data();

    bindingLayouts[0] = wgpuDeviceCreateBindGroupLayout(wgpContext.device, &bindGroupLayoutDescriptor);

    return bindingLayouts;
}

std::vector<WGPUBindGroup> DeferredRendering::OnBindGroupsGBuffer() {
    std::vector<WGPUBindGroup> bindGroups(1);

    std::vector<WGPUBindGroupEntry> bindGroupEntries(2);
    bindGroupEntries[0].binding = 0u;
    bindGroupEntries[0].buffer = m_uniformBuffer.getBuffer();
    bindGroupEntries[0].offset = 0u;
    bindGroupEntries[0].size = 2u * sizeof(glm::mat4);

    bindGroupEntries[1].binding = 1u;
    bindGroupEntries[1].buffer = m_cameraBuffer.getBuffer();
    bindGroupEntries[1].offset = 0u;
    bindGroupEntries[1].size = 2u * sizeof(glm::mat4);

    WGPUBindGroupDescriptor bindGroupDesc = {};
    bindGroupDesc.layout = wgpuRenderPipelineGetBindGroupLayout(wgpContext.renderPipelines.at("RP_GBUFFER"), 0u);
    bindGroupDesc.entryCount = (uint32_t)bindGroupEntries.size();
    bindGroupDesc.entries = bindGroupEntries.data();

    bindGroups[0] = wgpuDeviceCreateBindGroup(wgpContext.device, &bindGroupDesc);

    return bindGroups;
}

WGPUBindGroup DeferredRendering::createDeferredBindGroup() {
    std::vector<WGPUBindGroupEntry> bindGroupEntries(3);

    bindGroupEntries[0].binding = 0u;
    bindGroupEntries[0].textureView = m_normalTexture.getTextureView();

    bindGroupEntries[1].binding = 1u;
    bindGroupEntries[1].textureView = m_albedoTexture.getTextureView();

    bindGroupEntries[2].binding = 2u;
    bindGroupEntries[2].textureView = m_depthTexture.getTextureView();

    WGPUBindGroupDescriptor bindGroupDesc = {};
    bindGroupDesc.layout = wgpuRenderPipelineGetBindGroupLayout(wgpContext.renderPipelines.at("RP_DEFERRED"), 0u);
    bindGroupDesc.entryCount = (uint32_t)bindGroupEntries.size();
    bindGroupDesc.entries = bindGroupEntries.data();

    return wgpuDeviceCreateBindGroup(wgpContext.device, &bindGroupDesc);
}

WGPUBindGroup DeferredRendering::createLightBindGroup() {
    std::vector<WGPUBindGroupEntry> bindGroupEntries(3);

    bindGroupEntries[0].binding = 0u;
    bindGroupEntries[0].buffer = m_lightBuffer.getBuffer();
    bindGroupEntries[0].size = wgpuBufferGetSize(m_lightBuffer.getBuffer());

    bindGroupEntries[1].binding = 1u;
    bindGroupEntries[1].buffer = m_configBuffer.getBuffer();
    bindGroupEntries[1].size = wgpuBufferGetSize(m_configBuffer.getBuffer());

    bindGroupEntries[2].binding = 2u;
    bindGroupEntries[2].buffer = m_cameraBuffer.getBuffer();
    bindGroupEntries[2].size = wgpuBufferGetSize(m_cameraBuffer.getBuffer());

    WGPUBindGroupDescriptor bindGroupDesc = {};
    bindGroupDesc.layout = wgpuRenderPipelineGetBindGroupLayout(wgpContext.renderPipelines.at("RP_DEFERRED"), 1u);
    bindGroupDesc.entryCount = (uint32_t)bindGroupEntries.size();
    bindGroupDesc.entries = bindGroupEntries.data();

    return wgpuDeviceCreateBindGroup(wgpContext.device, &bindGroupDesc);
}

WGPUBindGroup DeferredRendering::createComputeBindGroup() {
    std::vector<WGPUBindGroupEntry> bindGroupEntries(3);

    bindGroupEntries[0].binding = 0u;
    bindGroupEntries[0].buffer = m_lightBuffer.getBuffer();
    bindGroupEntries[0].size = wgpuBufferGetSize(m_lightBuffer.getBuffer());

    bindGroupEntries[1].binding = 1u;
    bindGroupEntries[1].buffer = m_configBuffer.getBuffer();
    bindGroupEntries[1].size = wgpuBufferGetSize(m_configBuffer.getBuffer());

    bindGroupEntries[2].binding = 2u;
    bindGroupEntries[2].buffer = m_extentBuffer.getBuffer();
    bindGroupEntries[2].size = wgpuBufferGetSize(m_extentBuffer.getBuffer());

    WGPUBindGroupDescriptor bindGroupDesc = {};
    bindGroupDesc.layout = wgpuComputePipelineGetBindGroupLayout(wgpContext.computePipelines.at("CP_DEFFERED"), 0u);
    bindGroupDesc.entryCount = (uint32_t)bindGroupEntries.size();
    bindGroupDesc.entries = bindGroupEntries.data();

    return wgpuDeviceCreateBindGroup(wgpContext.device, &bindGroupDesc);
}

float DeferredRendering::randomFloat(float min, float max) {
    return ((max - min) * ((float)rand() / (float)RAND_MAX)) + min;
}

glm::vec3& DeferredRendering::RotateY(glm::vec3& p, float rad, const glm::vec3& centerOfRotation) {
    float x = p[0] - centerOfRotation[0];
    float z = p[2] - centerOfRotation[2];

    p[0] = z * sinf(rad) + x * cosf(rad) + centerOfRotation[0];
    p[2] = z * cosf(rad) - x * sinf(rad) + centerOfRotation[2];

    return p;
}