#pragma once

#include <webgpu.h>
#include <Nuklear/NkContext.h>
#include <Video/VideoDecoder.h>
#include <States/StateMachine.h>
#include <core/Camera.h>

class VideoDecode : public State {

public:

    VideoDecode(StateMachine& machine);
    ~VideoDecode() override;

    void fixedUpdate() override;
    void update() override;
    void render() override;
    void resize(int deltaW, int deltaH) override;

    void OnDraw(const WGPUCommandEncoder& commandEncoder, const WGPURenderPassDescriptor& renderPassDescriptor);
    void OnFillBuffer(nk_context& nkCntxt);
    void OnButton(const Event::MouseButtonEvent& event) override;

private:

    std::vector<WGPUBindGroupLayout> OnBindGroupLayouts();
    WGPUBindGroup createBindGroup();

    Camera m_camera;
    VideoDecoder m_videoDecoder;

    float ctrl_size = 160.0f;
    float side_padding = 50.0f;

    float bottom_margin = 250.0f;
    float ctrl_y = wgpHeight - ctrl_size - bottom_margin;
    float play_x = side_padding;
    float pause_x = wgpWidth - ctrl_size * 1.5f - side_padding;
    bool m_isPressed = false;
};