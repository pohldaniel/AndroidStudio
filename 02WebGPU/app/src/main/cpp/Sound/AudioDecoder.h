#pragma once
#include <string>
#include <vector>

extern "C" {
#include <libavformat/avformat.h>
#include <libavcodec/avcodec.h>
#include <libswresample/swresample.h>
#include <libavutil/opt.h>
}

#include "OpenALPlayer.h"
#include "OboePlayer.h"

class AudioDecoder {
    struct AVMemBuffer {
        const uint8_t* base;
        const uint8_t* ptr;
        size_t size;
        size_t total_size;
    };
public:

    AudioDecoder();
    ~AudioDecoder();

    template <typename AudioImpl = OpenALPlayer>
    void init() {
        auto audio = std::make_unique<AudioImpl>();
        init(std::move(audio));
    }

    template <typename AudioImpl = OpenALPlayer>
    void open(const std::string& filename) {
        auto audio = std::make_unique<AudioImpl>();
        open(filename, std::move(audio));
    }

    template <class AudioImpl = OpenALPlayer>
    AudioImpl* get() {
        return static_cast<AudioImpl*>(m_audioOutput.get());
    }

    void playTrack(const std::string& filename);
    void close();
    void update();

    void play();
    void pause();

private:

    void init(std::unique_ptr<IAudioOutput> audioOutput = nullptr);
    void open(const std::string& filename, std::unique_ptr<IAudioOutput> audioOutput = nullptr);
    void queryFirstFrame();
    bool decodeAudioFrame(std::vector<float>& outPcmData);

    AVFormatContext* m_formatContext = nullptr;
    AVCodecContext* m_codecContext = nullptr;
    SwrContext* m_swrContext = nullptr;
    int m_audioStreamIndex = -1;
    bool m_isPaused = true;

    AVPacket* m_packet = nullptr;
    AVFrame* m_frame = nullptr;

    std::unique_ptr<IAudioOutput> m_audioOutput = nullptr;

    AVMemBuffer m_memBuffer;

    uint8_t* m_data = nullptr;
    uint32_t m_size;

    static int Read_memory_packet(void* opaque, uint8_t* buf, int buf_size);
    static int64_t Seek_memory_packet(void* opaque, int64_t offset, int whence);
};