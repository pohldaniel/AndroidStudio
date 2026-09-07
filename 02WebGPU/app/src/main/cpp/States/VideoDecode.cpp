#include <WebGPU/WgpContext.h>

#include <Nuklear/NkJoystick.h>
#include <Nuklear/NkStyle.h>

#include <States/AudioDecode.h>
#include <States/Isometric.h>

#include "VideoDecode.h"
#include "InputTouch.h"

VideoDecode::VideoDecode(StateMachine& machine) : State(machine, States::VIDEO_DECODE)  {
    nkInit(static_cast<float>(wgpWidth), static_cast<float>(wgpHeight));
    nkInitFont("fonts/upheavtt.ttf", 47.0f);

    m_camera.perspective(glm::radians(25.0f), static_cast<float>(wgpWidth) / static_cast<float>(wgpHeight), 0.1f, 1000.0f);
    m_camera.orthographic(0.0f, static_cast<float>(wgpWidth), 0.0f, static_cast<float>(wgpHeight), -1.0f, 1.0f);
    m_camera.lookAt(glm::vec3(0.0f, 5.0f, 25.0f), glm::vec3(0.0f, 5.0f, 0.0f), glm::vec3(0.0f, 1.0f, 0.0f));
    m_camera.setRotationSpeed(0.125f);
    m_camera.setMovingSpeed(10.0f);

    wgpContext.addSahderModule("VIDEO", "shader/video_yuv.wgsl");
    wgpContext.createRenderPipeline("VIDEO", "RP_VIDEO", VL_NONE, std::bind(&VideoDecode::OnBindGroupLayouts, this));

    m_videoDecoder.open<YUVDecoder, OpenALPlayer>("videos/big_buck_bunny.mp4");
    m_videoDecoder.getDecoder<YUVDecoder>()->setBindGroup(createBindGroup());

    m_videoDecoder.queryFirstFrame();

    wgpContext.OnDraw = std::bind(&VideoDecode::OnDraw, this, std::placeholders::_1, std::placeholders::_2);
    nkContext.OnFillBuffer = std::bind(&VideoDecode::OnFillBuffer, this, std::placeholders::_1);
}

VideoDecode::~VideoDecode() {

}

void VideoDecode::fixedUpdate() {

}

void VideoDecode::update() {
    nkUpdateInput(0, 0, false, false, 0.0f);
    m_videoDecoder.update(m_dt);
}

void VideoDecode::render() {
    wgpDraw();
}

void VideoDecode::OnDraw(const WGPUCommandEncoder& commandEncoder, const WGPURenderPassDescriptor& renderPassDescriptor) {

    {
        WGPURenderPassEncoder renderPassEncoder = wgpuCommandEncoderBeginRenderPass(commandEncoder,&renderPassDescriptor);
        wgpuRenderPassEncoderSetPipeline(renderPassEncoder,wgpContext.renderPipelines.at("RP_VIDEO"));
        wgpuRenderPassEncoderSetViewport(renderPassEncoder, 0.0f, 0.0f, wgpWidth, wgpHeight, 0.0f,1.0f);
        wgpuRenderPassEncoderSetBindGroup(renderPassEncoder, 0u,m_videoDecoder.getDecoder()->getBindGroup(), 0u, NULL);
        wgpuRenderPassEncoderDraw(renderPassEncoder, 3u, 1u, 0u, 0u);
        wgpuRenderPassEncoderEnd(renderPassEncoder);
        wgpuRenderPassEncoderRelease(renderPassEncoder);
    }

    {
        WGPURenderPassColorAttachment renderPassColorAttachment = renderPassDescriptor.colorAttachments[0];
        renderPassColorAttachment.loadOp = WGPULoadOp::WGPULoadOp_Load;

        WGPURenderPassDescriptor rndrPssDscrptor = renderPassDescriptor;
        rndrPssDscrptor.colorAttachments = &renderPassColorAttachment;

        nkDraw(commandEncoder, rndrPssDscrptor);
    }
}

