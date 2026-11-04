#include <WebGPU/WgpContext.h>
#include <WebGPU/WgpRenderer.h>

#include <States/ComputeParticleEasing.h>
#include <States/Collada.h>

#include "Globals.h"
#include "InputTouch.h"
#include "DeltaClock.h"
#include "PristineGrid.h"

PristineGrid::PristineGrid(StateMachine& machine) : State(machine, States::PRISTINE_GRID){
    m_camera.perspective(glm::radians(72.0f), static_cast<float>(wgpWidth) / static_cast<float>(wgpHeight), 0.1f, 1000.0f);
    m_camera.orthographic(0.0f, static_cast<float>(wgpWidth), static_cast<float>(wgpHeight), 0.0f , -1.0f, 1.0f);
    m_camera.lookAt(glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(0.0f, 0.0f, 0.0f) + glm::vec3(0.0f, 0.0f, -1.0f), glm::vec3(0.0f, 1.0f, 0.0f));
    m_camera.setMovingSpeed(100.0f);
    m_camera.setRotationSpeed(0.1f);

    m_quad.buildQuadXZ({ -20.0, -0.5f, -20.0f }, { 40.0f, 40.0f }, 1u, 1u, true, false);
    m_clearColor = { 0.0f, 0.0f, 0.2f, 1.0f };

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

    m_gridOption.base_color = { 0.0f, 0.0f, 0.0f, 1.0f };
    m_gridOption.line_color = { 1.0f, 1.0f, 1.0f, 1.0f };
    m_gridOption.line_width = { 0.05f, 0.05f };
    m_gridOptionBuffer.createBuffer(&m_gridOption, sizeof(GridOption), WGPUBufferUsage_CopyDst | WGPUBufferUsage_Uniform);

    wgpContext.addSahderModule("PRISTINE", "shader/pristine.wgsl");
    wgpContext.createRenderPipeline("PRISTINE", "RP_PRISTINE", VL_PT, std::bind(&PristineGrid::OnBindGroupLayouts, this));

    m_wgpQuad.create(m_quad);
    m_wgpQuad.setBindGroups("BG", std::bind(&PristineGrid::OnBindGroups, this));

    wgpContext.setClearColor({ 0.0f, 0.0f, 0.2f, 1.0f });
    wgpContext.OnDraw = std::bind(&PristineGrid::OnDraw, this, std::placeholders::_1, std::placeholders::_2);
}

PristineGrid::~PristineGrid() {
    m_uniformBuffer.markForDelete();
    m_gridOptionBuffer.markForDelete();
}

void PristineGrid::fixedUpdate() {

}

void PristineGrid::update() {
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

    wgpuQueueWriteBuffer(wgpContext.queue, m_gridOptionBuffer.getBuffer(), 0, &m_gridOption, sizeof(GridOption));
}

void PristineGrid::render() {
    wgpDraw();
}

void PristineGrid::OnDraw(const WGPUCommandEncoder& commandEncoder, const WGPURenderPassDescriptor& renderPassDescriptor) {
    {
        WGPURenderPassEncoder renderPassEncoder = wgpuCommandEncoderBeginRenderPass(commandEncoder, &renderPassDescriptor);
        wgpuRenderPassEncoderSetViewport(renderPassEncoder, 0.0f, 0.0f, static_cast<float>(wgpWidth), static_cast<float>(wgpHeight), 0.0f, 1.0f);
        wgpuRenderPassEncoderSetPipeline(renderPassEncoder, wgpContext.renderPipelines.at("RP_PRISTINE"));

        m_wgpQuad.draw(renderPassEncoder);

        wgpuRenderPassEncoderEnd(renderPassEncoder);
        wgpuRenderPassEncoderRelease(renderPassEncoder);
    }
}

void PristineGrid::resize(int deltaW, int deltaH) {
    m_camera.perspective(glm::radians(72.0f), static_cast<float>(wgpWidth) / static_cast<float>(wgpHeight), 0.1f, 1000.0f);
    m_camera.orthographic(0.0f, static_cast<float>(wgpWidth), static_cast<float>(wgpHeight), 0.0f , -1.0f, 1.0f);
}

