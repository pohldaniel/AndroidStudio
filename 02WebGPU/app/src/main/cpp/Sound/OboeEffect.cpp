#include "OboeEffect.h"
#include "AssetIO.h"

CacheLRU<std::string, OboeEffect::CacheEntry> OboeEffect::Cache;

OboeEffect::OboeEffect(){

}

OboeEffect::~OboeEffect(){

}

void OboeEffect::init() {
    oboe::AudioStreamBuilder builder;

    builder.setDirection(oboe::Direction::Output)
            ->setPerformanceMode(oboe::PerformanceMode::LowLatency)
            ->setSharingMode(oboe::SharingMode::Exclusive)
            ->setFormat(oboe::AudioFormat::I16)
            ->setChannelCount(oboe::ChannelCount::Stereo)
            ->setSampleRate(44100)
            ->setDataCallback(this);

    oboe::Result result = builder.openStream(m_stream);
}

void OboeEffect::play(const std::string& file) {
    const CacheEntry& entry = Cache.Get(file);
    if (entry.m_samples.empty())
        return;

    bool channelFound = false;
    for (auto& channel : m_softwareMixer.m_channels) {
        int expected = 0;
        if (channel.status = 1) {
            channel.pcmData = &entry.m_samples;
            channel.progress = 0;
            channel.pitchFactor = 1.0f;
            channelFound = true;
            break;
        }
    }

    if (!channelFound) {
        size_t maxProgress = 0;
        ActiveSound* oldestChannel = nullptr;
        for (auto& channel : m_softwareMixer.m_channels) {
            if (channel.progress > maxProgress) {
                maxProgress = channel.progress;
                oldestChannel = &channel;
            }
        }

        if (oldestChannel) {
            oldestChannel->pcmData = &entry.m_samples;
            oldestChannel->progress = 0;
            oldestChannel->status = 1;
        }
    }

    resume();
}

oboe::DataCallbackResult OboeEffect::onAudioReady(
        oboe::AudioStream *audioStream,
        void *audioData,
        int32_t numFrames) {

    size_t samplesNeeded = numFrames * 2;
    size_t bytesNeeded = samplesNeeded * sizeof(int16_t);
    int16_t* out = static_cast<int16_t*>(audioData);

    size_t bytesRead = m_ringBuffer.read(reinterpret_cast<uint8_t*>(out), bytesNeeded);

    if (bytesRead < bytesNeeded) {
        std::memset(reinterpret_cast<uint8_t*>(out) + bytesRead, 0, bytesNeeded - bytesRead);
    }

    m_softwareMixer.mixAudio(out, static_cast<int32_t>(samplesNeeded));

    return oboe::DataCallbackResult::Continue;
}

void OboeEffect::resume() {
    if (m_stream) m_stream->requestStart();
}

int OboeEffect::CacheEntry::Read_memory_packet(void* opaque, uint8_t* buf, int buf_size) {
    AVMemBuffer* bd = static_cast<AVMemBuffer*>(opaque);
    if (!bd || bd->size == 0) return AVERROR_EOF;

    int read_bytes = std::min(buf_size, static_cast<int>(bd->size));

    std::memcpy(buf, bd->ptr, read_bytes);
    bd->ptr += read_bytes;
    bd->size -= read_bytes;

    return read_bytes;
}

int64_t OboeEffect::CacheEntry::Seek_memory_packet(void* opaque, int64_t offset, int whence) {
    AVMemBuffer* bd = static_cast<AVMemBuffer*>(opaque);
    if (!bd) return -1;

    int64_t new_offset = 0;

    switch (whence) {
        case AVSEEK_SIZE:
            return bd->total_size;
        case SEEK_SET:
            new_offset = offset;
            break;
        case SEEK_CUR:
            new_offset = static_cast<int64_t>(bd->ptr - bd->base) + offset;
            break;
        case SEEK_END:
            new_offset = static_cast<int64_t>(bd->total_size) + offset;
            break;
        default:
            return -1;
    }

    if (new_offset < 0 || static_cast<size_t>(new_offset) > bd->total_size) {
        return -1;
    }

    bd->ptr = bd->base + new_offset;
    bd->size = bd->total_size - new_offset;

    return new_offset;
}

