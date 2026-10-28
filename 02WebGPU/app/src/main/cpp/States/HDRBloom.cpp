#include <WebGPU/WgpContext.h>
#include <WebGPU/WgpRenderer.h>

#include <States/Controller.h>
#include <States/Collada.h>

#include "Globals.h"
#include "InputTouch.h"
#include "GamePad.h"
#include "HDRBloom.h"

HDRBloom::HDRBloom(StateMachine& machine) : State(machine, States::HDR_BLOOM){
    m_camera.perspective(45.0f, static_cast<float>(wgpWidth) / static_cast<float>(wgpHeight), 0.1f, 1000.0f);
    m_camera.orthographic(0.0f, static_cast<float>(wgpWidth), static_cast<float>(wgpHeight), 0.0f , -1.0f, 1.0f);
    m_camera.lookAt(glm::vec3(0.0f, 0.0f, 5.0f), glm::vec3(0.0f, 0.0f, -5.0f), glm::vec3(0.0f, 1.0f, 0.0f));
    m_camera.setMovingSpeed(10.0f);
    m_camera.setRotationSpeed(0.1f);

    m_compositeBuffer.createBuffer(sizeof(CompositeUniforms), WGPUBufferUsage_CopyDst | WGPUBufferUsage_Uniform);
    m_compositeUniforms.bloomStrength = 0.2f;
    m_compositeUniforms.exposure = 1.0f;
    wgpuQueueWriteBuffer(wgpContext.queue, m_compositeBuffer.getBuffer(), 0, &m_compositeUniforms, sizeof(CompositeUniforms));

    m_storageBuffer.createBuffer(sizeof(Instance) * 20u, WGPUBufferUsage_CopyDst | WGPUBufferUsage_Storage);
    m_lightBuffer.createBuffer(sizeof(Light) * 4u, WGPUBufferUsage_CopyDst | WGPUBufferUsage_Storage);

    m_uniformBuffer.createBuffer(sizeof(Uniforms), WGPUBufferUsage_CopyDst | WGPUBufferUsage_Uniform);
    m_uniforms.projection = m_camera.getOrthographicMatrix();
    m_uniforms.view = m_camera.getViewMatrix();
    m_uniforms.env = m_camera.getRotationMatrix();
    m_uniforms.model = glm::mat4(1.0f);
    m_uniforms.normal = Camera::GetNormalMatrix(m_camera.getViewMatrix() * m_uniforms.model);
    m_uniforms.color = { 1.0f, 0.0f, 0.0f, 1.0f };
    m_uniforms.camPosition = m_camera.getPosition();
    m_uniforms.lightVP = glm::mat4(1.0f);
    m_uniforms.shadow = Camera::BIAS *  m_uniforms.lightVP;
    m_uniforms.lightPosition = glm::vec3(50.0f, 100.0f, -100.0f);
    wgpuQueueWriteBuffer(wgpContext.queue, m_uniformBuffer.getBuffer(), 0, &m_uniforms, sizeof(Uniforms));

    m_wgpBrightnessTarget.createEmpty(wgpWidth, wgpHeight, 1u, WGPUTextureUsage_TextureBinding | WGPUTextureUsage_RenderAttachment, WGPUTextureFormat_RGBA8Unorm, 1u, 1u);
    m_wgpSceneTarget.createEmpty(wgpWidth, wgpHeight, 1u, WGPUTextureUsage_TextureBinding | WGPUTextureUsage_RenderAttachment, WGPUTextureFormat_RGBA8Unorm, 1u, 1u);
    m_wgpSceneDepth.createEmpty(wgpWidth, wgpHeight, 1u, WGPUTextureUsage_RenderAttachment, WGPUTextureFormat_Depth24PlusStencil8, 1u, 1u);

    wgpContext.addSahderModule("BLOOM", "shader/bloom.wgsl");
    wgpContext.createRenderPipeline("BLOOM", "RP_BLOOM", VL_PTN, std::bind(&HDRBloom::OnBindGroupLayouts, this), 1u,
                                    WGPUPrimitiveTopology_TriangleList,
                                    WGPUTextureFormat_Undefined,
                                    WGPUTextureFormat_Undefined,
                                    WGPUCompareFunction_LessEqual,
                                    { DEPTH_STENCIL_STATE | FRAGMENT_STATE, ColorMode::WRITE_RGBA, DepthMode::WRITE, StencilMode::DEFAULT, BlendMode::ALPHA_BLENDING, WGPUTextureFormat_RGBA8Unorm });

    wgpContext.addSahderModule("BLOOM_DOWN", "shader/bloom_down.wgsl");
    wgpContext.createRenderPipeline("BLOOM_DOWN", "RP_BLOOM_DOWN", VL_NONE, std::bind(&HDRBloom::OnBindGroupLayoutsSampler, this), 1u,
                                    WGPUPrimitiveTopology_TriangleList,
                                    WGPUTextureFormat_RGBA16Float,
                                    WGPUTextureFormat_Undefined,
                                    WGPUCompareFunction_LessEqual,
                                    {  FRAGMENT_STATE, ColorMode::WRITE_RGBA, DepthMode::PASS, StencilMode::DEFAULT, BlendMode::ADDITIVE_BLENDING_ONE });

    wgpContext.addSahderModule("BLOOM_UP", "shader/bloom_up.wgsl");
    wgpContext.createRenderPipeline("BLOOM_UP", "RP_BLOOM_UP", VL_NONE, std::bind(&HDRBloom::OnBindGroupLayoutsSampler, this), 1u,
                                    WGPUPrimitiveTopology_TriangleList,
                                    WGPUTextureFormat_RGBA16Float,
                                    WGPUTextureFormat_Undefined,
                                    WGPUCompareFunction_LessEqual,
                                    {  BLEND_STATE | FRAGMENT_STATE, ColorMode::WRITE_RGBA, DepthMode::PASS, StencilMode::DEFAULT, BlendMode::ADDITIVE_BLENDING_ONE });

    wgpContext.addSahderModule("BLOOM_COMPOSITE", "shader/bloom_composite.wgsl");
    wgpContext.createRenderPipeline("BLOOM_COMPOSITE", "RP_BLOOM_COMPOSITE", VL_NONE, std::bind(&HDRBloom::OnBindGroupLayoutsComposite, this), 1u);

    m_cube.buildCube();
    m_cubeEmessive.buildCube();
    m_cubeEmessive.flipNormal();

    m_wgpTextureEmpty.createEmpty(2u, 2u, 1u, WGPUTextureUsage_TextureBinding | WGPUTextureUsage_CopyDst, WGPUTextureFormat_BGRA8Unorm);
    m_wgpTextureEmpty.fillLayer();

    m_wgpTextureWood.setTextureFormat(WGPUTextureFormat_RGBA8UnormSrgb);
    m_wgpTextureWood.loadFromFile("textures/wood.png");
    m_wgpTextureContainer.setTextureFormat(WGPUTextureFormat_RGBA8UnormSrgb);
    m_wgpTextureContainer.loadFromFile("textures/container.png");

    m_wgpCube.create(m_cube);
    m_wgpCubeEmessive.create(m_cubeEmessive);

    m_wgpCube.addBindGroup("BG_WOOD", createBindGroup(m_wgpTextureWood));
    m_wgpCube.addBindGroup("BG_CONTAINER", createBindGroup(m_wgpTextureContainer));
    m_wgpCube.setBindGroupsSlot("BG_WOOD");

    m_wgpCubeEmessive.addBindGroup("BG_EMPTY", createBindGroup(m_wgpTextureEmpty));
    m_wgpCubeEmessive.setBindGroupsSlot("BG_EMPTY");

    wgpContext.setClearColor({ 0.0f, 0.0f, 0.0f, 1.0f });
    wgpContext.OnDraw = std::bind(&HDRBloom::OnDraw, this, std::placeholders::_1, std::placeholders::_2);
    wgpContext.OnPostDraw = std::bind(&HDRBloom::OnPostDraw, this);

    m_lights.push_back({ glm::vec3(0.0f, 0.5f, 1.5f),   0.0f, glm::vec4(5.0f, 5.0f, 5.0f, 1.0f) });
    m_lights.push_back({ glm::vec3(-4.0f, 0.5f, -3.0f), 0.0f, glm::vec4(10.0f, 0.0f, 0.0f, 1.0f) });
    m_lights.push_back({ glm::vec3(3.0f, 0.5f, 1.0f),   0.0f, glm::vec4(0.0f, 0.0f, 15.0f, 1.0f) });
    m_lights.push_back({ glm::vec3(-.8f, 2.4f, -1.0f),  0.0f, glm::vec4(0.0f, 5.0f, 0.0f, 1.0f) });

    wgpuQueueWriteBuffer(wgpContext.queue, m_lightBuffer.getBuffer(), 0, m_lights.data(), sizeof(Light) * m_lights.size());

    m_bloomFBO.init(wgpContext.device, wgpWidth / 2, wgpHeight / 2, 6u);
    m_bloomFBO.createBindGroups(wgpContext.device,
                                wgpuRenderPipelineGetBindGroupLayout(wgpContext.renderPipelines.at("RP_BLOOM_DOWN"), 0u),
                                wgpContext.getSampler(SS_LINEAR_CLAMP),
                                m_wgpBrightnessTarget.getTextureView());

    m_bindGroupComposite = createBindGroupComposite();

    WGPURenderPassColorAttachment renderPassColorAttachment = {};
    renderPassColorAttachment.view = m_wgpBrightnessTarget.getTextureView();
    renderPassColorAttachment.resolveTarget = NULL;
    renderPassColorAttachment.loadOp = WGPULoadOp::WGPULoadOp_Clear;
    renderPassColorAttachment.storeOp = WGPUStoreOp::WGPUStoreOp_Store;
    renderPassColorAttachment.clearValue = { 0.0f, 0.0f, 0.0f, 0.0f };
    renderPassColorAttachment.depthSlice = WGPU_DEPTH_SLICE_UNDEFINED;
    m_renderPassColorAttachments.push_back(renderPassColorAttachment);

    m_instances.push_back({ glm::translate(glm::vec3(0.0f, -1.0f, 0.0f)) * glm::scale(glm::vec3(12.5f, 0.5f, 12.5f)),
                            glm::vec4(1.0f, 1.0f, 1.0f, 1.0f) });

    m_instances.push_back({ glm::translate(glm::vec3(0.0f, 1.5f, 0.0f)) * glm::scale(glm::vec3(0.5f, 0.5f, 0.5f)),
                            glm::vec4(1.0f, 1.0f, 1.0f, 1.0f) });

    m_instances.push_back({ glm::translate(glm::vec3(2.0f, 0.0f, 1.0f)) * glm::scale(glm::vec3(0.5f, 0.5f, 0.5f)),
                            glm::vec4(1.0f, 1.0f, 1.0f, 1.0f) });

    m_instances.push_back({ glm::translate(glm::vec3(-1.0f, -1.0f, 2.0f)) * glm::rotate(glm::radians(60.0f), glm::normalize(glm::vec3(1.0, 0.0, 1.0))),
                            glm::vec4(1.0f, 1.0f, 1.0f, 1.0f) });

    m_instances.push_back({ glm::translate(glm::vec3(0.0f, 2.7f, 4.0f)) * glm::scale(glm::vec3(0.5f, 0.5f, 0.5f)),
                            glm::vec4(1.0f, 1.0f, 1.0f, 1.0f) });

    m_instances.push_back({ glm::translate(glm::vec3(-2.0f, 1.0f, -3.0f)) * glm::scale(glm::vec3(0.5f, 0.5f, 0.5f)),
                            glm::vec4(1.0f, 1.0f, 1.0f, 1.0f) });

    m_instances.push_back({ glm::translate(glm::vec3(-3.0f, 0.0f, 0.0f)) * glm::rotate(glm::radians(124.0f), glm::normalize(glm::vec3(1.0, 0.0, 1.0))),
                            glm::vec4(1.0f, 1.0f, 1.0f, 1.0f) });

    for (unsigned int i = 0; i < m_lights.size(); i++) {
        m_instances.push_back({ glm::translate(glm::vec3(m_lights[i].position)) * glm::scale(glm::vec3(0.25f, 0.25f, 0.25f)),
                                m_lights[i].color });
    }

    wgpuQueueWriteBuffer(wgpContext.queue, m_storageBuffer.getBuffer(), 0u, m_instances.data(), m_instances.size() * sizeof(Instance));
}

