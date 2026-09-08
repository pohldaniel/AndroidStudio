#include <string>
#include <algorithm>

#include "OboePlayer.h"

OboePlayer::OboePlayer() {
    m_ringBuffer.init(44100 * 2 * sizeof(int16_t));
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
           ->setFormat(oboe::AudioFormat::Float)
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
    m_accumulator.clear();
    m_ringBuffer.clear();
}

void OboePlayer::enqueueData(const std::vector<float>& pcmData) {
    if (!pcmData.empty()) {
        m_accumulator.insert(m_accumulator.end(), pcmData.begin(), pcmData.end());
    }

    if (m_accumulator.size() < 4096) {
        return;
    }

    size_t availableWrite = m_ringBuffer.getAvailableWrite();
    if (availableWrite > 0 && !m_accumulator.empty()) {
        size_t toWrite = std::min(availableWrite, m_accumulator.size());
        m_ringBuffer.write(m_accumulator.data(), toWrite);
        m_accumulator.erase(m_accumulator.begin(), m_accumulator.begin() + toWrite);
    }
    resume();
}

oboe::DataCallbackResult OboePlayer::onAudioReady(
    oboe::AudioStream *audioStream, 
    void *audioData, 
    int32_t numFrames) {

    size_t samplesNeeded = numFrames * 2;
    float* out = static_cast<float*>(audioData);
    size_t samplesRead = m_ringBuffer.read(out, samplesNeeded);
    if (samplesRead < samplesNeeded) {
        std::fill_n(out + samplesRead, samplesNeeded - samplesRead, 0.0f);
    }

    m_softwareMixer.mixAudio(out, static_cast<int32_t>(samplesNeeded));

    return oboe::DataCallbackResult::Continue;
}

void OboePlayer::setVolume(float volume){
    m_softwareMixer.setVolume(volume);
}

float OboePlayer::getVolume() {
    return m_softwareMixer.getVolume();
}