void PristineGrid::OnButton(const Event::MouseButtonEvent& event) {
    wgpCleanState();
    m_isRunning = false;

    if(event.button == Event::MouseButtonEvent::BUTTON_LEFT){
        m_machine.addStateAtBottom(new ComputeParticleEasing(m_machine));
    }

    if(event.button == Event::MouseButtonEvent::BUTTON_RIGHT){
        m_machine.addStateAtBottom(new Collada(m_machine));
    }
}

std::vector<WGPUBindGroupLayout> PristineGrid::OnBindGroupLayouts() {
    std::vector<WGPUBindGroupLayout> bindingLayouts(2);

    std::vector<WGPUBindGroupLayoutEntry> bindingLayoutEntries0(1);
    bindingLayoutEntries0[0].binding = 0u;
    bindingLayoutEntries0[0].visibility = WGPUShaderStage_Vertex | WGPUShaderStage_Fragment;
    bindingLayoutEntries0[0].buffer.type = WGPUBufferBindingType::WGPUBufferBindingType_Uniform;
    bindingLayoutEntries0[0].buffer.minBindingSize = sizeof(Uniforms);

    WGPUBindGroupLayoutDescriptor bindGroupLayoutDescriptor0 = {};
    bindGroupLayoutDescriptor0.entryCount = (uint32_t)bindingLayoutEntries0.size();
    bindGroupLayoutDescriptor0.entries = bindingLayoutEntries0.data();

    bindingLayouts[0] = wgpuDeviceCreateBindGroupLayout(wgpContext.device, &bindGroupLayoutDescriptor0);

    std::vector<WGPUBindGroupLayoutEntry> bindingLayoutEntries1(1);
    bindingLayoutEntries1[0].binding = 0u;
    bindingLayoutEntries1[0].visibility = WGPUShaderStage_Vertex | WGPUShaderStage_Fragment;
    bindingLayoutEntries1[0].buffer.type = WGPUBufferBindingType::WGPUBufferBindingType_Uniform;
    bindingLayoutEntries1[0].buffer.minBindingSize = sizeof(GridOption);

    WGPUBindGroupLayoutDescriptor bindGroupLayoutDescriptor1 = {};
    bindGroupLayoutDescriptor1.entryCount = (uint32_t)bindingLayoutEntries1.size();
    bindGroupLayoutDescriptor1.entries = bindingLayoutEntries1.data();

    bindingLayouts[1] = wgpuDeviceCreateBindGroupLayout(wgpContext.device, &bindGroupLayoutDescriptor1);

    return bindingLayouts;
}

std::vector<WGPUBindGroup> PristineGrid::OnBindGroups() {
    std::vector<WGPUBindGroup> bindGroups(2);

    std::vector<WGPUBindGroupEntry> bindings0(1);

    bindings0[0].binding = 0u;
    bindings0[0].buffer = m_uniformBuffer.getBuffer();
    bindings0[0].offset = 0u;
    bindings0[0].size = wgpuBufferGetSize(m_uniformBuffer.getBuffer());

    WGPUBindGroupDescriptor bindGroupDesc0 = {};
    bindGroupDesc0.layout = wgpuRenderPipelineGetBindGroupLayout(wgpContext.renderPipelines.at("RP_PRISTINE"), 0u);
    bindGroupDesc0.entryCount = (uint32_t)bindings0.size();
    bindGroupDesc0.entries = bindings0.data();

    bindGroups[0] = wgpuDeviceCreateBindGroup(wgpContext.device, &bindGroupDesc0);

    std::vector<WGPUBindGroupEntry> bindings1(1);

    bindings1[0].binding = 0u;
    bindings1[0].buffer = m_gridOptionBuffer.getBuffer();
    bindings1[0].offset = 0u;
    bindings1[0].size = wgpuBufferGetSize(m_gridOptionBuffer.getBuffer());

    WGPUBindGroupDescriptor bindGroupDesc1 = {};
    bindGroupDesc1.layout = wgpuRenderPipelineGetBindGroupLayout(wgpContext.renderPipelines.at("RP_PRISTINE"), 1u);
    bindGroupDesc1.entryCount = (uint32_t)bindings1.size();
    bindGroupDesc1.entries = bindings1.data();

    bindGroups[1] = wgpuDeviceCreateBindGroup(wgpContext.device, &bindGroupDesc1);

    return bindGroups;
}