HDRBloom::~HDRBloom() {
    m_uniformBuffer.markForDelete();
    m_compositeBuffer.markForDelete();

    m_storageBuffer.markForDelete();
    m_lightBuffer.markForDelete();

    m_wgpTextureEmpty.markForDelete();
    m_wgpTextureWood.markForDelete();
    m_wgpTextureContainer.markForDelete();
    m_wgpSceneTarget.markForDelete();
    m_wgpBrightnessTarget.markForDelete();
    m_wgpSceneDepth.markForDelete();

    m_bloomFBO.cleanup();
}

void HDRBloom::fixedUpdate() {

}

void HDRBloom::update() {
    IGamePad* gamePad = AndroidGamepad::GetPlayer();
    if(gamePad) {
        glm::vec3 direction = glm::vec3(0.0f);
        float dx = 0.0f;
        float dy = 0.0f;
        bool move = false;

        float leftX = gamePad->getLeftStickX();
        float leftY = gamePad->getLeftStickY();
        float rightX = gamePad->getRightStickX();
        float rightY = gamePad->getRightStickY();
        float leftTrigger = gamePad->getLeftTrigger();
        float rightTrigger = gamePad->getRightTrigger();
        float verticalMove = rightTrigger - leftTrigger;

        direction += glm::vec3(rightX, verticalMove, rightY);
        move = leftX != 0.0f || leftY != 0.0f || rightX != 0.0f || rightY != 0.0f || verticalMove != 0.0f;

        const float ROTATION_SPEED = 5.0f;
        dx = -leftX * ROTATION_SPEED;
        dy = leftY * ROTATION_SPEED;

        if (move || dx != 0.0f || dy != 0.0f) {
            if (dx != 0.0f || dy != 0.0f) {
                m_camera.rotate(dx, dy);
            }
            if (move) {
                if (glm::length(direction) > 1.0f) {
                    direction = glm::normalize(direction);
                }
                m_camera.move(direction * m_dt);
            }
        }
    }
    m_uniforms.projection = m_camera.getPerspectiveMatrix();
    m_uniforms.view = m_camera.getViewMatrix();
    m_uniforms.env = m_camera.getRotationMatrix();
    m_uniforms.model = glm::mat4(1.0f);
    m_uniforms.normal = Camera::GetNormalMatrix(m_camera.getViewMatrix() * m_uniforms.model);
    m_uniforms.color = { 1.0f, 0.0f, 0.0f, 1.0f };
    m_uniforms.camPosition = m_camera.getPosition();
    m_uniforms.lightVP = glm::mat4(1.0f);
    m_uniforms.shadow = Camera::BIAS *  m_uniforms.lightVP;
    m_uniforms.lightPosition = glm::vec3(50.0f, 100.0f, -100.0f);
    wgpuQueueWriteBuffer(wgpContext.queue, m_uniformBuffer.getBuffer(), 0u, &m_uniforms, sizeof(Uniforms));
}

