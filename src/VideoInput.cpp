#include <stdexcept>

#include "Utils.hpp"
#include "VideoInput.hpp"

VideoInput::VideoInput(const std::string& filename) {
    AVFormatContext* fmt = nullptr;

    if (avformat_open_input(&fmt, filename.c_str(), nullptr, nullptr) < 0)
        throw std::runtime_error("Failed to open input file");

    m_format.reset(fmt);

    if (avformat_find_stream_info(m_format.get(), nullptr) < 0)
        throw std::runtime_error("Failed to find stream info");

    const AVCodec* decoder = nullptr;

    m_streamIndex = av_find_best_stream(
        m_format.get(), AVMEDIA_TYPE_VIDEO, -1, -1, &decoder, 0);

    if (m_streamIndex < 0)
        throw std::runtime_error("No video stream found");

    AVCodecContext* codec = avcodec_alloc_context3(decoder);
    if (!codec)
        throw std::runtime_error("Failed to allocate codec");

    m_decoder.reset(codec);
    m_decoder->thread_count = 0; // multithreading, use all cores
    m_decoder->thread_type = FF_THREAD_FRAME;

    if (avcodec_parameters_to_context(
            m_decoder.get(),
            m_format->streams[m_streamIndex]->codecpar) < 0)
        throw std::runtime_error("Failed to copy codec params");

    if (avcodec_open2(m_decoder.get(), decoder, nullptr) < 0)
        throw std::runtime_error("Failed to open decoder");

    m_frame.reset(av_frame_alloc());
    m_tempFrame.reset(av_frame_alloc());

    if (!m_frame || !m_tempFrame)
        throw std::runtime_error("Failed to allocate frames");
}

bool VideoInput::decodeNextFrame() {
    if (m_eof)
        return false;

    AVPacket pkt = {0};

    while (true) {
        int ret = av_read_frame(m_format.get(), &pkt);
        if (ret < 0) {
            avcodec_send_packet(m_decoder.get(), nullptr);
            if (avcodec_receive_frame(m_decoder.get(), m_tempFrame.get()) == 0) {
                av_frame_unref(m_frame.get());
                av_frame_move_ref(m_frame.get(), m_tempFrame.get());
                m_decodedFrameIndex++;
                return true;
            }
            m_eof = true;
            return false;
        }

        if (pkt.stream_index != m_streamIndex) {
            av_packet_unref(&pkt);
            continue;
        }

        avcodec_send_packet(m_decoder.get(), &pkt);
        av_packet_unref(&pkt);

        ret = avcodec_receive_frame(m_decoder.get(), m_tempFrame.get());
        if (ret == AVERROR(EAGAIN))
            continue;
        if (ret < 0) {
            m_eof = true;
            return false;
        }

        av_frame_unref(m_frame.get());
        av_frame_move_ref(m_frame.get(), m_tempFrame.get());
        m_decodedFrameIndex++;
        return true;
    }
}

std::unique_ptr<AVFrame, FrameDeleter>
VideoInput::getScaledFrame(int width, int height) {
    if (!m_sws) {
        m_sws.reset(sws_getContext(
            m_frame->width,
            m_frame->height,
            static_cast<AVPixelFormat>(m_frame->format),
            width,
            height,
            AV_PIX_FMT_YUV420P,
            SWS_FAST_BILINEAR,
            nullptr,
            nullptr,
            nullptr));
    }

    AVFrame* raw = av_frame_alloc();
    if (!raw)
        throw std::runtime_error("Failed to allocate frame");

    std::unique_ptr<AVFrame, FrameDeleter> scaled(raw);

    scaled->format = AV_PIX_FMT_YUV420P;
    scaled->width = width;
    scaled->height = height;

    if (av_frame_get_buffer(scaled.get(), 0) < 0)
        throw std::runtime_error("Failed to allocate buffer");

    sws_scale(
        m_sws.get(),
        m_frame->data,
        m_frame->linesize,
        0,
        m_frame->height,
        scaled->data,
        scaled->linesize);

    return scaled;
}

double VideoInput::getDuration() const {
    if (!m_format || m_streamIndex < 0)
        throw std::runtime_error("Invalid state");

    AVStream* stream = m_format->streams[m_streamIndex];
    return stream->duration * av_q2d(stream->time_base);
}

int VideoInput::getVideoStreamIndex() const {
    return m_streamIndex;
}

int VideoInput::getAudioStreamIndex() const {
    return av_find_best_stream(m_format.get(), AVMEDIA_TYPE_AUDIO, -1, -1, nullptr, 0);
}