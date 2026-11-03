#include <WebGPU/WgpContext.h>
#include <WebGPU/WgpRenderer.h>

#include <States/HDRBloom.h>
#include <States/Collada.h>

#include "Globals.h"
#include "InputTouch.h"
#include "DeltaClock.h"
#include "ComputeParticleEasing.h"

ComputeParticleEasing::ComputeParticleEasing(StateMachine& machine) : State(machine, States::COMPUTE_PARTICLE_EASING){
    m_camera.perspective(72.0f, static_cast<float>(wgpWidth) / static_cast<float>(wgpHeight), 0.1f, 1000.0f);
    m_camera.orthographic(0.0f, static_cast<float>(wgpWidth), static_cast<float>(wgpHeight), 0.0f , -1.0f, 1.0f);
    m_camera.lookAt(glm::vec3(0.0f, 5.0f, 200.0f), glm::vec3(0.0f, 0.0f, 0.0f) + glm::vec3(0.0f, 0.0f, -1.0f), glm::vec3(0.0f, 1.0f, 0.0f));
    m_camera.setMovingSpeed(100.0f);
    m_camera.setRotationSpeed(0.1f);

    wgpContext.addSampler(wgpCreateSamplerExt(
            {
                    nullptr,
                    WGPU_STR("sampler"),
                    WGPUAddressMode_Repeat,
                    WGPUAddressMode_Repeat,
                    WGPUAddressMode_Repeat,
                    WGPUFilterMode_Linear,
                    WGPUFilterMode_Nearest,
                    WGPUMipmapFilterMode_Linear,
                    0.0f,
                    1.0f,
                    WGPUCompareFunction_Undefined,
                    1u
            }), SS_0);

    wgpVertexAttribute(VL_0).push_back(WGPUVertexAttribute{ NULL, WGPUVertexFormat_Float32x3, 16u,  0u });
    wgpVertexAttribute(VL_0).push_back(WGPUVertexAttribute{ NULL, WGPUVertexFormat_Float32x3, 32u,  1u });
    wgpVertexAttribute(VL_0).push_back(WGPUVertexAttribute{ NULL, WGPUVertexFormat_Float32,   156u, 2u });

    wgpVertexAttribute(VL_1).push_back(WGPUVertexAttribute{ NULL, WGPUVertexFormat_Float32x4, 0u, 3u });
    wgpVertexAttribute(VL_1).push_back(WGPUVertexAttribute{ NULL, WGPUVertexFormat_Float32x2, 16, 4u });

    wgpVertexBufferLayout(VL_0).push_back(WGPUVertexBufferLayout{ NULL, WGPUVertexStepMode_Instance, PROPERTY_NUM * 4u, wgpVertexAttribute(VL_0).size(), wgpVertexAttribute(VL_0).data() });
    wgpVertexBufferLayout(VL_0).push_back(WGPUVertexBufferLayout{ NULL, WGPUVertexStepMode_Vertex, 6u * 4u, wgpVertexAttribute(VL_1).size(), wgpVertexAttribute(VL_1).data() });

    m_wgpParticle.loadFromFile("textures/particle.png");

    initParticle();
    m_storageBuffer.createBuffer(m_initialParticle, sizeof(m_initialParticle), WGPUBufferUsage_CopyDst | WGPUBufferUsage_Vertex | WGPUBufferUsage_Storage);
    float simData[4] = { 0.0f , 2000.0f , 10000.0f, 0.0f };
    m_simBuffer.createBuffer(simData, sizeof(SimParam), WGPUBufferUsage_CopyDst | WGPUBufferUsage_Uniform);

    wgpContext.addSahderModule("EASING_COMPUTE", "shader/easing_compute.wgsl");
    wgpContext.createComputePipeline("EASING_COMPUTE", "main", "CP_EASING", std::bind(&ComputeParticleEasing::OnBindGroupLayoutsCompute, this));

    wgpContext.addSahderModule("EASING", "shader/easing.wgsl");
    wgpContext.createRenderPipeline("EASING", "RP_EASING", VL_0, std::bind(&ComputeParticleEasing::OnBindGroupLayouts, this),
                                    1u,
                                    WGPUPrimitiveTopology_Undefined,
                                    WGPUTextureFormat_Undefined,
                                    WGPUTextureFormat_Undefined,
                                    WGPUCompareFunction_LessEqual,
                                    { FRAGMENT_STATE| DEPTH_STENCIL_STATE | BLEND_STATE, ColorMode::WRITE_RGBA, DepthMode::PASS, StencilMode::DEFAULT, BlendMode::ADDITIVE_BLENDING_SRC, WGPUTextureFormat_Undefined, WGPUCullMode_None });

    wgpContext.setClearColor({ 0.0f, 0.0f, 0.0f, 1.0f });
    wgpContext.OnDraw = std::bind(&ComputeParticleEasing::OnDraw, this, std::placeholders::_1, std::placeholders::_2);

    m_bindGroup = createBindGroup();
    m_bindGroupParticle = createBindGroupParticle();


    static const float vertex_data[6 * 6] = {
            -0.005f, -0.005f, 0.0f, 1.0f, 0.0f, 0.0f,
            0.005f, -0.005f, 0.0f, 1.0f, 0.0f, 1.0f,
            -0.005f,  0.005f, 0.0f, 1.0f, 1.0f, 0.0f,

            -0.005f,  0.005f, 0.0f, 1.0f, 1.0f, 0.0f,
            0.005f, -0.005f, 0.0f, 1.0f, 0.0f, 1.0f,
            0.005f,  0.005f, 0.0f, 1.0f, 1.0f, 1.0f,
    };

    m_quadVerticesBuffer.createBuffer(vertex_data, sizeof(vertex_data), WGPUBufferUsage_CopyDst | WGPUBufferUsage_Vertex);
}