OboeEffect::CacheEntry::CacheEntry(const std::string& file) {
    AssetIO::LoadAsset(file.c_str(), m_data, m_size);

    m_memBuffer.base = m_data;
    m_memBuffer.ptr = m_data;
    m_memBuffer.size = m_size;
    m_memBuffer.total_size = m_size;

    AVFormatContext* formatCtx  = avformat_alloc_context();

    size_t avio_ctx_buffer_size = 4096;
    uint8_t* avio_ctx_buffer = static_cast<uint8_t*>(av_malloc(avio_ctx_buffer_size + AV_INPUT_BUFFER_PADDING_SIZE));

    if (!avio_ctx_buffer) {
        return;
    }

    AVIOContext* avio_ctx = avio_alloc_context(
            avio_ctx_buffer, avio_ctx_buffer_size, 0,
            &m_memBuffer, &Read_memory_packet, nullptr, &Seek_memory_packet
    );

    if (!avio_ctx) {
        av_freep(&avio_ctx_buffer);
        return;
    }

    formatCtx->pb = avio_ctx;
    formatCtx->flags |= AVFMT_FLAG_CUSTOM_IO;

    avformat_open_input(&formatCtx, nullptr, nullptr, nullptr);
    avformat_find_stream_info(formatCtx, nullptr);

    int streamIdx = -1;
    for (unsigned int i = 0; i < formatCtx->nb_streams; i++) {
        if (formatCtx->streams[i]->codecpar->codec_type == AVMEDIA_TYPE_AUDIO) {
            streamIdx = i;
            break;
        }
    }

    const AVCodec* codec = avcodec_find_decoder(formatCtx->streams[streamIdx]->codecpar->codec_id);
    AVCodecContext* codecCtx = avcodec_alloc_context3(codec);
    avcodec_parameters_to_context(codecCtx, formatCtx->streams[streamIdx]->codecpar);
    avcodec_open2(codecCtx, codec, nullptr);

    SwrContext* swr = swr_alloc();
    av_opt_set_chlayout(swr, "in_chlayout", &codecCtx->ch_layout, 0);
    av_opt_set_int(swr, "in_sample_rate", codecCtx->sample_rate, 0);
    av_opt_set_sample_fmt(swr, "in_sample_fmt", codecCtx->sample_fmt, 0);

    AVChannelLayout outLayout;
    av_channel_layout_default(&outLayout, 2);
    av_opt_set_chlayout(swr, "out_chlayout", &outLayout, 0);
    av_opt_set_int(swr, "out_sample_rate", 44100, 0);
    av_opt_set_sample_fmt(swr, "out_sample_fmt", AV_SAMPLE_FMT_S16, 0);
    swr_init(swr);

    AVPacket* packet = av_packet_alloc();
    AVFrame* frame = av_frame_alloc();
    std::vector<uint8_t> pcmData;

    while (av_read_frame(formatCtx, packet) >= 0) {
        if (packet->stream_index == streamIdx) {
            int send_ret = avcodec_send_packet(codecCtx, packet);
            if (send_ret >= 0) {
                int ret = 0;
                while (true) {
                    ret = avcodec_receive_frame(codecCtx, frame);

                    if (ret == AVERROR(EAGAIN) || ret == AVERROR_EOF) {
                        break;
                    }else if (ret < 0) {
                        break;
                    }

                    int outSamples = swr_get_out_samples(swr, frame->nb_samples);
                    if (outSamples <= 0) {
                        av_frame_unref(frame);
                        continue;
                    }

                    int maxFrameSize = outSamples * 2 * sizeof(int16_t);
                    size_t oldSize = pcmData.size();
                    pcmData.resize(oldSize + maxFrameSize);
                    uint8_t* buffer = pcmData.data() + oldSize;

                    int convertedSamples = swr_convert(swr, &buffer, outSamples, (const uint8_t**)frame->data, frame->nb_samples);
                    if (convertedSamples >= 0) {
                        int actualFrameSize = convertedSamples * 2 * sizeof(int16_t);
                        pcmData.resize(oldSize + actualFrameSize);
                    }else {
                        pcmData.resize(oldSize);
                    }
                    av_frame_unref(frame);
                }
            }
        }
        av_packet_unref(packet);
    }

    av_frame_free(&frame);
    av_packet_free(&packet);
    swr_free(&swr);
    avcodec_free_context(&codecCtx);
    avformat_close_input(&formatCtx);

    m_samples.resize(pcmData.size() / sizeof(int16_t));
    std::memcpy(m_samples.data(), pcmData.data(), pcmData.size());
    m_totalSamples = m_samples.size();

    AssetIO::Free(m_data);
}

OboeEffect::CacheEntry::~CacheEntry() {

}

OboeEffect::CacheEntry::CacheEntry(CacheEntry&& other) noexcept : m_samples(std::move(other.m_samples)), m_totalSamples(other.m_totalSamples){
    other.m_totalSamples = 0u;
}

OboeEffect::CacheEntry& OboeEffect::CacheEntry::operator=(CacheEntry&& other) noexcept {
    if (this != &other) {
        m_samples = std::move(other.m_samples);
        m_totalSamples = other.m_totalSamples;
        other.m_totalSamples = 0u;
    }
    return *this;
}