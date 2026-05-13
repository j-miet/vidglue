#include <iostream>
#include <stdexcept>

#include "utils.hpp"
#include "videooutput.hpp"

using std::runtime_error;

/// @brief Setup an output video + encoder
/// @param settings Settings, mostly for configuring encoder
VideoOutput::VideoOutput(const OutputSettings& settings) {
    bool usingGPU = false;
    const AVCodec* encoder = m_selectEncoder(settings.GPU, usingGPU);

    AVCodecContext* codec = avcodec_alloc_context3(encoder);
    if (!codec)
        throw runtime_error("Failed to allocate encoder");

    m_encoder.reset(codec);

    m_encoder->width = settings.OUTPUT_W;
    m_encoder->height = settings.OUTPUT_H;
    m_encoder->pix_fmt = AV_PIX_FMT_YUV420P;
    m_encoder->time_base = {1, settings.FPS};
    m_encoder->max_b_frames = settings.MAX_B_FRAMES;
    m_encoder->thread_count = 0;
    m_encoder->thread_type = FF_THREAD_FRAME | FF_THREAD_SLICE;

    if (usingGPU) {
        av_opt_set(m_encoder->priv_data, "preset", settings.GPU_PRESET.c_str(), 0);
        av_opt_set(m_encoder->priv_data, "rc", settings.GPU_RC.c_str(), 0);
        av_opt_set_int(m_encoder->priv_data, "cq", settings.GPU_CQ, 0);
    } else {
        av_opt_set(m_encoder->priv_data, "preset", settings.CPU_PRESET.c_str(), 0);
        av_opt_set_int(m_encoder->priv_data, "crf", settings.CPU_CRF, 0);
    }

    if (avcodec_open2(m_encoder.get(), encoder, nullptr) < 0)
        throw runtime_error("Failed to open encoder");

    AVFormatContext* fmt = nullptr;
    if (avformat_alloc_output_context2(&fmt, nullptr, nullptr, settings.FILENAME.c_str()) < 0)
        throw runtime_error("Failed to create output context");

    m_outFormat.reset(fmt);

    m_outVideo = avformat_new_stream(m_outFormat.get(), nullptr);
    if (!m_outVideo)
        throw runtime_error("Failed to create video stream");

    if (avcodec_parameters_from_context(m_outVideo->codecpar, m_encoder.get()) < 0)
        throw runtime_error("Failed to copy codec params");

    m_outVideo->time_base = m_encoder->time_base;

    if (avio_open(&m_outFormat->pb, settings.FILENAME.c_str(), AVIO_FLAG_WRITE) < 0)
        throw runtime_error("Failed to open output file");
}

VideoOutput::~VideoOutput() {
    finish();
}

/// @brief Add an audio stream to output video by copying it from input
/// @param inAudio Input audio
void VideoOutput::addAudioStream(AVStream* inAudio) {
    AVStream* outAudio = avformat_new_stream(m_outFormat.get(), nullptr);
    if (!outAudio)
        throw runtime_error("Failed to create audio stream");

    if (avcodec_parameters_copy(outAudio->codecpar, inAudio->codecpar) < 0)
        throw runtime_error("Failed to copy audio params");

    outAudio->time_base = inAudio->time_base;
}

/// @brief Write a output video header. Make sure you have attached both video and audio outputs before calling this.
void VideoOutput::writeHeader() {
    if (avformat_write_header(m_outFormat.get(), nullptr) < 0)
        throw runtime_error("Failed to write header");
}

/// @brief Encode a single frame and write package into output
/// @param frame Raw frame to be encoded
void VideoOutput::writeFrame(AVFrame* frame) {
    AVPacket* pkt{av_packet_alloc()};

    avcodec_send_frame(m_encoder.get(), frame);

    while (avcodec_receive_packet(m_encoder.get(), pkt) == 0) {
        av_packet_rescale_ts(pkt, m_encoder->time_base, m_outVideo->time_base);
        pkt->stream_index = m_outVideo->index;
        av_interleaved_write_frame(m_outFormat.get(), pkt);
        av_packet_unref(pkt);
    }
}

/// @brief Ensure all packets are written, write trailer and close output file
void VideoOutput::finish() {
    if (m_finished)
        return;

    AVPacket* pkt{av_packet_alloc()};
    avcodec_send_frame(m_encoder.get(), nullptr);

    while (avcodec_receive_packet(m_encoder.get(), pkt) == 0) {
        av_packet_rescale_ts(pkt, m_encoder->time_base, m_outVideo->time_base);
        av_interleaved_write_frame(m_outFormat.get(), pkt);
        av_packet_unref(pkt);
    }

    av_write_trailer(m_outFormat.get());

    if (m_outFormat->pb && !(m_outFormat->oformat->flags & AVFMT_NOFILE))
        avio_closep(&m_outFormat->pb);

    m_finished = true;
}

const AVCodec* VideoOutput::m_selectEncoder(bool gpuRequested, bool& usingGPU) {
    const AVCodec* encoder = nullptr;

    if (gpuRequested) {
        encoder = avcodec_find_encoder_by_name("h264_nvenc");
        if (encoder) {
            usingGPU = true;
            std::cout << "Using NVENC GPU encoder\n";
            return encoder;
        }
        std::cout << "GPU requested but not available, falling back to CPU\n";
    }

    usingGPU = false;

    encoder = avcodec_find_encoder_by_name("libx264");
    if (!encoder)
        encoder = avcodec_find_encoder(AV_CODEC_ID_H264);
    std::cout << "Using CPU encoder\n";
    return encoder;
}