ComputeParticleEasing::~ComputeParticleEasing() {
    m_storageBuffer.markForDelete();
    m_simBuffer.markForDelete();
    wgpuBindGroupRelease(m_bindGroup);
    wgpuBindGroupRelease(m_bindGroupParticle);
}

void ComputeParticleEasing::fixedUpdate() {

}

void ComputeParticleEasing::update() {
    m_time += m_dt * m_speed;
    wgpuQueueWriteBuffer(wgpContext.queue, m_simBuffer.getBuffer(), 0u, &m_time, sizeof(float));
}

void ComputeParticleEasing::render() {
    wgpDraw();
}

void ComputeParticleEasing::OnDraw(const WGPUCommandEncoder& commandEncoder, const WGPURenderPassDescriptor& renderPassDescriptor) {
    {
        WGPUComputePassEncoder computePassEncoder = wgpuCommandEncoderBeginComputePass(commandEncoder, nullptr);
        wgpuComputePassEncoderSetPipeline(computePassEncoder, wgpContext.computePipelines.at("CP_EASING"));
        wgpuComputePassEncoderSetBindGroup(computePassEncoder, 0u, m_bindGroupParticle, 0u, nullptr);
        wgpuComputePassEncoderDispatchWorkgroups(computePassEncoder, (uint32_t)ceilf(PARTICLE_NUM_EASING / WORKGROUP_SIZE), 1u, 1u);
        wgpuComputePassEncoderEnd(computePassEncoder);
        wgpuComputePassEncoderRelease(computePassEncoder);
    }
    {
        WGPURenderPassEncoder renderPassEncoder = wgpuCommandEncoderBeginRenderPass(commandEncoder, &renderPassDescriptor);
        wgpuRenderPassEncoderSetPipeline(renderPassEncoder, wgpContext.renderPipelines.at("RP_EASING"));
        wgpuRenderPassEncoderSetBindGroup(renderPassEncoder, 0u, m_bindGroup, 0u, 0u);

        wgpuRenderPassEncoderSetVertexBuffer(renderPassEncoder, 0u, m_storageBuffer.getBuffer(), 0u, WGPU_WHOLE_SIZE);
        wgpuRenderPassEncoderSetVertexBuffer(renderPassEncoder, 1u, m_quadVerticesBuffer.getBuffer(), 0u,WGPU_WHOLE_SIZE);

        wgpuRenderPassEncoderDraw(renderPassEncoder, 6u, PARTICLE_NUM_EASING, 0u, 0u);
        wgpuRenderPassEncoderEnd(renderPassEncoder);
        wgpuRenderPassEncoderRelease(renderPassEncoder);
    }
}

void ComputeParticleEasing::resize(int deltaW, int deltaH) {
    m_camera.perspective(72.0f, static_cast<float>(wgpWidth) / static_cast<float>(wgpHeight), 0.1f, 1000.0f);
    m_camera.orthographic(0.0f, static_cast<float>(wgpWidth), static_cast<float>(wgpHeight), 0.0f , -1.0f, 1.0f);
}