void VideoDecode::OnFillBuffer(nk_context& nkCntxt) {
    int current_touch = touchStates[0].touchActive ? 0 : -1;

    bottom_margin = (wgpWidth > wgpHeight) ? 160.0f : 250.0f;
    ctrl_y = static_cast<float>(wgpHeight) - ctrl_size - bottom_margin;
    play_x = side_padding;
    pause_x = static_cast<float>(wgpWidth) - (ctrl_size * 1.5f) - side_padding;

    if (ctrl_y + ctrl_size > static_cast<float>(wgpHeight)) {
        ctrl_y = static_cast<float>(wgpHeight) - ctrl_size;
    }

    set_transparent_window_style();
    if (rounded_button(nk_rect(play_x, ctrl_y, ctrl_size * 1.5f, ctrl_size), "PLAY", current_touch, m_isPressed)) {
        m_videoDecoder.play();
    }

    if (rounded_button(nk_rect(pause_x, ctrl_y, ctrl_size * 1.5f, ctrl_size), "PAUSE", current_touch, m_isPressed)) {
        m_videoDecoder.pause();
    }

    reset_transparent_window_style();
}

void VideoDecode::resize(int deltaW, int deltaH) {
    nkResize(static_cast<float>(wgpWidth), static_cast<float>(wgpHeight));
    m_camera.perspective(glm::radians(25.0f), static_cast<float>(wgpWidth) / static_cast<float>(wgpHeight), 0.1f, 1000.0f);
    m_camera.orthographic(0.0f, static_cast<float>(wgpWidth), 0.0f, static_cast<float>(wgpHeight), -1.0f, 1.0f);
}

void VideoDecode::OnButton(const Event::MouseButtonEvent& event) {
    wgpCleanState();
    nkShutDown();
    m_isRunning = false;

    if(event.button == Event::MouseButtonEvent::BUTTON_LEFT){
        m_machine.addStateAtBottom(new AudioDecode(m_machine));
    }

    if(event.button == Event::MouseButtonEvent::BUTTON_RIGHT){
        m_machine.addStateAtBottom(new Isometric(m_machine));
    }
}

std::vector<WGPUBindGroupLayout> VideoDecode::OnBindGroupLayouts() {
    std::vector<WGPUBindGroupLayout> bindingLayouts(1);

    std::vector<WGPUBindGroupLayoutEntry> bindingLayoutEntries(2);
    bindingLayoutEntries[0].binding = 0u;
    bindingLayoutEntries[0].visibility = WGPUShaderStage_Fragment;
    bindingLayoutEntries[0].sampler.type = WGPUSamplerBindingType_Filtering;

    bindingLayoutEntries[1].binding = 1u;
    bindingLayoutEntries[1].visibility = WGPUShaderStage_Fragment;
    bindingLayoutEntries[1].texture.viewDimension = WGPUTextureViewDimension_2D;
    bindingLayoutEntries[1].texture.sampleType = WGPUTextureSampleType_Float;

    WGPUBindGroupLayoutDescriptor bindGroupLayoutDescriptor = {};
    bindGroupLayoutDescriptor.entryCount = (uint32_t)bindingLayoutEntries.size();
    bindGroupLayoutDescriptor.entries = bindingLayoutEntries.data();

    bindingLayouts[0] = wgpuDeviceCreateBindGroupLayout(wgpContext.device, &bindGroupLayoutDescriptor);

    return bindingLayouts;
}

WGPUBindGroup VideoDecode::createBindGroup() {
    std::vector<WGPUBindGroupEntry> entries(2);

    entries[0].binding = 0u;
    entries[0].sampler = wgpContext.getSampler(SS_LINEAR_CLAMP);

    entries[1].binding = 1u;
    entries[1].textureView = m_videoDecoder.getDecoder()->getTextureViewY();

    WGPUBindGroupDescriptor bindGroupDesc = {};
    bindGroupDesc.layout = wgpuRenderPipelineGetBindGroupLayout(wgpContext.renderPipelines.at("RP_VIDEO"), 0u);
    bindGroupDesc.entryCount = (uint32_t)entries.size();
    bindGroupDesc.entries = (WGPUBindGroupEntry*)entries.data();
    return wgpuDeviceCreateBindGroup(wgpContext.device, &bindGroupDesc);
}