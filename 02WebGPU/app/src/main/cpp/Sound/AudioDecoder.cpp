#include "AudioDecoder.h"
#include "AssetIO.h"

AudioDecoder::AudioDecoder() {
    m_packet = av_packet_alloc();
    m_frame = av_frame_alloc();
    //av_log_set_level(AV_LOG_DEBUG);
    av_log_set_level(AV_LOG_ERROR);
}

AudioDecoder::~AudioDecoder() {
    close();
    av_packet_free(&m_packet);
    av_frame_free(&m_frame);
}

int read_memory_packet(void* opaque, uint8_t* buf, int buf_size) {
    AVMemBuffer* bd = static_cast<AVMemBuffer*>(opaque);
    if (!bd || bd->size == 0) return AVERROR_EOF;

    int read_bytes = std::min(buf_size, static_cast<int>(bd->size));

    std::memcpy(buf, bd->ptr, read_bytes);
    bd->ptr += read_bytes;
    bd->size -= read_bytes;

    return read_bytes;
}

int64_t seek_memory_packet(void* opaque, int64_t offset, int whence) {
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

void AudioDecoder::init(std::unique_ptr<IAudioOutput> audioOutput){
    if (audioOutput) {
        m_audioOutput = std::move(audioOutput);
        m_audioOutput->init();
    }
}

void AudioDecoder::open(const std::string& filename, std::unique_ptr<IAudioOutput> audioOutput) {

    AssetIO::LoadAsset(filename.c_str(), m_data, m_size);

    m_memBuffer.base = m_data;
    m_memBuffer.ptr = m_data;
    m_memBuffer.size = m_size;
    m_memBuffer.total_size = m_size;

    m_formatContext = avformat_alloc_context();
    if (!m_formatContext) return;

    size_t avio_ctx_buffer_size = 4096;
    uint8_t* avio_ctx_buffer = static_cast<uint8_t*>(av_malloc(avio_ctx_buffer_size + AV_INPUT_BUFFER_PADDING_SIZE));

    if (!avio_ctx_buffer) {
        return;
    }

    AVIOContext* avio_ctx = avio_alloc_context(
            avio_ctx_buffer, avio_ctx_buffer_size, 0,
            &m_memBuffer, &read_memory_packet, nullptr, &seek_memory_packet
    );

    if (!avio_ctx) {
        av_freep(&avio_ctx_buffer);
        return;
    }

    m_formatContext->pb = avio_ctx;
    m_formatContext->flags |= AVFMT_FLAG_CUSTOM_IO;

    if (avformat_open_input(&m_formatContext, filename.c_str(), nullptr, nullptr) < 0) return;
    if (avformat_find_stream_info(m_formatContext, nullptr) < 0) return;

    for (unsigned int i = 0; i < m_formatContext->nb_streams; i++) {
        if (m_formatContext->streams[i]->codecpar->codec_type == AVMEDIA_TYPE_AUDIO) {
            m_audioStreamIndex = i;
            break;
        }
    }
    if (m_audioStreamIndex == -1) return;

    const AVCodec* codec = avcodec_find_decoder(m_formatContext->streams[m_audioStreamIndex]->codecpar->codec_id);
    m_codecContext = avcodec_alloc_context3(codec);
    avcodec_parameters_to_context(m_codecContext, m_formatContext->streams[m_audioStreamIndex]->codecpar);
    avcodec_open2(m_codecContext, codec, nullptr);

    m_swrContext = swr_alloc();
    av_opt_set_chlayout(m_swrContext, "in_chlayout", &m_codecContext->ch_layout, 0);
    av_opt_set_int(m_swrContext, "in_sample_rate", m_codecContext->sample_rate, 0);
    av_opt_set_sample_fmt(m_swrContext, "in_sample_fmt", m_codecContext->sample_fmt, 0);

    AVChannelLayout outLayout;
    av_channel_layout_default(&outLayout, 2);
    av_opt_set_chlayout(m_swrContext, "out_chlayout", &outLayout, 0);
    av_opt_set_int(m_swrContext, "out_sample_rate", 44100, 0);
    av_opt_set_sample_fmt(m_swrContext, "out_sample_fmt", AV_SAMPLE_FMT_S16, 0);
    swr_init(m_swrContext);
    init(std::move(audioOutput));
    queryFirstFrame();

    m_isPaused = false;
}

void AudioDecoder::switchTrack(const std::string& filename) {
    close();
    if(m_audioOutput)
        m_audioOutput->flush();

    open(filename, nullptr);
}

void AudioDecoder::update() {
    if (m_isPaused) {
        return;
    }

    int ret = av_read_frame(m_formatContext, m_packet);

    if(ret == AVERROR_EOF) {
        av_seek_frame(m_formatContext, m_audioStreamIndex, 0, AVSEEK_FLAG_BYTE);
        avcodec_flush_buffers(m_codecContext);
        ret = av_read_frame(m_formatContext, m_packet);
    }

    if(ret >= 0) {
        if (m_packet->stream_index == m_audioStreamIndex && m_swrContext) {
            std::vector<uint8_t> pcmData;
            if (decodeAudioFrame(pcmData)) {
                m_audioOutput->enqueueData(pcmData);
            }
        }
        av_packet_unref(m_packet);      
    }
}

void AudioDecoder::queryFirstFrame() {
    int ret = 0;
    bool frameFound = false;

    while (av_read_frame(m_formatContext, m_packet) >= 0) {
        if (m_packet->stream_index == m_audioStreamIndex) {
            if (avcodec_send_packet(m_codecContext, m_packet) >= 0) {
                ret = avcodec_receive_frame(m_codecContext, m_frame);

                if (ret == 0) {
                    frameFound = true;
                    av_frame_unref(m_frame);
                }
            }
        }
        av_packet_unref(m_packet);

        if (frameFound || (ret < 0 && ret != AVERROR(EAGAIN))) {
            break;
        }
    }
}

bool AudioDecoder::decodeAudioFrame(std::vector<uint8_t>& outPcmData) {

    int ret = avcodec_send_packet(m_codecContext, m_packet);
    if (ret < 0) {
        return false;
    }

    while (ret >= 0) {
        ret = avcodec_receive_frame(m_codecContext, m_frame);
        if (ret == AVERROR(EAGAIN) || ret == AVERROR_EOF || ret < 0) {
            break;
        }

        //int64_t delay = swr_get_delay(m_swrContext, m_frame->sample_rate);
        //int64_t outSamples = av_rescale_rnd(delay + m_frame->nb_samples, m_codecContext->sample_rate, m_frame->sample_rate, AV_ROUND_UP);
        int outSamples = swr_get_out_samples(m_swrContext, m_frame->nb_samples);
        
        int maxFrameSize = outSamples * 2u * sizeof(int16_t);
        size_t oldSize = outPcmData.size();
        outPcmData.resize(oldSize + maxFrameSize);
        uint8_t* buffer = outPcmData.data() + oldSize;
      
        int convertedSamples = swr_convert(m_swrContext, &buffer, outSamples,(const uint8_t**)m_frame->data, m_frame->nb_samples);
        if (convertedSamples >= 0) {
            int actualFrameSize = convertedSamples * 2u * sizeof(int16_t);
            outPcmData.resize(oldSize + actualFrameSize);
        }else {
            outPcmData.resize(oldSize);
        }
       
        av_frame_unref(m_frame);
    }
    return true;
}

void AudioDecoder::close() {
    if (m_swrContext) {
        swr_free(&m_swrContext);
        m_swrContext = nullptr;
    }

    if (m_codecContext) {
        avcodec_free_context(&m_codecContext);
        m_codecContext = nullptr;
    }

    if (m_formatContext) {
        avformat_close_input(&m_formatContext);
        m_formatContext = nullptr;
    }

    if (m_data) {
        AssetIO::Free(m_data);
        m_data = nullptr;
    }

    m_audioStreamIndex = -1;
}

void AudioDecoder::pause() {
    m_isPaused = true;
    m_audioOutput->pause();
}

void AudioDecoder::play() {
    m_isPaused = false;
}