void ComputeParticleEasing::OnButton(const Event::MouseButtonEvent& event) {
    wgpCleanState();
    m_isRunning = false;

    if(event.button == Event::MouseButtonEvent::BUTTON_LEFT){
        m_machine.addStateAtBottom(new HDRBloom(m_machine));
    }

    if(event.button == Event::MouseButtonEvent::BUTTON_RIGHT){
        m_machine.addStateAtBottom(new Collada(m_machine));
    }
}

std::vector<WGPUBindGroupLayout> ComputeParticleEasing::OnBindGroupLayouts() {
    std::vector<WGPUBindGroupLayout> bindingLayouts(1);

    std::vector<WGPUBindGroupLayoutEntry> bindingLayoutEntries(2);

    bindingLayoutEntries[0].binding = 0u;
    bindingLayoutEntries[0].visibility = WGPUShaderStage_Fragment;
    bindingLayoutEntries[0].sampler.type = WGPUSamplerBindingType_Filtering;

    bindingLayoutEntries[1].binding = 1u;
    bindingLayoutEntries[1].visibility = WGPUShaderStage_Fragment;
    bindingLayoutEntries[1].texture.sampleType = WGPUTextureSampleType_Float;
    bindingLayoutEntries[1].texture.viewDimension = WGPUTextureViewDimension_2D;

    WGPUBindGroupLayoutDescriptor bindGroupLayoutDescriptor = {};
    bindGroupLayoutDescriptor.entryCount = (uint32_t)bindingLayoutEntries.size();
    bindGroupLayoutDescriptor.entries = bindingLayoutEntries.data();

    bindingLayouts[0] = wgpuDeviceCreateBindGroupLayout(wgpContext.device, &bindGroupLayoutDescriptor);

    return bindingLayouts;
}

std::vector<WGPUBindGroupLayout> ComputeParticleEasing::OnBindGroupLayoutsCompute() {
    std::vector<WGPUBindGroupLayout> bindingLayouts(1);

    std::vector<WGPUBindGroupLayoutEntry> bindingLayoutEntries(2);

    bindingLayoutEntries[0].binding = 0u;
    bindingLayoutEntries[0].visibility = WGPUShaderStage_Compute;
    bindingLayoutEntries[0].buffer.type = WGPUBufferBindingType::WGPUBufferBindingType_Uniform;
    bindingLayoutEntries[0].buffer.minBindingSize = sizeof(SimParam);

    bindingLayoutEntries[1].binding = 1u;
    bindingLayoutEntries[1].visibility = WGPUShaderStage_Compute;
    bindingLayoutEntries[1].buffer.type = WGPUBufferBindingType::WGPUBufferBindingType_Storage;
    bindingLayoutEntries[1].buffer.minBindingSize = sizeof(m_initialParticle);


    WGPUBindGroupLayoutDescriptor bindGroupLayoutDescriptor = {};
    bindGroupLayoutDescriptor.entryCount = (uint32_t)bindingLayoutEntries.size();
    bindGroupLayoutDescriptor.entries = bindingLayoutEntries.data();

    bindingLayouts[0] = wgpuDeviceCreateBindGroupLayout(wgpContext.device, &bindGroupLayoutDescriptor);

    return bindingLayouts;
}

WGPUBindGroup ComputeParticleEasing::createBindGroup() {
    std::vector<WGPUBindGroupEntry> bindings(2);

    bindings[0].binding = 0u;
    bindings[0].sampler = wgpContext.getSampler(SS_0);

    bindings[1].binding = 1u;
    bindings[1].textureView = m_wgpParticle.getTextureView();

    WGPUBindGroupDescriptor bindGroupDesc = {};
    bindGroupDesc.layout = wgpuRenderPipelineGetBindGroupLayout(wgpContext.renderPipelines.at("RP_EASING"), 0u);
    bindGroupDesc.entryCount = (uint32_t)bindings.size();
    bindGroupDesc.entries = bindings.data();

    return wgpuDeviceCreateBindGroup(wgpContext.device, &bindGroupDesc);
}

