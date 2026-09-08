#pragma once

#include <vector>
#include <oboe/Oboe.h>

#include "IAudioOutput.h"
#include "AudioRingBuffer.h"
#include "SoftwareMixer.h"

class OboePlayer : public oboe::AudioStreamDataCallback, public IAudioOutput{
public:
    OboePlayer();
    ~OboePlayer();

    bool init() override;
    void enqueueData(const std::vector<float>& pcmData) override;
    void pause() override;
    void resume() override;

    void setVolume(float volume) override;
    float getVolume() override;

private:

    void flush() override;

    oboe::DataCallbackResult onAudioReady(
        oboe::AudioStream *audioStream,
        void *audioData, 
        int32_t numFrames) override;

    std::shared_ptr<oboe::AudioStream> m_stream;
    AudioRingBuffer m_ringBuffer;
    std::vector<float> m_accumulator;

    SoftwareMixer m_softwareMixer;
};