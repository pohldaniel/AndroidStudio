#pragma once

#include <webgpu.h>
#include <Nuklear/NkContext.h>
#include <Sound/AudioDecoder.h>
#include <States/StateMachine.h>
#include <core/Camera.h>

class AudioDecode : public State {

public:

    AudioDecode(StateMachine& machine);
    ~AudioDecode() override;

    void fixedUpdate() override;
    void update() override;
    void render() override;
    void resize(int deltaW, int deltaH) override;

    void OnDraw(const WGPUCommandEncoder& commandEncoder, const WGPURenderPassDescriptor& renderPassDescriptor);
    void OnFillBuffer(nk_context& nkCntxt);
    void OnButton(const Event::MouseButtonEvent& event) override;

private:

    Camera m_camera;
    AudioDecoder m_audioDecoder;

    float btn_w = 800.0f;
    float btn_h = 180.0f;
    float spacing = 40.0f;
    float start_x = (wgpWidth - btn_w) / 2.0f;
    float total_block_h = (3.0f * btn_h) + (2.0f * spacing);
    float start_y = (wgpHeight - total_block_h) / 2.0f;

    float ctrl_size = 160.0f;
    float side_padding = 50.0f;

    float bottom_margin = 250.0f;
    float ctrl_y = wgpHeight - ctrl_size - bottom_margin;
    float play_x = side_padding;
    float pause_x = wgpWidth - ctrl_size * 1.5f - side_padding;

    int m_currentSong = 0;
    bool m_isPressed = false;
};