void HDRBloom::render() {
    wgpDraw();
}

void HDRBloom::OnDraw(const WGPUCommandEncoder& commandEncoder, const WGPURenderPassDescriptor& renderPassDescriptor) {
    WgpRenderer::Draw(m_wgpSceneTarget, m_wgpSceneDepth, [this, &commandEncoder](const WGPURenderPassDescriptor& renderPassDescriptor) {
        m_renderPassColorAttachments.push_back(renderPassDescriptor.colorAttachments[0]);

        WGPURenderPassDescriptor rndrPssDscrptor = renderPassDescriptor;
        rndrPssDscrptor.colorAttachments = m_renderPassColorAttachments.data();
        rndrPssDscrptor.colorAttachmentCount = m_renderPassColorAttachments.size();

        WGPURenderPassEncoder renderPassEncoder = wgpuCommandEncoderBeginRenderPass(commandEncoder, &rndrPssDscrptor);

        wgpuRenderPassEncoderSetPipeline(renderPassEncoder, wgpContext.renderPipelines.at("RP_BLOOM"));
        m_wgpCube.setBindGroupsSlot("BG_WOOD");
        m_wgpCube.draw(renderPassEncoder, 1u, 0u);

        m_wgpCube.setBindGroupsSlot("BG_CONTAINER");
        m_wgpCube.draw(renderPassEncoder, 6u, 1u);

        m_wgpCubeEmessive.setBindGroupsSlot("BG_EMPTY");
        m_wgpCubeEmessive.draw(renderPassEncoder, 4u, 7u);

        wgpuRenderPassEncoderEnd(renderPassEncoder);
        wgpuRenderPassEncoderRelease(renderPassEncoder);

        m_renderPassColorAttachments.pop_back();
    });

    WgpRenderer::Draw(m_bloomFBO.m_texture, [this, &commandEncoder](const WGPURenderPassDescriptor& renderPassDescriptor, uint32_t mip) {
        WGPURenderPassEncoder renderPassEncoder = wgpuCommandEncoderBeginRenderPass(commandEncoder, &renderPassDescriptor);
        wgpuRenderPassEncoderSetPipeline(renderPassEncoder, wgpContext.renderPipelines.at("RP_BLOOM_DOWN"));

        wgpuRenderPassEncoderSetBindGroup(renderPassEncoder, 0u, m_bloomFBO.mMipChain[mip].downsampleBindGroup, 0u, nullptr);

        wgpuRenderPassEncoderDraw(renderPassEncoder, 3u, 1u, 0u, 0u);

        wgpuRenderPassEncoderEnd(renderPassEncoder);
        wgpuRenderPassEncoderRelease(renderPassEncoder);

    });

    size_t mipCount = m_bloomFBO.mMipChain.size();
    for (size_t mip = mipCount - 1; mip > 0; mip--) {
        const auto& nextMip = m_bloomFBO.mMipChain[mip - 1];
        WgpRenderer::Draw(nextMip.renderTargetView, WGPULoadOp::WGPULoadOp_Load, [this, &commandEncoder, &mip](const WGPURenderPassDescriptor& renderPassDescriptor) {
            WGPURenderPassEncoder renderPassEncoder = wgpuCommandEncoderBeginRenderPass(commandEncoder, &renderPassDescriptor);

            wgpuRenderPassEncoderSetPipeline(renderPassEncoder, wgpContext.renderPipelines.at("RP_BLOOM_UP"));
            wgpuRenderPassEncoderSetBindGroup(renderPassEncoder, 0u, m_bloomFBO.mMipChain[mip].upsampleBindGroup, 0u, nullptr);

            wgpuRenderPassEncoderDraw(renderPassEncoder, 3u, 1u, 0u, 0u);

            wgpuRenderPassEncoderEnd(renderPassEncoder);
            wgpuRenderPassEncoderRelease(renderPassEncoder);
        });
    }

    {
        WGPURenderPassEncoder renderPassEncoder = wgpuCommandEncoderBeginRenderPass(commandEncoder, &renderPassDescriptor);
        wgpuRenderPassEncoderSetViewport(renderPassEncoder, 0.0f, 0.0f, static_cast<float>(wgpWidth), static_cast<float>(wgpHeight), 0.0f, 1.0f);

        wgpuRenderPassEncoderSetPipeline(renderPassEncoder, wgpContext.renderPipelines.at("RP_BLOOM_COMPOSITE"));
        wgpuRenderPassEncoderSetBindGroup(renderPassEncoder, 0u, m_bindGroupComposite, 0u, nullptr);
        wgpuRenderPassEncoderDraw(renderPassEncoder, 6u, 1u, 0u, 0u);

        wgpuRenderPassEncoderEnd(renderPassEncoder);
        wgpuRenderPassEncoderRelease(renderPassEncoder);
    }
}

