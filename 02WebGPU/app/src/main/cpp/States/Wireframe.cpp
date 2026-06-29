#include "WgpContext.h"
#include "Wireframe.h"

Wireframe::Wireframe(StateMachine& machine) : State(machine, States::WIREFRAME) {

    m_camera.perspective(glm::radians(45.0f), static_cast<float>(wgpWidth) / static_cast<float>(wgpHeight), 0.1f, 1000.0f);
    m_camera.orthographic(0.0f, static_cast<float>(wgpWidth), 0.0f, static_cast<float>(wgpHeight), -1.0f, 1.0f);
    m_camera.lookAt(glm::vec3(1.0f, 2.0f, 4.0f), glm::vec3(0.2f, 0.2f, 1.5f) + glm::vec3(0.0f, 0.0f, -1.0f), glm::vec3(0.0f, 1.0f, 0.0f));
    m_camera.setRotationSpeed(0.125f);
    m_camera.setMovingSpeed(10.0f);

    m_dragon.loadModel("models/dragon/dragon.obj", glm::vec3(0.0f, 1.0f, 0.0f), -90.0f, glm::vec3(0.0f, -1.0f, 0.0f), 0.1f, false, false, false, false, false, true);
    m_dragon.rewind();
    m_dragon.generateColors(ModelColor::MC_POSITION);

    m_wgpBuffer.createBuffer(sizeof(Uniforms), WGPUBufferUsage_CopyDst | WGPUBufferUsage_Uniform);

    wgpContext.addSahderModule("PTN", "shader/shader.wgsl");
    wgpContext.createRenderPipeline("PTN", "RP_PTNC", VL_PTNC, std::bind(&Wireframe::OnBindGroupLayouts, this));

    wgpContext.addSahderModule("WF", "shader/wireframe.wgsl");
    wgpContext.createRenderPipeline("WF", "RP_WF", VL_NONE, std::bind(&Wireframe::OnBindGroupLayoutsWF, this), 1u, WGPUPrimitiveTopology::WGPUPrimitiveTopology_LineList);

    m_wgpDragon.create(m_dragon);
    m_wgpDragon.setBindGroups("BG_WF", std::bind(&Wireframe::OnBindGroupsWF, this));
    m_wgpDragon.setBindGroups("BG", std::bind(&Wireframe::OnBindGroups, this));

    AddBindgroups(m_wgpDragon);

    m_uniforms.projection = m_camera.getPerspectiveMatrix();
    m_uniforms.view = m_camera.getViewMatrix();
    m_uniforms.env = m_camera.getRotationMatrix();
    m_uniforms.model = glm::mat4(1.0f);
    m_uniforms.normal = Camera::GetNormalMatrix(m_uniforms.view * m_uniforms.model);
    m_uniforms.color = { 1.0f, 1.0f, 1.0f, 1.0f };
    m_uniforms.camPosition = { 0.0f, 0.0f, 0.0f };
    m_uniforms.lightVP = glm::mat4(1.0f);
    m_uniforms.shadow = Camera::BIAS * m_uniforms.lightVP;
    m_uniforms.lightPosition = { 0.0f, 0.0f, 0.0f };

    wgpuQueueWriteBuffer(wgpContext.queue, m_wgpBuffer.getBuffer(), 0, &m_uniforms, sizeof(Uniforms));
    wgpContext.OnDraw = std::bind(&Wireframe::OnDraw, this, std::placeholders::_1, std::placeholders::_2);
}

Wireframe::~Wireframe() {
    m_wgpBuffer.markForDelete();
}

void Wireframe::fixedUpdate() {

}

void Wireframe::update() {

}

void Wireframe::render() {
    wgpDraw();
}

void Wireframe::OnDraw(const WGPUCommandEncoder& commandEncoder, const WGPURenderPassDescriptor& renderPassDescriptor) {
    m_uniforms.projection = m_camera.getPerspectiveMatrix();
    m_uniforms.view = m_camera.getViewMatrix();
    wgpuQueueWriteBuffer(wgpContext.queue, m_wgpBuffer.getBuffer(), 0, &m_uniforms, sizeof(Uniforms));

    WGPURenderPassEncoder renderPassEncoder = wgpuCommandEncoderBeginRenderPass(wgpContext.commandEncoder, &renderPassDescriptor);
    wgpuRenderPassEncoderSetPipeline(renderPassEncoder, wgpContext.renderPipelines.at("RP_PTNC"));
    m_wgpDragon.draw(renderPassEncoder);

    wgpuRenderPassEncoderEnd(renderPassEncoder);
    wgpuRenderPassEncoderRelease(renderPassEncoder);
}

