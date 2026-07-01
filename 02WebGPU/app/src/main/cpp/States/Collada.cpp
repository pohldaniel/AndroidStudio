#include <States/Wireframe.h>
#include "WgpContext.h"
#include "AssetIO.h"
#include "Collada.h"

Collada::Collada(StateMachine& machine) : State(machine, States::COLLADA)  {
    StateMachine::DisableWireframe();

    uint8_t* data; uint32_t size;
    AssetIO::LoadAsset("models/cowboy/cowboy.dae", data, size);

    MemoryIOSystem* memoryFS = new MemoryIOSystem();
    memoryFS->AddFile("models/cowboy/cowboy.dae", std::vector<char>{data, data + size});
    m_run.loadAnimationAssimp(memoryFS, "models/cowboy/cowboy.dae", "Armature_Armature", "run");

    memoryFS = new MemoryIOSystem();
    memoryFS->AddFile("models/cowboy/cowboy.dae", std::vector<char>{data, data + size});
    m_cowboy.loadModelAssimp(memoryFS,"models/cowboy/cowboy.dae", 0u);
    m_cowboy.applyBindpose(true);
    m_cowboy.addAnimationState(m_run);
    m_cowboy.getAnimationState(0)->setLooped(true);

    AssetIO::Free(data);

    m_camera.perspective(glm::radians(25.0f), static_cast<float>(wgpWidth) / static_cast<float>(wgpHeight), 0.1f, 1000.0f);
    m_camera.orthographic(0.0f, static_cast<float>(wgpWidth), 0.0f, static_cast<float>(wgpHeight), -1.0f, 1.0f);
    m_camera.lookAt(glm::vec3(0.0f, 5.0f, 25.0f), glm::vec3(0.0f, 5.0f, 0.0f), glm::vec3(0.0f, 1.0f, 0.0f));
    m_camera.setRotationSpeed(0.125f);
    m_camera.setMovingSpeed(10.0f);

    wgpContext.setClearColor({1.0f, 1.0f, 1.0f, 1.0f});
    wgpContext.addSahderModule("ANIMATION", "shader/animation.wgsl");
    wgpContext.createRenderPipeline("ANIMATION", "RP_ANIMATION", VL_PTNWJ, std::bind(&Collada::OnBindGroupLayouts, this));

    m_wgpBuffer.createBuffer(sizeof(Uniforms), WGPUBufferUsage_CopyDst | WGPUBufferUsage_Uniform);
    m_wgpSkinBuffer.createBuffer(sizeof(glm::mat4) * MAX_JOIN, WGPUBufferUsage_CopyDst | WGPUBufferUsage_Storage);
    m_wgpTexture.loadFromFile("models/cowboy/cowboy.png", true);

    m_wgpCowboy.create(m_cowboy);
    m_wgpCowboy.setBindGroups("BG", std::bind(&Collada::OnBindGroups, this));

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
    wgpContext.OnDraw = std::bind(&Collada::OnDraw, this, std::placeholders::_1, std::placeholders::_2);
}

Collada::~Collada() {
    m_wgpBuffer.markForDelete();
    m_wgpSkinBuffer.markForDelete();
}

void Collada::fixedUpdate() {

}

void Collada::update() {
    m_cowboy.update(m_dt);
    m_cowboy.updateSkinning();
    const AnimatedMesh* mesh = static_cast<const AnimatedMesh*>(m_cowboy.getMesh());
    wgpuQueueWriteBuffer(wgpContext.queue, m_wgpSkinBuffer.getBuffer(), 0u, mesh->getSkinMatrices(), mesh->getNumBones() * sizeof(glm::mat4));
}

void Collada::render() {
    wgpDraw();
}

void Collada::OnDraw(const WGPUCommandEncoder& commandEncoder, const WGPURenderPassDescriptor& renderPassDescriptor) {
    m_uniforms.projection = m_camera.getPerspectiveMatrix();
    m_uniforms.view = m_camera.getViewMatrix();
    wgpuQueueWriteBuffer(wgpContext.queue, m_wgpBuffer.getBuffer(), 0, &m_uniforms, sizeof(Uniforms));

    WGPURenderPassEncoder renderPassEncoder = wgpuCommandEncoderBeginRenderPass(wgpContext.commandEncoder, &renderPassDescriptor);
    wgpuRenderPassEncoderSetPipeline(renderPassEncoder, wgpContext.renderPipelines.at("RP_ANIMATION"));
    m_wgpCowboy.draw(renderPassEncoder);

    wgpuRenderPassEncoderEnd(renderPassEncoder);
    wgpuRenderPassEncoderRelease(renderPassEncoder);
}

void Collada::resize(int deltaW, int deltaH) {
    m_camera.perspective(glm::radians(25.0f), static_cast<float>(wgpWidth) / static_cast<float>(wgpHeight), 0.1f, 1000.0f);
    m_camera.orthographic(0.0f, static_cast<float>(wgpWidth), 0.0f, static_cast<float>(wgpHeight), -1.0f, 1.0f);
}

