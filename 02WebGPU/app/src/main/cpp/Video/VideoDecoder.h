#pragma once
#include <vector>
#include <string>
#include <memory>

#include <Sound/IAudioOutput.h>
#include <Sound/OpenALPlayer.h>

#include "IVideoDecoder.h"
#include "YUVDecoder.h"
#include "RGBADecoder.h"

extern "C" {
#include <libavformat/avformat.h>
#include <libavcodec/avcodec.h>
#include <libswresample/swresample.h>
#include <libavutil/opt.h>
}

class VideoDecoder {
    struct AVMemBuffer {
        const uint8_t* base;
        const uint8_t* ptr;
        size_t size;
        size_t total_size;
    };
public:
   
    VideoDecoder();
    ~VideoDecoder();

    template <typename VideoImpl = YUVDecoder, typename AudioImpl = OpenALPlayer>
    void open(const std::string& filename) {
        auto decoder = std::make_unique<VideoImpl>();
        auto audio = std::make_unique<AudioImpl>();
        open(filename, std::move(decoder), std::move(audio));
    }

    template <class VideoImpl = IVideoDecoder>
    VideoImpl* getDecoder() {
        return static_cast<VideoImpl*>(m_decoder.get());
    }

    template <class AudioImpl = IAudioOutput>
    AudioImpl* getAudioOutput() {
        return static_cast<AudioImpl*>(m_audioOutput.get());
    }

    void close();
    void update(float deltaTime);
    void OnPostDraw();

    void play();
    void pause();

    void seekTo(float seconds);
    void setVolume(float volume);
    float getVolume();
    void queryFirstFrame();

private:

    void init(std::unique_ptr<IVideoDecoder> videoDecoder, std::unique_ptr<IAudioOutput> audioOutput);
    void open(const std::string& filename, std::unique_ptr<IVideoDecoder> videoDecoder = nullptr, std::unique_ptr<IAudioOutput> audioOutput = nullptr);

    bool decodeVideoFrame();
    bool decodeAudioFrame(std::vector<float>& outPcmData);
   
    AVBufferRef* m_hwDeviceContext = nullptr;
    AVPacket* m_packet = nullptr;
    AVFrame* m_videoFrame = nullptr;
    AVFrame* m_audioFrame = nullptr;

    AVFormatContext* m_formatContext = nullptr;
    AVCodecContext* m_audioCodecContext = nullptr;
    AVCodecContext* m_videoCodecContext = nullptr;
    SwrContext* m_swrContext = nullptr;

    bool m_isPaused = false;
    float m_accumulator = 0.0f;
    float m_timePerFrame = 0.0f;
    float m_currentTime = 0.0f;
    float m_videoTimebase = 0.0f;
    float m_fps = 30.0f;
    float m_duration = 0.0f;
   
    int m_videoStreamIndex = -1;
    int m_audioStreamIndex = -1;
    bool m_wantSeek = false;

    std::unique_ptr<IAudioOutput> m_audioOutput = nullptr;
    std::unique_ptr<IVideoDecoder> m_decoder = nullptr;

    AVMemBuffer m_memBuffer;
    uint8_t* m_data = nullptr;
    uint32_t m_size;

    static int Read_memory_packet(void* opaque, uint8_t* buf, int buf_size);
    static int64_t Seek_memory_packet(void* opaque, int64_t offset, int whence);
};