void Wireframe::resize(int deltaW, int deltaH) {
    m_camera.perspective(glm::radians(45.0f), static_cast<float>(wgpWidth) / static_cast<float>(wgpHeight), 0.1f, 1000.0f);
    m_camera.orthographic(0.0f, static_cast<float>(wgpWidth), 0.0f, static_cast<float>(wgpHeight), -1.0f, 1.0f);
}

std::vector <WGPUBindGroupLayout> Wireframe::OnBindGroupLayouts() {
    std::vector<WGPUBindGroupLayout> bindingLayouts(2);

    std::vector<WGPUBindGroupLayoutEntry> bindingLayoutEntries0(2);

    WGPUBindGroupLayoutEntry& uniformLayout = bindingLayoutEntries0[0];
    uniformLayout.binding = 0u;
    uniformLayout.visibility = WGPUShaderStage_Vertex | WGPUShaderStage_Fragment;
    uniformLayout.buffer.type = WGPUBufferBindingType_Uniform;
    uniformLayout.buffer.minBindingSize = sizeof(Uniforms);

    WGPUBindGroupLayoutEntry& samplerBindingLayout = bindingLayoutEntries0[1];
    samplerBindingLayout.binding = 1u;
    samplerBindingLayout.visibility = WGPUShaderStage_Fragment;
    samplerBindingLayout.sampler.type = WGPUSamplerBindingType_Filtering;

    WGPUBindGroupLayoutDescriptor bindGroupLayoutDescriptor0 = {};
    bindGroupLayoutDescriptor0.entryCount = (uint32_t)bindingLayoutEntries0.size();
    bindGroupLayoutDescriptor0.entries = bindingLayoutEntries0.data();

    bindingLayouts[0] = wgpuDeviceCreateBindGroupLayout(wgpContext.device, &bindGroupLayoutDescriptor0);

    std::vector<WGPUBindGroupLayoutEntry> bindingLayoutEntries1(1);

    WGPUBindGroupLayoutEntry& textureBindingLayout = bindingLayoutEntries1[0];
    textureBindingLayout.binding = 0u;
    textureBindingLayout.visibility = WGPUShaderStage_Fragment;
    textureBindingLayout.texture.sampleType = WGPUTextureSampleType_Float;
    textureBindingLayout.texture.viewDimension = WGPUTextureViewDimension_2D;


    WGPUBindGroupLayoutDescriptor bindGroupLayoutDescriptor1 = {};
    bindGroupLayoutDescriptor1.entryCount = (uint32_t)bindingLayoutEntries1.size();
    bindGroupLayoutDescriptor1.entries = bindingLayoutEntries1.data();

    bindingLayouts[1] = wgpuDeviceCreateBindGroupLayout(wgpContext.device, &bindGroupLayoutDescriptor1);

    return bindingLayouts;
}

std::vector <WGPUBindGroupLayout> Wireframe::OnBindGroupLayoutsWF() {
    std::vector<WGPUBindGroupLayout> bindingLayouts(2);

    std::vector<WGPUBindGroupLayoutEntry> bindingLayoutEntries0(1);

    WGPUBindGroupLayoutEntry& uniformLayout = bindingLayoutEntries0[0];
    uniformLayout.binding = 0u;
    uniformLayout.visibility = WGPUShaderStage_Vertex | WGPUShaderStage_Fragment;
    uniformLayout.buffer.type = WGPUBufferBindingType_Uniform;
    uniformLayout.buffer.minBindingSize = sizeof(Uniforms);

    WGPUBindGroupLayoutDescriptor bindGroupLayoutDescriptor0 = {};
    bindGroupLayoutDescriptor0.entryCount = (uint32_t)bindingLayoutEntries0.size();
    bindGroupLayoutDescriptor0.entries = bindingLayoutEntries0.data();
    bindingLayouts[0] = wgpuDeviceCreateBindGroupLayout(wgpContext.device, &bindGroupLayoutDescriptor0);

    std::vector<WGPUBindGroupLayoutEntry> bindingLayoutEntries1(2);

    WGPUBindGroupLayoutEntry& indiceBindingLayout = bindingLayoutEntries1[0];
    indiceBindingLayout.binding = 0u;
    indiceBindingLayout.visibility = WGPUShaderStage_Vertex;
    indiceBindingLayout.buffer.type = WGPUBufferBindingType_ReadOnlyStorage;
    indiceBindingLayout.buffer.hasDynamicOffset = false;

    WGPUBindGroupLayoutEntry& vertexBindingLayout = bindingLayoutEntries1[1];
    vertexBindingLayout.binding = 1u;
    vertexBindingLayout.visibility = WGPUShaderStage_Vertex;
    vertexBindingLayout.buffer.type = WGPUBufferBindingType_ReadOnlyStorage;
    vertexBindingLayout.buffer.hasDynamicOffset = false;

    WGPUBindGroupLayoutDescriptor bindGroupLayoutDescriptor1 = {};
    bindGroupLayoutDescriptor1.entryCount = (uint32_t)bindingLayoutEntries1.size();
    bindGroupLayoutDescriptor1.entries = bindingLayoutEntries1.data();
    bindingLayouts[1] = wgpuDeviceCreateBindGroupLayout(wgpContext.device, &bindGroupLayoutDescriptor1);

    return bindingLayouts;
}