void HDRBloom::OnPostDraw() {
    if (m_wantResize) {
        m_wantResize = false;

        m_wgpSceneTarget.resize(wgpWidth, wgpHeight);
        m_wgpSceneDepth.resize(wgpWidth, wgpHeight);
        m_wgpBrightnessTarget.resize(wgpWidth, wgpHeight);
        m_renderPassColorAttachments[0].view = m_wgpBrightnessTarget.getTextureView();


        m_bloomFBO.cleanup();
        m_bloomFBO.init(wgpContext.device, wgpWidth / 2, wgpHeight / 2, 6u);
        m_bloomFBO.createBindGroups(wgpContext.device,
                                    wgpuRenderPipelineGetBindGroupLayout(wgpContext.renderPipelines.at("RP_BLOOM_DOWN"), 0u),
                                    wgpContext.getSampler(SS_LINEAR_CLAMP),
                                    m_wgpBrightnessTarget.getTextureView());
        wgpuBindGroupRelease(m_bindGroupComposite);
        m_bindGroupComposite = createBindGroupComposite();
    }
}

void HDRBloom::resize(int deltaW, int deltaH) {
    m_camera.perspective(glm::radians(45.0f), static_cast<float>(wgpWidth) / static_cast<float>(wgpHeight), 0.1f, 1000.0f);
    m_camera.orthographic(0.0f, static_cast<float>(wgpWidth), static_cast<float>(wgpHeight), 0.0f , -1.0f, 1.0f);
    m_wantResize = true;
}

