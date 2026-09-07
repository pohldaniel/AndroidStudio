#include <string>
#include <algorithm>

#include "OboePlayer.h"

OboePlayer::OboePlayer() {
    m_ringBuffer.init(44100 * 2 * sizeof(int16_t));
    m_volume = 32768;
}

OboePlayer::~OboePlayer() {
    if (m_stream) {
        m_stream->close();
    }
}

bool OboePlayer::init() {
    oboe::AudioStreamBuilder builder;
    
    builder.setDirection(oboe::Direction::Output)
           ->setPerformanceMode(oboe::PerformanceMode::LowLatency)
           ->setSharingMode(oboe::SharingMode::Exclusive)
           ->setFormat(oboe::AudioFormat::I16)
           ->setChannelCount(oboe::ChannelCount::Stereo)
           ->setSampleRate(44100)
           ->setDataCallback(this);

    oboe::Result result = builder.openStream(m_stream);
    return true;
}

void OboePlayer::resume() {
    if (m_stream) m_stream->requestStart();
}

void OboePlayer::pause() {
    if (m_stream) m_stream->requestPause();
}

void OboePlayer::flush() {
    if (m_stream) {
        m_stream->requestStop(); 
    }
    m_audioAccumulator.clear();
    m_ringBuffer.clear();
}

void OboePlayer::enqueueData(const std::vector<uint8_t>& pcmData) {
    if (!pcmData.empty()) {
        m_audioAccumulator.insert(m_audioAccumulator.end(), pcmData.begin(), pcmData.end());
    }

    if (m_audioAccumulator.size() < 4096) return;

    size_t availableWrite = m_ringBuffer.getAvailableWrite();
    if (availableWrite > 0 && !m_audioAccumulator.empty()) {
        size_t toWrite = std::min(availableWrite, m_audioAccumulator.size());
        m_ringBuffer.write(m_audioAccumulator.data(), toWrite);
        m_audioAccumulator.erase(m_audioAccumulator.begin(), m_audioAccumulator.begin() + toWrite);
    }
    resume();
}

oboe::DataCallbackResult OboePlayer::onAudioReady(
    oboe::AudioStream *audioStream, 
    void *audioData, 
    int32_t numFrames) {

    size_t bytesNeeded = numFrames * 2 * sizeof(int16_t);
    int16_t* out = static_cast<int16_t*>(audioData);

    size_t bytesRead = m_ringBuffer.read(reinterpret_cast<uint8_t*>(out), bytesNeeded);

    if (bytesRead < bytesNeeded) {
        std::memset(reinterpret_cast<uint8_t*>(out) + bytesRead, 0, bytesNeeded - bytesRead);
    }

    int32_t currentVolume = m_volume;
    int32_t totalSamples = numFrames * 2;
    std::transform(out, out + totalSamples, out, [currentVolume](int16_t sample) {
        int32_t intermediate = static_cast<int32_t>(sample) * currentVolume;
        return static_cast<int16_t>(intermediate >> 15);
    });

    return oboe::DataCallbackResult::Continue;
}

void OboePlayer::setVolume(float volume){
    m_volume = static_cast<int32_t>(volume * 32768.0f);
}

float OboePlayer::getVolume() {
    return static_cast<float>(m_volume) / 32768.0f;
}