std::vector<WGPUBindGroup> Wireframe::OnBindGroups() {
    std::vector<WGPUBindGroup> bindGroups(1);

    std::vector<WGPUBindGroupEntry> bindGroupEntries(2);

    bindGroupEntries[0].binding = 0u;
    bindGroupEntries[0].buffer = m_wgpBuffer.getBuffer();
    bindGroupEntries[0].offset = 0u;
    bindGroupEntries[0].size = sizeof(Uniforms);

    bindGroupEntries[1].binding = 1u;
    bindGroupEntries[1].sampler = wgpContext.getSampler(SS_LINEAR_CLAMP);

    WGPUBindGroupDescriptor bindGroupDesc = {};
    bindGroupDesc.layout = wgpuRenderPipelineGetBindGroupLayout(wgpContext.renderPipelines.at("RP_PTNC"), 0u);
    bindGroupDesc.entryCount = (uint32_t)bindGroupEntries.size();
    bindGroupDesc.entries = bindGroupEntries.data();

    bindGroups[0] = wgpuDeviceCreateBindGroup(wgpContext.device, &bindGroupDesc);

    return bindGroups;
}

std::vector<WGPUBindGroup> Wireframe::OnBindGroupsWF() {
    std::vector<WGPUBindGroup> bindGroups(1);

    std::vector<WGPUBindGroupEntry> BindGroupEntries(1);

    BindGroupEntries[0].binding = 0;
    BindGroupEntries[0].buffer = m_wgpBuffer.getBuffer();
    BindGroupEntries[0].offset = 0;
    BindGroupEntries[0].size = sizeof(Uniforms);

    WGPUBindGroupDescriptor bindGroupDesc = {};
    bindGroupDesc.layout = wgpuRenderPipelineGetBindGroupLayout(wgpContext.renderPipelines.at("RP_WF"), 0u);
    bindGroupDesc.entryCount = (uint32_t)BindGroupEntries.size();
    bindGroupDesc.entries = BindGroupEntries.data();
    bindGroups[0] = wgpuDeviceCreateBindGroup(wgpContext.device, &bindGroupDesc);

    return bindGroups;
}

void Wireframe::AddBindgroups(const WgpModel& model) {
    for (std::list<WgpMesh>::const_iterator it = model.getMeshes().begin(); it != model.getMeshes().end(); ++it) {
        const WgpMesh& mesh = *it;

        std::vector<WGPUBindGroupEntry> bindingGroupEntries(1);
        bindingGroupEntries[0].binding = 0u;
        bindingGroupEntries[0].textureView = mesh.getTexture().getTextureView();

        WGPUBindGroupDescriptor bindGroupDesc = {};
        bindGroupDesc.layout = wgpuRenderPipelineGetBindGroupLayout(wgpContext.renderPipelines.at("RP_PTNC"), 1u);
        bindGroupDesc.entryCount = (uint32_t)bindingGroupEntries.size();
        bindGroupDesc.entries = bindingGroupEntries.data();

        mesh.addBindGroup("BG", wgpuDeviceCreateBindGroup(wgpContext.device, &bindGroupDesc));

        std::vector<WGPUBindGroupEntry> bindingGroupEntriesWF(2);
        bindingGroupEntriesWF[0].binding = 0u;
        bindingGroupEntriesWF[0].buffer = mesh.getVertexBuffer().getBuffer();
        bindingGroupEntriesWF[0].offset = 0u;
        bindingGroupEntriesWF[0].size = wgpuBufferGetSize(mesh.getVertexBuffer().getBuffer());

        bindingGroupEntriesWF[1].binding = 1u;
        bindingGroupEntriesWF[1].buffer = mesh.getIndexBuffer().getBuffer();
        bindingGroupEntriesWF[1].offset = 0u;
        bindingGroupEntriesWF[1].size = wgpuBufferGetSize(mesh.getIndexBuffer().getBuffer());

        WGPUBindGroupDescriptor bindGroupDescWF = {};
        bindGroupDescWF.layout = wgpuRenderPipelineGetBindGroupLayout(wgpContext.renderPipelines.at("RP_WF"), 1u);
        bindGroupDescWF.entryCount = (uint32_t)bindingGroupEntriesWF.size();
        bindGroupDescWF.entries = bindingGroupEntriesWF.data();

        mesh.addBindGroup("BG_WF", wgpuDeviceCreateBindGroup(wgpContext.device, &bindGroupDescWF));
    }
}