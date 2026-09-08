#include <algorithm>
#include <iostream>

#include "VideoDecoder.h"
#include "AssetIO.h"

VideoDecoder::VideoDecoder() : m_audioOutput(nullptr){
    m_packet = av_packet_alloc();
    m_videoFrame = av_frame_alloc();
    m_audioFrame = av_frame_alloc();  
    //av_log_set_level(AV_LOG_DEBUG);
    av_log_set_level(AV_LOG_ERROR);
}

VideoDecoder::~VideoDecoder() {
    close();
    av_packet_free(&m_packet);
    av_frame_free(&m_videoFrame);
    av_frame_free(&m_audioFrame);
}

int VideoDecoder::Read_memory_packet(void* opaque, uint8_t* buf, int buf_size) {
    AVMemBuffer* bd = static_cast<AVMemBuffer*>(opaque);
    if (!bd || bd->size == 0) return AVERROR_EOF;

    int read_bytes = std::min(buf_size, static_cast<int>(bd->size));

    std::memcpy(buf, bd->ptr, read_bytes);
    bd->ptr += read_bytes;
    bd->size -= read_bytes;

    return read_bytes;
}

int64_t VideoDecoder::Seek_memory_packet(void* opaque, int64_t offset, int whence) {
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

void VideoDecoder::init(std::unique_ptr<IVideoDecoder> videoDecoder, std::unique_ptr<IAudioOutput> audioOutput){
    if (videoDecoder) {
        m_decoder = std::move(videoDecoder);
    }

    if (audioOutput) {
        m_audioOutput = std::move(audioOutput);
        m_audioOutput->init();
    }
}

void VideoDecoder::open(const std::string& filename, std::unique_ptr<IVideoDecoder> videoDecoder, std::unique_ptr<IAudioOutput> audioOutput) {
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
            &m_memBuffer, &Read_memory_packet, nullptr, &Seek_memory_packet
    );

    if (!avio_ctx) {
        av_freep(&avio_ctx_buffer);
        return;
    }

    m_formatContext->pb = avio_ctx;
    m_formatContext->flags |= AVFMT_FLAG_CUSTOM_IO;

    if (avformat_open_input(&m_formatContext, nullptr, nullptr, nullptr) < 0) return;
    if (avformat_find_stream_info(m_formatContext, nullptr) < 0) return;

    for (int i = 0; i < m_formatContext->nb_streams; i++) {
        if (m_formatContext->streams[i]->codecpar->codec_type == AVMEDIA_TYPE_VIDEO && m_videoStreamIndex == -1) {
            m_videoStreamIndex = i;
        } else if (m_formatContext->streams[i]->codecpar->codec_type == AVMEDIA_TYPE_AUDIO && m_audioStreamIndex == -1) {
            m_audioStreamIndex = i;
        }
    }

    if (m_videoStreamIndex == -1) return;

    const AVCodec* videoCodec = avcodec_find_decoder(m_formatContext->streams[m_videoStreamIndex]->codecpar->codec_id);
    m_videoCodecContext = avcodec_alloc_context3(videoCodec);
    avcodec_parameters_to_context(m_videoCodecContext, m_formatContext->streams[m_videoStreamIndex]->codecpar);

    init(std::move(videoDecoder), std::move(audioOutput));
    AVHWDeviceType avType = AV_HWDEVICE_TYPE_NONE;
    int err = av_hwdevice_ctx_create(&m_hwDeviceContext, avType, nullptr, nullptr, 0);

    avcodec_open2(m_videoCodecContext, videoCodec, nullptr);
    m_decoder->init(m_videoCodecContext->width, m_videoCodecContext->height);

    AVRational streamFps = m_formatContext->streams[m_videoStreamIndex]->r_frame_rate;
    m_fps = (streamFps.den > 0) ? av_q2d(streamFps) : 30.0f;
    m_timePerFrame = 1.0f / m_fps;

    if (m_audioStreamIndex != -1) {
        const AVCodec* audioCodec = avcodec_find_decoder(m_formatContext->streams[m_audioStreamIndex]->codecpar->codec_id);
        if (audioCodec) {
            m_audioCodecContext = avcodec_alloc_context3(audioCodec);
            avcodec_parameters_to_context(m_audioCodecContext, m_formatContext->streams[m_audioStreamIndex]->codecpar);
            if (avcodec_open2(m_audioCodecContext, audioCodec, nullptr) >= 0) {
                
                m_swrContext = swr_alloc();
                av_opt_set_chlayout(m_swrContext, "in_chlayout", &m_audioCodecContext->ch_layout, 0);
                av_opt_set_int(m_swrContext, "in_sample_rate", m_audioCodecContext->sample_rate, 0);
                av_opt_set_sample_fmt(m_swrContext, "in_sample_fmt", m_audioCodecContext->sample_fmt, 0);

                AVChannelLayout outLayout;
                av_channel_layout_default(&outLayout, 2);
                av_opt_set_chlayout(m_swrContext, "out_chlayout", &outLayout, 0);
                av_opt_set_int(m_swrContext, "out_sample_rate", 44100, 0);
                av_opt_set_sample_fmt(m_swrContext, "out_sample_fmt", AV_SAMPLE_FMT_FLT, 0);
                swr_init(m_swrContext);
            }
        }
    }
 
    if (m_formatContext->duration != AV_NOPTS_VALUE) {
        m_duration = static_cast<float>(m_formatContext->duration) / AV_TIME_BASE;
    }

    AVStream* videoStream = m_formatContext->streams[m_videoStreamIndex];
    m_videoTimebase = av_q2d(videoStream->time_base);
    m_isPaused = false;
    m_currentTime = 0.0f;
}

