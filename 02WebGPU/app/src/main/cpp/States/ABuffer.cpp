#include <WebGPU/WgpContext.h>
#include <WebGPU/WgpRenderer.h>

#include <States/HDRBloom.h>
#include <States/ComputeParticleEasing.h>

#include "Globals.h"
#include "InputTouch.h"
#include "DeltaClock.h"
#include "ABuffer.h"

ABuffer::ABuffer(StateMachine& machine) : State(machine, States::ABUFFER){
    wgpSetSurfaceDepthFormat(WGPUTextureFormat::WGPUTextureFormat_Depth24Plus);
    wgpTextureAddUsage(wgpContext.depthTexture, wgpContext.depthTextureView, WGPUTextureUsage_TextureBinding);

    m_camera.perspective(glm::radians(72.0f), static_cast<float>(wgpWidth) / static_cast<float>(wgpHeight), 0.1f, 1000.0f);
    m_camera.orthographic(0.0f, static_cast<float>(wgpWidth), static_cast<float>(wgpHeight), 0.0f , -1.0f, 1.0f);
    m_camera.lookAt(glm::vec3(0.0f, 5.0f, 200.0f), glm::vec3(0.0f, 0.0f, 0.0f) + glm::vec3(0.0f, 0.0f, -1.0f), glm::vec3(0.0f, 1.0f, 0.0f));
    m_camera.setMovingSpeed(100.0f);
    m_camera.setRotationSpeed(0.1f);

    createBuffer();

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

    wgpContext.addSahderModule("ABUFFER_OPAQUE", "shader/abuffer_opaque.wgsl");
    wgpContext.createRenderPipeline("ABUFFER_OPAQUE", "RP_ABUFFER_OPAQUE", VL_PN, std::bind(&ABuffer::OnBindGroupLayouts, this));

    wgpContext.addSahderModule("ABUFFER_TRANSLUCENT", "shader/abuffer_translucent.wgsl");
    wgpContext.createRenderPipeline("ABUFFER_TRANSLUCENT", "RP_ABUFFER_TRANSLUCENT", VL_PN, std::bind(&ABuffer::OnBindGroupLayoutsTranslucent, this),
                                    1u,
                                    WGPUPrimitiveTopology_Undefined,
                                    WGPUTextureFormat_Undefined,
                                    WGPUTextureFormat_Undefined,
                                    WGPUCompareFunction_LessEqual,
                                    { FRAGMENT_STATE | BLEND_STATE, ColorMode::WRITE_NONE, DepthMode::PASS, StencilMode::DEFAULT, BlendMode::ALPHA_BLENDING });

    wgpContext.addSahderModule("ABUFFER_COMPOSITE", "shader/abuffer_composite.wgsl");
    wgpContext.createRenderPipeline("ABUFFER_COMPOSITE", "RP_ABUFFER_COMPOSITE", VL_NONE, std::bind(&ABuffer::OnBindGroupLayoutsComposite, this),
                                    1u,
                                    WGPUPrimitiveTopology_Undefined,
                                    WGPUTextureFormat_Undefined,
                                    WGPUTextureFormat_Undefined,
                                    WGPUCompareFunction_LessEqual,
                                    { FRAGMENT_STATE | BLEND_STATE, ColorMode::WRITE_RGBA, DepthMode::PASS, StencilMode::DEFAULT, BlendMode::ALPHA_BLENDING_ONE });

    m_trackball.reshape(wgpWidth, wgpHeight);
    m_teapot.loadModel("models/teapot.obj", false, false, true);
    m_wgpTeapot.create(m_teapot);
    m_wgpTeapot.setBindGroups("BG_OPAQUE", std::bind(&ABuffer::OnBindGroups, this));

    wgpContext.setClearColor({ 0.0f, 0.0f, 0.0f, 1.0f });
    wgpContext.OnDraw = std::bind(&ABuffer::OnDraw, this, std::placeholders::_1, std::placeholders::_2);

    m_bindGroupTranslucent = createBindGroupTranslucent();
    m_bindGroupComposite = createBindGroupComposite();
}

