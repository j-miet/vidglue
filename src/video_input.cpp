extern "C" {
#include <libavutil/hwcontext.h>
}

#include <chrono>
#include <iostream>
#include <stdexcept>

#include "utils.hpp"
#include "video_input.hpp"

using std::runtime_error;

AVPixelFormat VideoInput::get_hw_format(AVCodecContext* ctx, const AVPixelFormat* pix_fmts) {
    for (const enum AVPixelFormat* p = pix_fmts; *p != -1; p++) {
        if (*p == AV_PIX_FMT_CUDA)
            return *p;
    }

    std::cerr << "Warning: CUDA pixel format not offered, falling back\n";
    return pix_fmts[0];
}

/// @brief Initialize an input video + decoder
/// @param filename Path to video file
VideoInput::VideoInput(const std::string& filename) {
    AVFormatContext* fmt = nullptr;

    if (avformat_open_input(&fmt, filename.c_str(), nullptr, nullptr) < 0)
        throw runtime_error("Failed to open input file");

    m_format.reset(fmt);

    if (avformat_find_stream_info(m_format.get(), nullptr) < 0)
        throw runtime_error("Failed to find stream info");

    const AVCodec* decoder = nullptr;

    m_streamIndex = av_find_best_stream(m_format.get(), AVMEDIA_TYPE_VIDEO, -1, -1, &decoder, 0);

    if (m_streamIndex < 0)
        throw runtime_error("No video stream found");

    AVCodecContext* codec = avcodec_alloc_context3(decoder);
    if (!codec)
        throw runtime_error("Failed to allocate codec");

    m_decoder.reset(codec);
    m_decoder->thread_count = 0;                                // multithreading, use all cores
    m_decoder->thread_type = FF_THREAD_FRAME | FF_THREAD_SLICE; // let ffmpeg pick optimal model

    // Keep this disabled for now, GPU decoding is currently inefficient
    // checkDecoderHW();

    if (avcodec_parameters_to_context(m_decoder.get(), m_format->streams[m_streamIndex]->codecpar) < 0)
        throw runtime_error("Failed to copy codec params");

    if (avcodec_open2(m_decoder.get(), decoder, nullptr) < 0)
        throw runtime_error("Failed to open decoder");

    m_frame.reset(av_frame_alloc());
    m_tempFrame.reset(av_frame_alloc());

    if (!m_frame || !m_tempFrame)
        throw runtime_error("Failed to allocate frames");

    AVPacket* decodePacket = av_packet_alloc();
    m_packet.reset(decodePacket);
}

VideoInput::~VideoInput() {
    if (m_hwDeviceContext)
        av_buffer_unref(&m_hwDeviceContext);
}

/// @brief Decode next frame from input
/// @return Whether decoding was successful or not. This could be because of error or EOF.
bool VideoInput::decodeNextFrame() {
    if (m_eof)
        return false;

    while (true) {
        int ret = av_read_frame(m_format.get(), m_packet.get());
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

        if (m_packet.get()->stream_index != m_streamIndex) {
            av_packet_unref(m_packet.get());
            continue;
        }

        avcodec_send_packet(m_decoder.get(), m_packet.get());
        av_packet_unref(m_packet.get());

        ret = avcodec_receive_frame(m_decoder.get(), m_tempFrame.get());
        if (ret == AVERROR(EAGAIN))
            continue;
        if (ret < 0) {
            m_eof = true;

            return false;
        }

        av_frame_unref(m_frame.get());

        // GPU decoding is currently inefficient; don't call checkDecoderHW in constructor
        // Reason is that data transfer + moving it to frame can be a very slow operation on somewhat dated hardware
        // -- TODO should probably just add a separate config boolean flag which enables/disables this
        if (m_useHW && m_tempFrame->format == AV_PIX_FMT_CUDA) {
            if (av_hwframe_transfer_data(m_frame.get(), m_tempFrame.get(), 0) < 0) {
                throw runtime_error("Failed to transfer GPU frame to CPU");
            }
        } else {
            av_frame_move_ref(m_frame.get(), m_tempFrame.get());
        }

        m_decodedFrameIndex++;

        return true;
    }
}

/// @brief Get duration of input video
double VideoInput::getDuration() const {
    if (!m_format || m_streamIndex < 0)
        throw runtime_error("Invalid state");

    AVStream* stream = m_format->streams[m_streamIndex];
    return stream->duration * av_q2d(stream->time_base);
}

/// @brief Get index of first audio stream
int VideoInput::getAudioStreamIndex() const {
    return av_find_best_stream(m_format.get(), AVMEDIA_TYPE_AUDIO, -1, -1, nullptr, 0);
}

void VideoInput::m_checkDecoderHW() {
    // Try to enable CUDA decoding
    if (av_hwdevice_ctx_create(&m_hwDeviceContext, AV_HWDEVICE_TYPE_CUDA, nullptr, nullptr, 0) >= 0) {

        m_decoder->hw_device_ctx = av_buffer_ref(m_hwDeviceContext);
        m_decoder->get_format = get_hw_format; // set function pointer which ffmpeg calls internally
        m_useHW = true;

        std::cout << "Using GPU decoding (CUDA)\n";
    } else {
        std::cout << "GPU decoding not available, using CPU\n";
    }
}