void VideoDecoder::queryFirstFrame() {
    for (;;) {
        if (av_read_frame(m_formatContext, m_packet) < 0) {
            continue;
        }

        if (m_packet->stream_index == m_videoStreamIndex) {
            if (decodeVideoFrame()) {
                return;
            }
        }
    }
}

void VideoDecoder::update(float deltaTime) {
    if (m_isPaused)
        return;
   
    m_accumulator += deltaTime;
    bool newFrameUploaded = false;

    while(m_accumulator >= m_timePerFrame && !newFrameUploaded) {
        int ret = av_read_frame(m_formatContext, m_packet);
        if(ret == AVERROR_EOF) {
            av_seek_frame(m_formatContext, -1, 0, AVSEEK_FLAG_BACKWARD);
            if (m_videoCodecContext) avcodec_flush_buffers(m_videoCodecContext);
            if (m_audioCodecContext) avcodec_flush_buffers(m_audioCodecContext);
            continue;
        }

        if(m_packet->stream_index == m_videoStreamIndex) {               
            if(decodeVideoFrame()) {
                newFrameUploaded = true;
                m_accumulator -= m_timePerFrame;
            }
        }else if(m_packet->stream_index == m_audioStreamIndex && m_swrContext) {
            std::vector<float> pcmData;
            if (decodeAudioFrame(pcmData)) {
                m_audioOutput->enqueueData(pcmData);
            }
        }
        av_packet_unref(m_packet);
    }

    if (m_accumulator > m_timePerFrame * 2.0) {
        m_accumulator = m_timePerFrame;
    }

    if(!m_wantSeek)
        m_decoder->beginMemoryAccess();
}

void VideoDecoder::OnPostDraw() {
    if (m_isPaused && m_wantSeek)
        return;

    m_decoder->endMemoryAccess();
    m_wantSeek = false;
}

bool VideoDecoder::decodeVideoFrame() {
    if(avcodec_send_packet(m_videoCodecContext, m_packet) < 0)
        return false;

    if(avcodec_receive_frame(m_videoCodecContext, m_videoFrame) < 0)
        return false;

    if(m_videoFrame->pts != AV_NOPTS_VALUE) {
        m_currentTime = m_videoFrame->pts * m_videoTimebase;
    }else if(m_videoFrame->pkt_dts != AV_NOPTS_VALUE) {
        m_currentTime = m_videoFrame->pkt_dts * m_videoTimebase;
    }
    
    m_decoder->updateTexture(m_videoFrame);
    av_frame_unref(m_videoFrame);
    av_packet_unref(m_packet);
    return true;    
}