WGPUBindGroup ComputeParticleEasing::createBindGroupParticle() {
    std::vector<WGPUBindGroupEntry> bindings(2);

    bindings[0].binding = 0u;
    bindings[0].buffer = m_simBuffer.getBuffer();
    bindings[0].offset = 0u;
    bindings[0].size = wgpuBufferGetSize(m_simBuffer.getBuffer());

    bindings[1].binding = 1u;
    bindings[1].buffer = m_storageBuffer.getBuffer();
    bindings[1].offset = 0u;
    bindings[1].size = wgpuBufferGetSize(m_storageBuffer.getBuffer());

    WGPUBindGroupDescriptor bindGroupDesc = {};
    bindGroupDesc.layout = wgpuComputePipelineGetBindGroupLayout(wgpContext.computePipelines.at("CP_EASING"), 0u);
    bindGroupDesc.entryCount = (uint32_t)bindings.size();
    bindGroupDesc.entries = bindings.data();

    return wgpuDeviceCreateBindGroup(wgpContext.device, &bindGroupDesc);
}

float ComputeParticleEasing::random_float_min_max(float min, float max){
    return ((max - min) * ((float)rand() / (float)RAND_MAX)) + min;
}

float ComputeParticleEasing::random_float(){
    return random_float_min_max(0.0f, 1.0f);
}

void ComputeParticleEasing::initParticle() {
    const float current_time = Clock();
    for (uint32_t i = 0; i < (uint32_t)PARTICLE_NUM_EASING; ++i) {
        const float life = random_float() * 8000.0f + 2000.0f;
        const float age = random_float() * life;
        m_initialParticle[PROPERTY_NUM * i + 0]
                = current_time - age;                                   // start time
        m_initialParticle[PROPERTY_NUM * i + 1] = life; // life
        // position
        m_initialParticle[PROPERTY_NUM * i + 4]
                = random_float() * 2 - 1; // x
        m_initialParticle[PROPERTY_NUM * i + 5]
                = random_float() * 2 - 1; // y
        m_initialParticle[PROPERTY_NUM * i + 6]
                = random_float() * 2 - 1; // z
        m_initialParticle[PROPERTY_NUM * i + 7] = 0.0f;
        // scale
        m_initialParticle[PROPERTY_NUM * i + 8] = 0.0f; // scaleX
        m_initialParticle[PROPERTY_NUM * i + 9] = 0.0f; // scaleY
        m_initialParticle[PROPERTY_NUM * i + 10] = 0.0f; // scaleZ
        m_initialParticle[PROPERTY_NUM * i + 11] = 0.0f;
        // x
        m_initialParticle[PROPERTY_NUM * i + 12] = 0.0f; // startValue
        m_initialParticle[PROPERTY_NUM * i + 13]
                = random_float() * 2.0f - 1.0f; // endValue
        m_initialParticle[PROPERTY_NUM * i + 14]
                = (int)(random_float() * 27.0f); // ease
        // y
        m_initialParticle[PROPERTY_NUM * i + 16] = 0.0f; // startValue
        m_initialParticle[PROPERTY_NUM * i + 17]
                = random_float() * 2.0f - 1.0f; // endValue
        m_initialParticle[PROPERTY_NUM * i + 18]
                = (int)(random_float() * 27.0f); // ease
        // z
        m_initialParticle[PROPERTY_NUM * i + 20] = 0.0f; // startValue
        m_initialParticle[PROPERTY_NUM * i + 21]
                = random_float() * 2.0f - 1.0f; // endValue
        m_initialParticle[PROPERTY_NUM * i + 22]
                = (int)(random_float() * 27.0f); // ease
        // scaleX
        const float t_scale = random_float() * 12.0f;
        m_initialParticle[PROPERTY_NUM * i + 24] = 0.0f;    // startValue
        m_initialParticle[PROPERTY_NUM * i + 25] = t_scale; // endValue
        m_initialParticle[PROPERTY_NUM * i + 26] = 0.0f;    // ease
        // scaleY
        m_initialParticle[PROPERTY_NUM * i + 28] = 0.0f;    // startValue
        m_initialParticle[PROPERTY_NUM * i + 29] = t_scale; // endValue
        m_initialParticle[PROPERTY_NUM * i + 30] = 0.0f;    // ease
        // scaleZ
        m_initialParticle[PROPERTY_NUM * i + 32] = 0.0f;    // startValue
        m_initialParticle[PROPERTY_NUM * i + 33] = t_scale; // endValue
        m_initialParticle[PROPERTY_NUM * i + 34] = 0.0f;    // ease
        // alpha
        m_initialParticle[PROPERTY_NUM * i + 36]
                = random_float();                                        // startValue
        m_initialParticle[PROPERTY_NUM * i + 37] = 0.0f; // endValue
        m_initialParticle[PROPERTY_NUM * i + 38]
                = (int)(random_float() * 27.0f);                         // ease
        m_initialParticle[PROPERTY_NUM * i + 39] = 0.0f; // value
    }
}