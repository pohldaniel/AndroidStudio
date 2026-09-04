#include <WebGPU/WgpContext.h>

#include <Nuklear/NkJoystick.h>
#include <Nuklear/NkStyle.h>

#include <States/VolumeRendering.h>
#include <States/Isometric.h>

#include "AudioDecode.h"
#include "InputTouch.h"

AudioDecode::AudioDecode(StateMachine& machine) : State(machine, States::AUDIO_DECODE)  {
    nkInit(static_cast<float>(wgpWidth), static_cast<float>(wgpHeight));
    nkInitFont("fonts/upheavtt.ttf", 47.0f);

    m_camera.perspective(glm::radians(25.0f), static_cast<float>(wgpWidth) / static_cast<float>(wgpHeight), 0.1f, 1000.0f);
    m_camera.orthographic(0.0f, static_cast<float>(wgpWidth), 0.0f, static_cast<float>(wgpHeight), -1.0f, 1.0f);
    m_camera.lookAt(glm::vec3(0.0f, 5.0f, 25.0f), glm::vec3(0.0f, 5.0f, 0.0f), glm::vec3(0.0f, 1.0f, 0.0f));
    m_camera.setRotationSpeed(0.125f);
    m_camera.setMovingSpeed(10.0f);

    wgpContext.OnDraw = std::bind(&AudioDecode::OnDraw, this, std::placeholders::_1, std::placeholders::_2);
    nkContext.OnFillBuffer = std::bind(&AudioDecode::OnFillBuffer, this, std::placeholders::_1);

    m_audioDecoder.init<OpenALPlayer>();
}

AudioDecode::~AudioDecode() {

}

void AudioDecode::fixedUpdate() {

}

void AudioDecode::update() {
    nkUpdateInput(0, 0, false, false, 0.0f);
    m_audioDecoder.update();
}

void AudioDecode::render() {
    wgpDraw();
}

void AudioDecode::OnDraw(const WGPUCommandEncoder& commandEncoder, const WGPURenderPassDescriptor& renderPassDescriptor) {
    nkDraw(commandEncoder, renderPassDescriptor);
}

void AudioDecode::OnFillBuffer(nk_context& nkCntxt) {
    int current_touch = touchStates[0].touchActive ? 0 : -1;
    set_transparent_window_style();

    float y = start_y;
    if (rounded_button(nk_rect(start_x, y, btn_w, btn_h), "Ambient", current_touch, m_isPressed)) {
        if (m_currentSong != 1) {
            m_audioDecoder.switchTrack("sounds/ambient.mp3");
            m_currentSong = 1;
        }
    }

    y = start_y + btn_h + spacing;
    if (rounded_button(nk_rect(start_x, y, btn_w, btn_h), "Paradise Found", current_touch, m_isPressed)) {
        if (m_currentSong != 2) {
            m_audioDecoder.switchTrack("sounds/paradise_found.mp3");
            m_currentSong = 2;
        }
    }

    y = start_y + (btn_h + spacing) * 2.0f;
    if (rounded_button(nk_rect(start_x, y, btn_w, btn_h), "Screen Saver", current_touch, m_isPressed)) {
        if (m_currentSong != 3) {
            m_audioDecoder.switchTrack("sounds/screen_saver.mp3");
            m_currentSong = 3;
        }
    }

    if (rounded_button(nk_rect(play_x, ctrl_y, ctrl_size * 1.5f, ctrl_size), "PLAY", current_touch, m_isPressed)) {
        m_audioDecoder.play();
    }

    if (rounded_button(nk_rect(pause_x, ctrl_y, ctrl_size * 1.5f, ctrl_size), "PAUSE", current_touch, m_isPressed)) {
       m_audioDecoder.pause();
    }

    reset_transparent_window_style();
}

void AudioDecode::resize(int deltaW, int deltaH) {
    m_camera.perspective(glm::radians(25.0f), static_cast<float>(wgpWidth) / static_cast<float>(wgpHeight), 0.1f, 1000.0f);
    m_camera.orthographic(0.0f, static_cast<float>(wgpWidth), 0.0f, static_cast<float>(wgpHeight), -1.0f, 1.0f);
}

void AudioDecode::OnButton(const Event::MouseButtonEvent& event) {
    wgpCleanState();
    nkShutDown();
    m_isRunning = false;

    if(event.button == Event::MouseButtonEvent::BUTTON_LEFT){
        m_machine.addStateAtBottom(new VolumeRendering(m_machine));
    }

    if(event.button == Event::MouseButtonEvent::BUTTON_RIGHT){
        m_machine.addStateAtBottom(new Isometric(m_machine));
    }
}