void HDRBloom::OnButton(const Event::MouseButtonEvent& event) {
    wgpCleanState();
    m_isRunning = false;

    if(event.button == Event::MouseButtonEvent::BUTTON_LEFT){
        m_machine.addStateAtBottom(new Controller(m_machine));
    }

    if(event.button == Event::MouseButtonEvent::BUTTON_RIGHT){
        m_machine.addStateAtBottom(new Collada(m_machine));
    }
}

std::vector<WGPUBindGroupLayout> HDRBloom::OnBindGroupLayouts() {
    std::vector<WGPUBindGroupLayout> bindingLayouts(1);

    std::vector<WGPUBindGroupLayoutEntry> bindingLayoutEntries(5);
    bindingLayoutEntries[0].binding = 0u;
    bindingLayoutEntries[0].visibility = WGPUShaderStage_Vertex | WGPUShaderStage_Fragment;
    bindingLayoutEntries[0].buffer.type = WGPUBufferBindingType::WGPUBufferBindingType_Uniform;
    bindingLayoutEntries[0].buffer.minBindingSize = sizeof(Uniforms);

    bindingLayoutEntries[1].binding = 1u;
    bindingLayoutEntries[1].visibility = WGPUShaderStage_Vertex | WGPUShaderStage_Fragment;
    bindingLayoutEntries[1].buffer.type = WGPUBufferBindingType::WGPUBufferBindingType_ReadOnlyStorage;
    bindingLayoutEntries[1].buffer.minBindingSize = sizeof(Instance);

    bindingLayoutEntries[2].binding = 2u;
    bindingLayoutEntries[2].visibility = WGPUShaderStage_Fragment;
    bindingLayoutEntries[2].sampler.type = WGPUSamplerBindingType::WGPUSamplerBindingType_Filtering;

    bindingLayoutEntries[3].binding = 3u;
    bindingLayoutEntries[3].visibility = WGPUShaderStage_Fragment;
    bindingLayoutEntries[3].texture.sampleType = WGPUTextureSampleType::WGPUTextureSampleType_Float;
    bindingLayoutEntries[3].texture.viewDimension = WGPUTextureViewDimension::WGPUTextureViewDimension_2D;

    bindingLayoutEntries[4].binding = 4u;
    bindingLayoutEntries[4].visibility = WGPUShaderStage_Vertex | WGPUShaderStage_Fragment;
    bindingLayoutEntries[4].buffer.type = WGPUBufferBindingType::WGPUBufferBindingType_ReadOnlyStorage;
    bindingLayoutEntries[4].buffer.minBindingSize = sizeof(Light);

    WGPUBindGroupLayoutDescriptor bindGroupLayoutDescriptor = {};
    bindGroupLayoutDescriptor.entryCount = (uint32_t)bindingLayoutEntries.size();
    bindGroupLayoutDescriptor.entries = bindingLayoutEntries.data();

    bindingLayouts[0] = wgpuDeviceCreateBindGroupLayout(wgpContext.device, &bindGroupLayoutDescriptor);

    return bindingLayouts;
}

