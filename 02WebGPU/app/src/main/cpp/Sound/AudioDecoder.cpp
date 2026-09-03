#include "AudioDecoder.h"
#include "AssetIO.h"

uint8_t* data;
uint32_t size;

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

void AudioDecoder::open(const std::string& filename, std::unique_ptr<IAudioOutput> audioOutput) {

    AssetIO::LoadAsset(filename.c_str(), data, size);

    m_memBuffer.ptr = data;
    m_memBuffer.size = size;

    m_formatContext = avformat_alloc_context();
    if (!m_formatContext) return;

    // 1. ⚠️ UNBEDINGT NÖTIG: FFmpeg verlangt ein Padding am Ende des IO-Buffers!
    size_t avio_ctx_buffer_size = 4096;
    uint8_t* avio_ctx_buffer = static_cast<uint8_t*>(av_malloc(avio_ctx_buffer_size + AV_INPUT_BUFFER_PADDING_SIZE));

    if (!avio_ctx_buffer) {
        return;
    }

    // 2. Den Kontext erstellen
    AVIOContext* avio_ctx = avio_alloc_context(
            avio_ctx_buffer, avio_ctx_buffer_size, 0,
            &m_memBuffer, &read_memory_packet, nullptr, nullptr
    );

    // 3. ⚠️ PROTECTION: Wenn avio_ctx nullptr ist, dürfen wir es NIEMALS an pb übergeben!
    if (!avio_ctx) {
        av_freep(&avio_ctx_buffer);
        return;
    }

    // Erst wenn wir absolut sicher sind, dass avio_ctx gültig ist, zuweisen:
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

    if (audioOutput) {
        m_audioOutput = std::move(audioOutput);
        m_audioOutput->init();
    }
    queryFirstFrame();
}

void AudioDecoder::switchTrack(const std::string& filename) {
    close();
    m_audioOutput->flush();
    m_audioStreamIndex = -1;
    open(filename, nullptr);
}

void AudioDecoder::update() {
    if (!m_codecContext)
        return;

    int ret = av_read_frame(m_formatContext, m_packet);

    if(ret == AVERROR_EOF) {
        av_seek_frame(m_formatContext, -1, 0, AVSEEK_FLAG_BACKWARD);
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
    if (m_swrContext) 
        swr_free(&m_swrContext);

    if (m_codecContext) 
        avcodec_free_context(&m_codecContext);

    if (m_formatContext) 
        avformat_close_input(&m_formatContext);
}