bool VideoDecoder::decodeAudioFrame(std::vector<float>& outPcmData) {
    if (avcodec_send_packet(m_audioCodecContext, m_packet) < 0)
        return false;

    if (avcodec_receive_frame(m_audioCodecContext, m_audioFrame) < 0)
        return false;

    int outSamples = swr_get_out_samples(m_swrContext, m_audioFrame->nb_samples);

    outPcmData.resize(outSamples * 2);
    uint8_t* outputBuffer = reinterpret_cast<uint8_t*>(outPcmData.data());

    int translated = swr_convert(m_swrContext, &outputBuffer, outSamples,
                                 (const uint8_t**)m_audioFrame->data, m_audioFrame->nb_samples);

    if (translated < 0)
        return false;

    outPcmData.resize(translated * 2);

    av_frame_unref(m_audioFrame);
    return true;
}

void VideoDecoder::close() {
    if (m_swrContext) {
        swr_free(&m_swrContext); m_swrContext = nullptr; 
    }

    if (m_videoCodecContext) { 
        avcodec_free_context(&m_videoCodecContext); 
        m_videoCodecContext = nullptr; 
    }

    if (m_audioCodecContext) { 
        avcodec_free_context(&m_audioCodecContext); 
        m_audioCodecContext = nullptr;
    }

    if (m_formatContext) { 
        avformat_close_input(&m_formatContext); 
        m_formatContext = nullptr; 
    }

    if (m_data) {
        AssetIO::Free(m_data);
        m_data = nullptr;
    }
}

void VideoDecoder::seekTo(float seconds) {
    m_wantSeek = true;
    if (!m_formatContext || !m_videoCodecContext) return;

    m_currentTime = std::clamp(seconds, 0.0f, m_duration);
    int64_t targetVideoPts = static_cast<int64_t>(m_currentTime / m_videoTimebase);

    int seek_res = avformat_seek_file(
        m_formatContext,
        m_videoStreamIndex,
        INT64_MIN,
        targetVideoPts,
        targetVideoPts,
        AVSEEK_FLAG_BACKWARD
    );

    if (seek_res < 0) {
        av_seek_frame(m_formatContext, m_videoStreamIndex, targetVideoPts, AVSEEK_FLAG_BACKWARD);
    }

   
    avcodec_flush_buffers(m_videoCodecContext);
    if (m_audioCodecContext) 
        avcodec_flush_buffers(m_audioCodecContext);

    m_decoder->clearCache();

    m_videoCodecContext->skip_frame = AVDISCARD_NONREF;

    bool seekDone = false;
    AVFrame* finalFrame = av_frame_alloc();

    while (av_read_frame(m_formatContext, m_packet) >= 0) {
        if (m_packet->stream_index == m_videoStreamIndex) {

            if (avcodec_send_packet(m_videoCodecContext, m_packet) >= 0) {
                while (avcodec_receive_frame(m_videoCodecContext, m_videoFrame) >= 0) {

                    int64_t currentPts = (m_videoFrame->pts != AV_NOPTS_VALUE)
                        ? m_videoFrame->pts
                        : m_videoFrame->pkt_dts;

                    if (currentPts >= targetVideoPts) {
                        m_currentTime = currentPts * m_videoTimebase;
                        av_frame_move_ref(finalFrame, m_videoFrame);
                        seekDone = true;
                        break;
                    }

                    av_frame_unref(m_videoFrame);
                }
            }
        }
        av_packet_unref(m_packet);
        if (seekDone) break;
    }

    m_videoCodecContext->skip_frame = AVDISCARD_DEFAULT;
    if (!seekDone && m_videoFrame->format != -1) {
        av_frame_move_ref(finalFrame, m_videoFrame);
        seekDone = true;
    }

    if (seekDone && finalFrame->format != -1) {
       m_decoder->updateTexture(finalFrame);
    }

    av_frame_free(&finalFrame);
    m_accumulator = 0.0;
}

void VideoDecoder::setVolume(float volume) {
    m_audioOutput->setVolume(volume);
}

float VideoDecoder::getVolume() {
    return m_audioOutput->getVolume();
}

void VideoDecoder::pause() {
    m_isPaused = true;
    m_audioOutput->pause();
}

void VideoDecoder::play() {
    m_isPaused = false;
}