std::vector<WGPUBindGroupLayout> HDRBloom::OnBindGroupLayoutsSampler() {
    std::vector<WGPUBindGroupLayout> bindingLayouts(1);

    std::vector<WGPUBindGroupLayoutEntry> bindingLayoutEntries(3);
    bindingLayoutEntries[0].binding = 0u;
    bindingLayoutEntries[0].visibility = WGPUShaderStage_Fragment;
    bindingLayoutEntries[0].sampler.type = WGPUSamplerBindingType::WGPUSamplerBindingType_Filtering;

    bindingLayoutEntries[1].binding = 1u;
    bindingLayoutEntries[1].visibility = WGPUShaderStage_Fragment;
    bindingLayoutEntries[1].texture.sampleType = WGPUTextureSampleType::WGPUTextureSampleType_Float;
    bindingLayoutEntries[1].texture.viewDimension = WGPUTextureViewDimension::WGPUTextureViewDimension_2D;

    bindingLayoutEntries[2].binding = 2u;
    bindingLayoutEntries[2].visibility = WGPUShaderStage_Vertex | WGPUShaderStage_Fragment;
    bindingLayoutEntries[2].buffer.type = WGPUBufferBindingType::WGPUBufferBindingType_Uniform;

    WGPUBindGroupLayoutDescriptor bindGroupLayoutDescriptor = {};
    bindGroupLayoutDescriptor.entryCount = (uint32_t)bindingLayoutEntries.size();
    bindGroupLayoutDescriptor.entries = bindingLayoutEntries.data();

    bindingLayouts[0] = wgpuDeviceCreateBindGroupLayout(wgpContext.device, &bindGroupLayoutDescriptor);

    return bindingLayouts;
}

WGPUBindGroup HDRBloom::createBindGroup(const WgpTexture& texture) {
    std::vector<WGPUBindGroupEntry> bindGroupEntries(5);
    bindGroupEntries[0].binding = 0u;
    bindGroupEntries[0].buffer = m_uniformBuffer.getBuffer();
    bindGroupEntries[0].offset = 0u;
    bindGroupEntries[0].size = sizeof(Uniforms);

    bindGroupEntries[1].binding = 1u;
    bindGroupEntries[1].buffer = m_storageBuffer.getBuffer();
    bindGroupEntries[1].offset = 0u;
    bindGroupEntries[1].size = wgpuBufferGetSize(m_storageBuffer.getBuffer());

    bindGroupEntries[2].binding = 2u;
    bindGroupEntries[2].sampler = wgpContext.getSampler(SS_LINEAR_CLAMP);

    bindGroupEntries[3].binding = 3u;
    bindGroupEntries[3].textureView = texture.getTextureView();

    bindGroupEntries[4].binding = 4u;
    bindGroupEntries[4].buffer = m_lightBuffer.getBuffer();
    bindGroupEntries[4].offset = 0u;
    bindGroupEntries[4].size = wgpuBufferGetSize(m_lightBuffer.getBuffer());

    WGPUBindGroupDescriptor bindGroupDesc = {};
    bindGroupDesc.layout = wgpuRenderPipelineGetBindGroupLayout(wgpContext.renderPipelines.at("RP_BLOOM"), 0u);
    bindGroupDesc.entryCount = (uint32_t)bindGroupEntries.size();
    bindGroupDesc.entries = bindGroupEntries.data();
    return wgpuDeviceCreateBindGroup(wgpContext.device, &bindGroupDesc);
}