ABuffer::~ABuffer() {
    m_uniformBuffer.markForDelete();
    m_sliceInfoBuffer.markForDelete();
    m_linkedListBuffer.markForDelete();
    m_headsInitBuffer.markForDelete();
    m_headsBuffer.markForDelete();

    wgpuBindGroupRelease(m_bindGroupTranslucent);
    wgpuBindGroupRelease(m_bindGroupComposite);
}

void ABuffer::fixedUpdate() {

}

void ABuffer::update() {
    glm::vec3 position = m_camera.getPosition();
    RotateY(position, m_dt * 0.5f);
    m_camera.setPosition(position, true);

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

void ABuffer::render() {
    wgpDraw();
}

void ABuffer::OnDraw(const WGPUCommandEncoder& commandEncoder, const WGPURenderPassDescriptor& renderPassDescriptor) {
    {
        WGPURenderPassEncoder renderPassEncoder = wgpuCommandEncoderBeginRenderPass(commandEncoder, &renderPassDescriptor);
        wgpuRenderPassEncoderSetViewport(renderPassEncoder, 0.0f, 0.0f, static_cast<float>(wgpWidth), static_cast<float>(wgpHeight), 0.0f, 1.0f);
        wgpuRenderPassEncoderSetPipeline(renderPassEncoder, wgpContext.renderPipelines.at("RP_ABUFFER_OPAQUE"));

        m_wgpTeapot.setBindGroupsSlot("BG_OPAQUE");
        m_wgpTeapot.draw(renderPassEncoder, 8u);

        wgpuRenderPassEncoderEnd(renderPassEncoder);
        wgpuRenderPassEncoderRelease(renderPassEncoder);
    }

    for (uint32_t slice = 0; slice < m_numSlices; ++slice) {
        wgpuCommandEncoderCopyBufferToBuffer(commandEncoder, m_headsInitBuffer.getBuffer(), 0u, m_headsBuffer.getBuffer(), 0u, wgpuBufferGetSize(m_headsInitBuffer.getBuffer()));

        const uint32_t scissor_x = 0;
        const uint32_t scissor_y = slice * m_sliceHeight;
        const uint32_t scissor_width = wgpWidth;
        const uint32_t scissor_height = std::min((slice + 1) * m_sliceHeight, static_cast<uint32_t>(wgpHeight)) - slice * m_sliceHeight;

        {
            WGPURenderPassColorAttachment renderPassColorAttachment = renderPassDescriptor.colorAttachments[0];
            renderPassColorAttachment.loadOp = WGPULoadOp::WGPULoadOp_Load;

            WGPURenderPassDescriptor rndrPssDscrptor = renderPassDescriptor;
            rndrPssDscrptor.colorAttachments = &renderPassColorAttachment;
            rndrPssDscrptor.depthStencilAttachment = nullptr;

            WGPURenderPassEncoder renderPassEncoder = wgpuCommandEncoderBeginRenderPass(commandEncoder, &rndrPssDscrptor);
            wgpuRenderPassEncoderSetScissorRect(renderPassEncoder, scissor_x, scissor_y, scissor_width, scissor_height);

            wgpuRenderPassEncoderSetPipeline(renderPassEncoder, wgpContext.renderPipelines.at("RP_ABUFFER_TRANSLUCENT"));
            const uint32_t dynamic_offsets[1] = { slice * wgpContext.limits.minUniformBufferOffsetAlignment };
            wgpuRenderPassEncoderSetBindGroup(renderPassEncoder, 0u, m_bindGroupTranslucent, 1u, dynamic_offsets);

            m_wgpTeapot.drawRaw(renderPassEncoder, 8u);

            wgpuRenderPassEncoderEnd(renderPassEncoder);
            wgpuRenderPassEncoderRelease(renderPassEncoder);
        }

        {
            WGPURenderPassColorAttachment renderPassColorAttachment = renderPassDescriptor.colorAttachments[0];
            renderPassColorAttachment.loadOp = WGPULoadOp::WGPULoadOp_Load;

            WGPURenderPassDescriptor rndrPssDscrptor = renderPassDescriptor;
            rndrPssDscrptor.colorAttachments = &renderPassColorAttachment;
            rndrPssDscrptor.depthStencilAttachment = nullptr;

            WGPURenderPassEncoder renderPassEncoder = wgpuCommandEncoderBeginRenderPass(commandEncoder, &rndrPssDscrptor);
            wgpuRenderPassEncoderSetScissorRect(renderPassEncoder, scissor_x, scissor_y, scissor_width, scissor_height);

            wgpuRenderPassEncoderSetPipeline(renderPassEncoder, wgpContext.renderPipelines.at("RP_ABUFFER_COMPOSITE"));
            const uint32_t dynamic_offsets[1] = { slice * wgpContext.limits.minUniformBufferOffsetAlignment };
            wgpuRenderPassEncoderSetBindGroup(renderPassEncoder, 0u, m_bindGroupComposite, 1u, dynamic_offsets);

            wgpuRenderPassEncoderDraw(renderPassEncoder, 6u, 1u, 0u, 0u);

            wgpuRenderPassEncoderEnd(renderPassEncoder);
            wgpuRenderPassEncoderRelease(renderPassEncoder);

        }
    }
}

void ABuffer::resize(int deltaW, int deltaH) {
    m_camera.perspective(glm::radians(72.0f), static_cast<float>(wgpWidth) / static_cast<float>(wgpHeight), 0.1f, 1000.0f);
    m_camera.orthographic(0.0f, static_cast<float>(wgpWidth), static_cast<float>(wgpHeight), 0.0f , -1.0f, 1.0f);

    wgpTextureAddUsage(wgpContext.depthTexture, wgpContext.depthTextureView, WGPUTextureUsage_TextureBinding);
    wgpuBindGroupRelease(m_bindGroupTranslucent);
    wgpuBindGroupRelease(m_bindGroupComposite);

    createBuffer();
    m_bindGroupTranslucent = createBindGroupTranslucent();
    m_bindGroupComposite = createBindGroupComposite();
}

void ABuffer::OnButton(const Event::MouseButtonEvent& event) {
    wgpCleanState();
    m_isRunning = false;

    if(event.button == Event::MouseButtonEvent::BUTTON_LEFT){
        m_machine.addStateAtBottom(new HDRBloom(m_machine));
    }

    if(event.button == Event::MouseButtonEvent::BUTTON_RIGHT){
        m_machine.addStateAtBottom(new ComputeParticleEasing(m_machine));
    }
}

std::vector<WGPUBindGroupLayout> ABuffer::OnBindGroupLayouts() {
    std::vector<WGPUBindGroupLayout> bindingLayouts(1);

    std::vector<WGPUBindGroupLayoutEntry> bindingLayoutEntries(1);
    bindingLayoutEntries[0].binding = 0u;
    bindingLayoutEntries[0].visibility = WGPUShaderStage_Vertex | WGPUShaderStage_Fragment;
    bindingLayoutEntries[0].buffer.type = WGPUBufferBindingType::WGPUBufferBindingType_Uniform;
    bindingLayoutEntries[0].buffer.minBindingSize = sizeof(Uniforms);

    WGPUBindGroupLayoutDescriptor bindGroupLayoutDescriptor = {};
    bindGroupLayoutDescriptor.entryCount = (uint32_t)bindingLayoutEntries.size();
    bindGroupLayoutDescriptor.entries = bindingLayoutEntries.data();

    bindingLayouts[0] = wgpuDeviceCreateBindGroupLayout(wgpContext.device, &bindGroupLayoutDescriptor);

    return bindingLayouts;
}

std::vector<WGPUBindGroupLayout> ABuffer::OnBindGroupLayoutsTranslucent() {
    std::vector<WGPUBindGroupLayout> bindingLayouts(1);

    std::vector<WGPUBindGroupLayoutEntry> bindingLayoutEntries(5);
    bindingLayoutEntries[0].binding = 0u;
    bindingLayoutEntries[0].visibility = WGPUShaderStage_Vertex | WGPUShaderStage_Fragment;
    bindingLayoutEntries[0].buffer.type = WGPUBufferBindingType::WGPUBufferBindingType_Uniform;
    bindingLayoutEntries[0].buffer.minBindingSize = sizeof(Uniforms);

    bindingLayoutEntries[1].binding = 1u;
    bindingLayoutEntries[1].visibility = WGPUShaderStage_Fragment;
    bindingLayoutEntries[1].buffer.type = WGPUBufferBindingType::WGPUBufferBindingType_Storage;

    bindingLayoutEntries[2].binding = 2u;
    bindingLayoutEntries[2].visibility = WGPUShaderStage_Fragment;
    bindingLayoutEntries[2].buffer.type = WGPUBufferBindingType::WGPUBufferBindingType_Storage;

    bindingLayoutEntries[3].binding = 3u;
    bindingLayoutEntries[3].visibility = WGPUShaderStage_Fragment;
    bindingLayoutEntries[3].texture.viewDimension = WGPUTextureViewDimension_2D;
    bindingLayoutEntries[3].texture.sampleType = WGPUTextureSampleType_Depth;

    bindingLayoutEntries[4].binding = 4u;
    bindingLayoutEntries[4].visibility = WGPUShaderStage_Fragment;
    bindingLayoutEntries[4].buffer.type = WGPUBufferBindingType::WGPUBufferBindingType_Uniform;
    bindingLayoutEntries[4].buffer.minBindingSize = sizeof(SliceInfo);
    bindingLayoutEntries[4].buffer.hasDynamicOffset = true;

    WGPUBindGroupLayoutDescriptor bindGroupLayoutDescriptor = {};
    bindGroupLayoutDescriptor.entryCount = (uint32_t)bindingLayoutEntries.size();
    bindGroupLayoutDescriptor.entries = bindingLayoutEntries.data();

    bindingLayouts[0] = wgpuDeviceCreateBindGroupLayout(wgpContext.device, &bindGroupLayoutDescriptor);

    return bindingLayouts;
}

std::vector<WGPUBindGroupLayout> ABuffer::OnBindGroupLayoutsComposite() {
    std::vector<WGPUBindGroupLayout> bindingLayouts(1);

    std::vector<WGPUBindGroupLayoutEntry> bindingLayoutEntries(3);

    bindingLayoutEntries[0].binding = 0u;
    bindingLayoutEntries[0].visibility = WGPUShaderStage_Fragment;
    bindingLayoutEntries[0].buffer.type = WGPUBufferBindingType::WGPUBufferBindingType_Storage;

    bindingLayoutEntries[1].binding = 1u;
    bindingLayoutEntries[1].visibility = WGPUShaderStage_Fragment;
    bindingLayoutEntries[1].buffer.type = WGPUBufferBindingType::WGPUBufferBindingType_Storage;

    bindingLayoutEntries[2].binding = 2u;
    bindingLayoutEntries[2].visibility = WGPUShaderStage_Fragment;
    bindingLayoutEntries[2].buffer.type = WGPUBufferBindingType::WGPUBufferBindingType_Uniform;
    bindingLayoutEntries[2].buffer.minBindingSize = sizeof(SliceInfo);
    bindingLayoutEntries[2].buffer.hasDynamicOffset = true;

    WGPUBindGroupLayoutDescriptor bindGroupLayoutDescriptor = {};
    bindGroupLayoutDescriptor.entryCount = (uint32_t)bindingLayoutEntries.size();
    bindGroupLayoutDescriptor.entries = bindingLayoutEntries.data();

    bindingLayouts[0] = wgpuDeviceCreateBindGroupLayout(wgpContext.device, &bindGroupLayoutDescriptor);

    return bindingLayouts;
}

std::vector<WGPUBindGroup> ABuffer::OnBindGroups() {
    std::vector<WGPUBindGroup> bindGroups(1);

    std::vector<WGPUBindGroupEntry> bindings(1);

    bindings[0].binding = 0u;
    bindings[0].buffer = m_uniformBuffer.getBuffer();
    bindings[0].offset = 0u;
    bindings[0].size = wgpuBufferGetSize(m_uniformBuffer.getBuffer());

    WGPUBindGroupDescriptor bindGroupDesc = {};
    bindGroupDesc.layout = wgpuRenderPipelineGetBindGroupLayout(wgpContext.renderPipelines.at("RP_ABUFFER_OPAQUE"), 0u);
    bindGroupDesc.entryCount = (uint32_t)bindings.size();
    bindGroupDesc.entries = bindings.data();

    bindGroups[0] = wgpuDeviceCreateBindGroup(wgpContext.device, &bindGroupDesc);
    return bindGroups;
}

WGPUBindGroup ABuffer::createBindGroupTranslucent() {
    std::vector<WGPUBindGroupEntry> bindings(5);

    bindings[0].binding = 0u;
    bindings[0].buffer = m_uniformBuffer.getBuffer();
    bindings[0].offset = 0u;
    bindings[0].size = wgpuBufferGetSize(m_uniformBuffer.getBuffer());

    bindings[1].binding = 1u;
    bindings[1].buffer = m_headsBuffer.getBuffer();
    bindings[1].offset = 0u;
    bindings[1].size = wgpuBufferGetSize(m_headsBuffer.getBuffer());

    bindings[2].binding = 2u;
    bindings[2].buffer = m_linkedListBuffer.getBuffer();
    bindings[2].offset = 0u;
    bindings[2].size = wgpuBufferGetSize(m_linkedListBuffer.getBuffer());

    bindings[3].binding = 3u;
    bindings[3].textureView = wgpContext.depthTextureView;

    bindings[4].binding = 4u;
    bindings[4].buffer = m_sliceInfoBuffer.getBuffer();
    bindings[4].offset = 0u;
    bindings[4].size = wgpContext.limits.minUniformBufferOffsetAlignment;

    WGPUBindGroupDescriptor bindGroupDesc = {};
    bindGroupDesc.layout = wgpuRenderPipelineGetBindGroupLayout(wgpContext.renderPipelines.at("RP_ABUFFER_TRANSLUCENT"), 0u);
    bindGroupDesc.entryCount = (uint32_t)bindings.size();
    bindGroupDesc.entries = bindings.data();

    return wgpuDeviceCreateBindGroup(wgpContext.device, &bindGroupDesc);
}

WGPUBindGroup ABuffer::createBindGroupComposite() {
    std::vector<WGPUBindGroupEntry> bindings(3);

    bindings[0].binding = 0u;
    bindings[0].buffer = m_headsBuffer.getBuffer();
    bindings[0].offset = 0u;
    bindings[0].size = wgpuBufferGetSize(m_headsBuffer.getBuffer());

    bindings[1].binding = 1u;
    bindings[1].buffer = m_linkedListBuffer.getBuffer();
    bindings[1].offset = 0u;
    bindings[1].size = wgpuBufferGetSize(m_linkedListBuffer.getBuffer());

    bindings[2].binding = 2u;
    bindings[2].buffer = m_sliceInfoBuffer.getBuffer();
    bindings[2].offset = 0u;
    bindings[2].size = wgpContext.limits.minUniformBufferOffsetAlignment;

    WGPUBindGroupDescriptor bindGroupDesc = {};
    bindGroupDesc.layout = wgpuRenderPipelineGetBindGroupLayout(wgpContext.renderPipelines.at("RP_ABUFFER_COMPOSITE"), 0u);
    bindGroupDesc.entryCount = (uint32_t)bindings.size();
    bindGroupDesc.entries = bindings.data();

    return wgpuDeviceCreateBindGroup(wgpContext.device, &bindGroupDesc);
}

void ABuffer::createBuffer() {
    m_sliceInfoBuffer.markForDelete();
    m_sliceInfoBuffer.cleanup();

    m_linkedListBuffer.markForDelete();
    m_linkedListBuffer.cleanup();

    m_headsInitBuffer.markForDelete();
    m_headsInitBuffer.cleanup();

    m_headsBuffer.markForDelete();
    m_headsBuffer.cleanup();

    const uint32_t bytesPerLine = wgpWidth * AverageLayers * (5u * sizeof(float) + 1u * sizeof(uint32_t));
    const uint32_t maxLines = (uint32_t)floorf(wgpContext.limits.maxStorageBufferBindingSize / (float)bytesPerLine);
    m_numSlices = (uint32_t)ceilf(wgpHeight / (float)maxLines);
    m_sliceHeight = (uint32_t)ceilf(wgpHeight / (float)m_numSlices);
    uint64_t totalBufferSize = m_numSlices * wgpContext.limits.minUniformBufferOffsetAlignment;

    m_sliceInfoBuffer.createBuffer(m_numSlices * wgpContext.limits.minUniformBufferOffsetAlignment, WGPUBufferUsage_CopyDst | WGPUBufferUsage_Uniform, true);

    uint8_t* byteData = (uint8_t*)wgpuBufferGetMappedRange(m_sliceInfoBuffer.getBuffer(), 0, m_numSlices * wgpContext.limits.minUniformBufferOffsetAlignment);
    const int32_t byteStride = wgpContext.limits.minUniformBufferOffsetAlignment;
    for (int32_t i = 0u; i < (int32_t)m_numSlices; ++i) {
        SliceInfo* currentSlice = (SliceInfo*)(byteData + (i * byteStride));
        currentSlice->targetWidth = wgpWidth;
        currentSlice->maxStorableFragments = AverageLayers * wgpWidth * m_sliceHeight;
        currentSlice->sliceStartY = i * m_sliceHeight;
        currentSlice->padding = 0u;
    }
    wgpuBufferUnmap(m_sliceInfoBuffer.getBuffer());

    m_linkedListBuffer.createBuffer(m_sliceHeight * bytesPerLine, WGPUBufferUsage_CopyDst | WGPUBufferUsage_Storage);

    const uint32_t bufferCount = 1u + wgpWidth * m_sliceHeight;
    m_headsInitBuffer.createBuffer(bufferCount * sizeof(uint32_t), WGPUBufferUsage_CopySrc, true);
    m_headsBuffer.createBuffer(bufferCount * sizeof(uint32_t), WGPUBufferUsage_CopyDst | WGPUBufferUsage_Storage);

    uint32_t* buffer = (uint32_t*)wgpuBufferGetMappedRange(m_headsInitBuffer.getBuffer(), 0u, bufferCount * sizeof(uint32_t));
    for (uint32_t i = 0; i < bufferCount; ++i) {
        buffer[i] = 0xffffffff;
    }
    wgpuBufferUnmap(m_headsInitBuffer.getBuffer());
}

glm::vec3& ABuffer::RotateY(glm::vec3& p, float rad, const glm::vec3& centerOfRotation) {
    float x = p[0] - centerOfRotation[0];
    float z = p[2] - centerOfRotation[2];

    p[0] = z * sinf(rad) + x * cosf(rad) + centerOfRotation[0];
    p[2] = z * cosf(rad) - x * sinf(rad) + centerOfRotation[2];

    return p;
}