#pragma once
#include <algorithm>

#include <AL/alc.h>
#include <AL/alext.h>

#include "IAudioOutput.h"

class OpenALPlayer : public IAudioOutput {

    friend class AudioDecoder;

public:

    OpenALPlayer();
    ~OpenALPlayer() override;

    bool init() override;
    void enqueueData(const std::vector<float>& pcmData) override;
    void pause() override;
    void resume() override;

    void setVolume(float volume) override;
    float getVolume() override;

private:
    
    void flush() override;

    ALuint m_source = 0u;
    ALuint m_buffers[2] = { 0u, 0u };
    std::vector<float> m_audioAccumulator;
};