std::vector<WGPUBindGroupLayout> HDRBloom::OnBindGroupLayoutsComposite() {
    std::vector<WGPUBindGroupLayout> bindingLayouts(1);

    std::vector<WGPUBindGroupLayoutEntry> bindingLayoutEntries(4);
    bindingLayoutEntries[0].binding = 0u;
    bindingLayoutEntries[0].visibility = WGPUShaderStage_Fragment;
    bindingLayoutEntries[0].sampler.type = WGPUSamplerBindingType::WGPUSamplerBindingType_Filtering;

    bindingLayoutEntries[1].binding = 1u;
    bindingLayoutEntries[1].visibility = WGPUShaderStage_Fragment;
    bindingLayoutEntries[1].texture.sampleType = WGPUTextureSampleType::WGPUTextureSampleType_Float;
    bindingLayoutEntries[1].texture.viewDimension = WGPUTextureViewDimension::WGPUTextureViewDimension_2D;
    bindingLayoutEntries[1].texture.multisampled = false;

    bindingLayoutEntries[2].binding = 2u;
    bindingLayoutEntries[2].visibility = WGPUShaderStage_Fragment;
    bindingLayoutEntries[2].texture.sampleType = WGPUTextureSampleType::WGPUTextureSampleType_Float;
    bindingLayoutEntries[2].texture.viewDimension = WGPUTextureViewDimension::WGPUTextureViewDimension_2D;
    bindingLayoutEntries[2].texture.multisampled = false;

    bindingLayoutEntries[3].binding = 3u;
    bindingLayoutEntries[3].visibility = WGPUShaderStage_Vertex | WGPUShaderStage_Fragment;
    bindingLayoutEntries[3].buffer.type = WGPUBufferBindingType::WGPUBufferBindingType_Uniform;
    bindingLayoutEntries[3].buffer.minBindingSize = sizeof(CompositeUniforms);

    WGPUBindGroupLayoutDescriptor bindGroupLayoutDescriptor = {};
    bindGroupLayoutDescriptor.entryCount = (uint32_t)bindingLayoutEntries.size();
    bindGroupLayoutDescriptor.entries = bindingLayoutEntries.data();

    bindingLayouts[0] = wgpuDeviceCreateBindGroupLayout(wgpContext.device, &bindGroupLayoutDescriptor);

    return bindingLayouts;
}

WGPUBindGroup HDRBloom::createBindGroupComposite() {
    std::vector<WGPUBindGroupEntry> bindGroupEntries(4);

    bindGroupEntries[0].binding = 0u;
    bindGroupEntries[0].sampler = wgpContext.getSampler(SS_LINEAR_CLAMP);

    bindGroupEntries[1].binding = 1u;
    bindGroupEntries[1].textureView = m_wgpSceneTarget.getTextureView();

    bindGroupEntries[2].binding = 2u;
    bindGroupEntries[2].textureView = m_bloomFBO.mMipChain[0].renderTargetView;

    bindGroupEntries[3].binding = 3u;
    bindGroupEntries[3].buffer = m_compositeBuffer.getBuffer();
    bindGroupEntries[3].offset = 0u;
    bindGroupEntries[3].size = wgpuBufferGetSize(m_compositeBuffer.getBuffer());

    WGPUBindGroupDescriptor bindGroupDesc = {};
    bindGroupDesc.layout = wgpuRenderPipelineGetBindGroupLayout(wgpContext.renderPipelines.at("RP_BLOOM_COMPOSITE"), 0u);
    bindGroupDesc.entryCount = (uint32_t)bindGroupEntries.size();
    bindGroupDesc.entries = bindGroupEntries.data();

    return wgpuDeviceCreateBindGroup(wgpContext.device, &bindGroupDesc);
}

