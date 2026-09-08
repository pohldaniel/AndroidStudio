#pragma once

#include <string>
#include <oboe/Oboe.h>

extern "C" {
#include <libavformat/avformat.h>
#include <libavcodec/avcodec.h>
#include <libswresample/swresample.h>
#include <libavutil/opt.h>
}

#include "AudioRingBuffer.h"
#include "SoftwareMixer.h"
#include "ISoundEffect.h"
#include "Cache.h"

class OboeEffect : public oboe::AudioStreamDataCallback, public ISoundEffect {
    struct AVMemBuffer {
        const uint8_t* base;
        const uint8_t* ptr;
        size_t size;
        size_t total_size;
    };

    struct CacheEntry {
        CacheEntry(const std::string& file);
        ~CacheEntry();

        CacheEntry(const CacheEntry&) = delete;
        CacheEntry& operator=(const CacheEntry&) = delete;

        CacheEntry(CacheEntry&& other) noexcept;
        CacheEntry& operator=(CacheEntry&& other) noexcept;

        std::vector<float> m_samples;
        uint32_t m_totalSamples;

        AVMemBuffer m_memBuffer;
        uint8_t* m_data = nullptr;
        uint32_t m_size;

        static int Read_memory_packet(void* opaque, uint8_t* buf, int buf_size);
        static int64_t Seek_memory_packet(void* opaque, int64_t offset, int whence);
    };

public:

    OboeEffect();
    ~OboeEffect();

    void init() override;
    void play(const std::string& file) override;
    void resume();

private:

    oboe::DataCallbackResult onAudioReady(
            oboe::AudioStream *audioStream,
            void *audioData,
            int32_t numFrames) override;

    std::shared_ptr<oboe::AudioStream> m_stream;
    SoftwareMixer m_softwareMixer;

    static CacheLRU<std::string, OboeEffect::CacheEntry> Cache;
};