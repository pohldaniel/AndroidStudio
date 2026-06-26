#include "WgpContext.h"
#include "Wireframe.h"

Wireframe::Wireframe(StateMachine& machine) : State(machine, States::WIREFRAME) {

    wgpContext.OnDraw = std::bind(&Wireframe::OnDraw, this, std::placeholders::_1, std::placeholders::_2);

    m_quad.buildQuadXY({ -1.0f, -1.0f, 0.0f }, { 2.0f, 2.0f }, 1u, 1u, true, true);

    m_wgpTexture.loadFromFile("textures/webgpu.png");
    m_wgpQuad.create(m_quad);
    m_wgpBuffer.createBuffer(sizeof(Uniforms), WGPUBufferUsage_CopyDst | WGPUBufferUsage_Uniform);

    m_uniforms.projection = glm::mat4(1.0f);
    m_uniforms.view = glm::mat4(1.0f);
    m_uniforms.env = glm::mat4(1.0f);
    m_uniforms.model = glm::mat4(1.0f);
    m_uniforms.normal = Camera::GetNormalMatrix(m_uniforms.view * m_uniforms.model);
    m_uniforms.color = { 1.0f, 1.0f, 1.0f, 1.0f };
    m_uniforms.camPosition = { 0.0f, 0.0f, 0.0f };
    m_uniforms.lightVP = glm::mat4(1.0f);
    m_uniforms.shadow = Camera::BIAS * m_uniforms.lightVP;
    m_uniforms.lightPosition = { 0.0f, 0.0f, 0.0f };

    wgpuQueueWriteBuffer(wgpContext.queue, m_wgpBuffer.getBuffer(), 0, &m_uniforms, sizeof(Uniforms));

    wgpContext.addSahderModule("TEXTURE", "shader/texture.wgsl");
    wgpContext.createRenderPipeline("TEXTURE", "RP_TEXTURE", VL_PTN, std::bind(&Wireframe::OnBindGroupLayouts, this));

    m_wgpQuad.setBindGroups("BG", std::bind(&Wireframe::OnBindGroups, this));

}

Wireframe::~Wireframe() {
    m_wgpTexture.markForDelete();
}

void Wireframe::OnDraw(const WGPUCommandEncoder& commandEncoder, const WGPURenderPassDescriptor& renderPassDescriptor) {
    WGPURenderPassEncoder renderPassEncoder = wgpuCommandEncoderBeginRenderPass(wgpContext.commandEncoder, &renderPassDescriptor);
    WGPURenderPipeline pipeline = wgpContext.renderPipelines.at("RP_TEXTURE");

    wgpuRenderPassEncoderSetPipeline(renderPassEncoder, wgpContext.renderPipelines.at("RP_TEXTURE"));
    m_wgpQuad.draw(renderPassEncoder);

    wgpuRenderPassEncoderEnd(renderPassEncoder);
    wgpuRenderPassEncoderRelease(renderPassEncoder);
}

void Wireframe::fixedUpdate() {

}

void Wireframe::update() {

}

void Wireframe::render() {

}

std::vector<WGPUBindGroupLayout> Wireframe::OnBindGroupLayouts(){
    std::vector<WGPUBindGroupLayout> bindingLayouts(1);

    std::vector<WGPUBindGroupLayoutEntry> bindingLayoutEntries(3);
    bindingLayoutEntries[0].binding = 0u;
    bindingLayoutEntries[0].visibility = WGPUShaderStage_Vertex | WGPUShaderStage_Fragment;
    bindingLayoutEntries[0].buffer.type = WGPUBufferBindingType_Uniform;
    bindingLayoutEntries[0].buffer.minBindingSize = sizeof(Uniforms);

    bindingLayoutEntries[1].binding = 1u;
    bindingLayoutEntries[1].visibility = WGPUShaderStage_Fragment;
    bindingLayoutEntries[1].sampler.type = WGPUSamplerBindingType_Filtering;

    bindingLayoutEntries[2].binding = 2u;
    bindingLayoutEntries[2].visibility = WGPUShaderStage_Fragment;
    bindingLayoutEntries[2].texture.viewDimension = WGPUTextureViewDimension_2D;
    bindingLayoutEntries[2].texture.sampleType = WGPUTextureSampleType_Float;

    WGPUBindGroupLayoutDescriptor bindGroupLayoutDescriptor = {};
    bindGroupLayoutDescriptor.entryCount = (uint32_t)bindingLayoutEntries.size();
    bindGroupLayoutDescriptor.entries = bindingLayoutEntries.data();

    bindingLayouts[0] = wgpuDeviceCreateBindGroupLayout(wgpContext.device, &bindGroupLayoutDescriptor);

    return bindingLayouts;
}

std::vector<WGPUBindGroup> Wireframe::OnBindGroups(){
    std::vector<WGPUBindGroup> bindGroups(1);

    std::vector<WGPUBindGroupEntry> bindGroupEntries(3);

    bindGroupEntries[0].binding = 0u;
    bindGroupEntries[0].buffer = m_wgpBuffer.getBuffer();
    bindGroupEntries[0].offset = 0u;
    bindGroupEntries[0].size = sizeof(Uniforms);

    bindGroupEntries[1].binding = 1u;
    bindGroupEntries[1].sampler = wgpContext.getSampler(SS_LINEAR_REPEAT);

    bindGroupEntries[2].binding = 2u;
    bindGroupEntries[2].textureView = m_wgpTexture.getTextureView();

    WGPUBindGroupDescriptor bindGroupDesc = {};
    bindGroupDesc.layout = wgpuRenderPipelineGetBindGroupLayout(wgpContext.renderPipelines.at("RP_TEXTURE"), 0u);
    bindGroupDesc.entryCount = (uint32_t)bindGroupEntries.size();
    bindGroupDesc.entries = bindGroupEntries.data();
    bindGroups[0] = wgpuDeviceCreateBindGroup(wgpContext.device, &bindGroupDesc);

    return bindGroups;
}