void Collada::OnButton() {
    wgpPipelineLayoutsRelease();
    wgpPipelinesRelease();
    wgpShaderModulesRelease();

    m_isRunning = false;
    m_machine.addStateAtBottom(new Wireframe(m_machine));
}

std::vector <WGPUBindGroupLayout> Collada::OnBindGroupLayouts() {
    std::vector<WGPUBindGroupLayout> bindingLayouts(2);

    std::vector<WGPUBindGroupLayoutEntry> bindingLayoutEntries0(2);
    bindingLayoutEntries0[0].binding = 0u;
    bindingLayoutEntries0[0].visibility = WGPUShaderStage_Vertex ;
    bindingLayoutEntries0[0].buffer.type = WGPUBufferBindingType::WGPUBufferBindingType_Uniform;
    bindingLayoutEntries0[0].buffer.minBindingSize = sizeof(Uniforms);

    bindingLayoutEntries0[1].binding = 1u;
    bindingLayoutEntries0[1].visibility = WGPUShaderStage_Vertex;
    bindingLayoutEntries0[1].buffer.type = WGPUBufferBindingType::WGPUBufferBindingType_ReadOnlyStorage;
    bindingLayoutEntries0[1].buffer.minBindingSize = 16 * sizeof(float);

    WGPUBindGroupLayoutDescriptor bindGroupLayoutDescriptor0 = {};
    bindGroupLayoutDescriptor0.entryCount = (uint32_t)bindingLayoutEntries0.size();
    bindGroupLayoutDescriptor0.entries = bindingLayoutEntries0.data();

    bindingLayouts[0] = wgpuDeviceCreateBindGroupLayout(wgpContext.device, &bindGroupLayoutDescriptor0);

    std::vector<WGPUBindGroupLayoutEntry> bindingLayoutEntries1(2);
    bindingLayoutEntries1[0].binding = 0u;
    bindingLayoutEntries1[0].visibility = WGPUShaderStage_Fragment;
    bindingLayoutEntries1[0].sampler.type = WGPUSamplerBindingType_Filtering;

    bindingLayoutEntries1[1].binding = 1u;
    bindingLayoutEntries1[1].visibility = WGPUShaderStage_Fragment;
    bindingLayoutEntries1[1].texture.sampleType = WGPUTextureSampleType_Float;
    bindingLayoutEntries1[1].texture.viewDimension = WGPUTextureViewDimension_2D;

    WGPUBindGroupLayoutDescriptor bindGroupLayoutDescriptor1 = {};
    bindGroupLayoutDescriptor1.entryCount = (uint32_t)bindingLayoutEntries1.size();
    bindGroupLayoutDescriptor1.entries = bindingLayoutEntries1.data();
    bindingLayouts[1] = wgpuDeviceCreateBindGroupLayout(wgpContext.device, &bindGroupLayoutDescriptor1);

    return bindingLayouts;
}

std::vector<WGPUBindGroup> Collada::OnBindGroups() {
    std::vector<WGPUBindGroup> bindGroups(2);

    std::vector<WGPUBindGroupEntry> bindGroupEntries0(2);
    bindGroupEntries0[0].binding = 0u;
    bindGroupEntries0[0].buffer = m_wgpBuffer.getBuffer();
    bindGroupEntries0[0].offset = 0u;
    bindGroupEntries0[0].size = wgpuBufferGetSize(m_wgpBuffer.getBuffer());

    bindGroupEntries0[1].binding = 1u;
    bindGroupEntries0[1].buffer = m_wgpSkinBuffer.getBuffer();
    bindGroupEntries0[1].offset = 0u;
    bindGroupEntries0[1].size = wgpuBufferGetSize(m_wgpSkinBuffer.getBuffer());

    WGPUBindGroupDescriptor bindGroupDescriptor0 = {};
    bindGroupDescriptor0.layout = wgpuRenderPipelineGetBindGroupLayout(wgpContext.renderPipelines.at("RP_ANIMATION"), 0u);
    bindGroupDescriptor0.entryCount = (uint32_t)bindGroupEntries0.size();
    bindGroupDescriptor0.entries = bindGroupEntries0.data();
    bindGroups[0] = wgpuDeviceCreateBindGroup(wgpContext.device, &bindGroupDescriptor0);

    std::vector<WGPUBindGroupEntry> bindGroupEntries1(2);
    bindGroupEntries1[0].binding = 0u;
    bindGroupEntries1[0].sampler = wgpContext.getSampler(SS_LINEAR_CLAMP);

    bindGroupEntries1[1].binding = 1u;
    bindGroupEntries1[1].textureView = m_wgpTexture.getTextureView();

    WGPUBindGroupDescriptor bindGroupDescriptor1 = {};
    bindGroupDescriptor1.layout = wgpuRenderPipelineGetBindGroupLayout(wgpContext.renderPipelines.at("RP_ANIMATION"), 1u);
    bindGroupDescriptor1.entryCount = (uint32_t)bindGroupEntries1.size();
    bindGroupDescriptor1.entries = bindGroupEntries1.data();
    bindGroups[1] = wgpuDeviceCreateBindGroup(wgpContext.device, &bindGroupDescriptor1);

    return bindGroups;
}