#pragma once

#include <vector>

extern "C" {
#include <libavformat/avformat.h>
#include <libavcodec/avcodec.h>
#include <libswresample/swresample.h>
#include <libavutil/opt.h>
}

#include <AL/al.h>

#include "ISoundEffect.h"
#include "Cache.h"

class OpenALEffect : public ISoundEffect {
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

        ALuint buffer;

        AVMemBuffer m_memBuffer;
        uint8_t* m_data = nullptr;
        uint32_t m_size;

        static int Read_memory_packet(void* opaque, uint8_t* buf, int buf_size);
        static int64_t Seek_memory_packet(void* opaque, int64_t offset, int whence);
    };

public:

    OpenALEffect();
    ~OpenALEffect();

    void init() override;
    void play(const std::string& file) override;
    void setVolume(float volume);

private:

    std::vector<ALuint> m_sources;
    size_t m_next;

    static CacheLRU<std::string, OpenALEffect::CacheEntry> Cache;
};