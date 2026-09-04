#pragma once

#include <vector>
#include <oboe/Oboe.h>

#include "AudioRingBuffer.h"

class OboePlayer : public oboe::AudioStreamDataCallback {
public:
    OboePlayer();
    ~OboePlayer();

    bool init();
    void start();
    void pause();
    void flush();
    
    void enqueueData(const std::vector<uint8_t>& pcmData);

private:

    oboe::DataCallbackResult onAudioReady(
        oboe::AudioStream *audioStream,
        void *audioData, 
        int32_t numFrames) override;

    std::shared_ptr<oboe::AudioStream> m_stream;
    AudioRingBuffer m_ringBuffer;
    std::vector<uint8_t> m_audioAccumulator;
};