void BloomFBO::createBindGroups(WGPUDevice device, WGPUBindGroupLayout layout, WGPUSampler sampler, WGPUTextureView sceneColorView) {
    size_t mipCount = mMipChain.size();

    for (size_t i = 0; i < mipCount; i++) {
        // --- DOWNSAMPLE ---
        WGPUTextureView currentSrcView = (i == 0) ? sceneColorView : mMipChain[i - 1].renderTargetView;
        std::array<WGPUBindGroupEntry, 3> downsampleEntries{};
        downsampleEntries[0].binding = 0u;
        downsampleEntries[0].sampler = sampler;
        downsampleEntries[1].binding = 1u;
        downsampleEntries[1].textureView = currentSrcView;
        downsampleEntries[2].binding = 2u;
        downsampleEntries[2].buffer = mMipChain[i].downbuffer;
        downsampleEntries[2].offset = 0u;
        downsampleEntries[2].size = wgpuBufferGetSize(mMipChain[i].downbuffer);

        WGPUBindGroupDescriptor downsampleDesc{};
        downsampleDesc.layout = layout;
        downsampleDesc.entryCount = downsampleEntries.size();
        downsampleDesc.entries = downsampleEntries.data();
        mMipChain[i].downsampleBindGroup = wgpuDeviceCreateBindGroup(device, &downsampleDesc);

        // --- UPSAMPLE ---
        std::array<WGPUBindGroupEntry, 3> upsampleEntries{};
        upsampleEntries[0].binding = 0u;
        upsampleEntries[0].sampler = sampler;
        upsampleEntries[1].binding = 1u;
        upsampleEntries[1].textureView = mMipChain[i].renderTargetView;
        upsampleEntries[2].binding = 2u;
        upsampleEntries[2].buffer = upbuffer;
        upsampleEntries[2].offset = 0u;
        upsampleEntries[2].size = wgpuBufferGetSize(upbuffer);

        WGPUBindGroupDescriptor upsampleDesc{};
        upsampleDesc.layout = layout;
        upsampleDesc.entryCount = upsampleEntries.size();
        upsampleDesc.entries = upsampleEntries.data();
        mMipChain[i].upsampleBindGroup = wgpuDeviceCreateBindGroup(device, &upsampleDesc);
    }
}

void BloomFBO::init(WGPUDevice device, uint32_t width, uint32_t height, uint32_t mipChainLength, float filterRadius) {
    uint32_t mipWidth = width;
    uint32_t mipHeight = height;
    m_texture.createEmpty(mipWidth, mipHeight, 1u, WGPUTextureUsage_TextureBinding | WGPUTextureUsage_RenderAttachment, WGPUTextureFormat_RGBA16Float, mipChainLength);

    for (uint32_t i = 0; i < mipChainLength; i++) {
        BloomMip mip;

        WGPUTextureViewDescriptor rtViewDesc = {};
        rtViewDesc.nextInChain = nullptr;
        rtViewDesc.format = WGPUTextureFormat_RGBA16Float;
        rtViewDesc.dimension = WGPUTextureViewDimension_2D;
        rtViewDesc.baseMipLevel = i;
        rtViewDesc.mipLevelCount = 1;
        rtViewDesc.baseArrayLayer = 0;
        rtViewDesc.arrayLayerCount = 1;
        rtViewDesc.aspect = WGPUTextureAspect_All;

        mip.renderTargetView = wgpuTextureCreateView(m_texture.getTexture(), &rtViewDesc);

        WGPUBufferDescriptor bufferDesc = {};
        bufferDesc.size = sizeof(DownsampleUniforms);
        bufferDesc.usage = WGPUBufferUsage_CopyDst | WGPUBufferUsage_Uniform;
        mip.downbuffer = wgpuDeviceCreateBuffer(device, &bufferDesc);

        DownsampleUniforms uniforms;
        uniforms.srcResolution = { static_cast<float>(mipWidth), static_cast<float>(mipHeight) };
        uniforms.mipLevel = m_karisAverageOnDownsample ? 0u : 1u;

        wgpuQueueWriteBuffer(wgpContext.queue, mip.downbuffer, 0u, &uniforms, sizeof(BloomFBO::DownsampleUniforms));

        mMipChain.emplace_back(mip);

        mipWidth = std::max(1u, mipWidth / 2);
        mipHeight = std::max(1u, mipHeight / 2);
    }

    WGPUBufferDescriptor bufferDesc = {};
    bufferDesc.size = sizeof(UpsampleUniforms);
    bufferDesc.usage = WGPUBufferUsage_CopyDst | WGPUBufferUsage_Uniform;
    upbuffer = wgpuDeviceCreateBuffer(device, &bufferDesc);

    UpsampleUniforms uniforms;
    uniforms.filterRadius = filterRadius;
    wgpuQueueWriteBuffer(wgpContext.queue, upbuffer, 0u, &uniforms, sizeof(BloomFBO::UpsampleUniforms));
}

void BloomFBO::resize(uint32_t width, uint32_t height) {
    m_texture.resize(width, height);
}

void BloomFBO::cleanup() {
    for (auto& mip : mMipChain) {
        if (mip.renderTargetView)
            wgpuTextureViewRelease(mip.renderTargetView);

        if (mip.downbuffer){
            wgpuBufferDestroy(mip.downbuffer);
            wgpuBufferRelease(mip.downbuffer);
        }

        if (mip.downsampleBindGroup) {
            wgpuBindGroupRelease(mip.downsampleBindGroup);
        }

        if (mip.upsampleBindGroup) {
            wgpuBindGroupRelease(mip.upsampleBindGroup);
        }
    }


    if (upbuffer) {
        wgpuBufferDestroy(upbuffer);
        wgpuBufferRelease(upbuffer);

    }

    mMipChain.clear();
    m_texture.markForDelete();
}