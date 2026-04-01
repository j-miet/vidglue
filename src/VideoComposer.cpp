#include <assert.h>

#include "Utils.hpp"
#include "VideoComposer.hpp"

VideoComposer::VideoComposer(std::vector<VideoInput>& inputs,
                             const std::vector<VideoLayout>& layout,
                             VideoOutput& output,
                             int outW, int outH, int fps,
                             const std::vector<double>& inputFPS,
                             int flags)
    : m_inputs(inputs),
      m_layout(layout),
      m_output(output),
      m_outW(outW),
      m_outH(outH),
      m_fps(fps),
      m_inputFPS(inputFPS) {

    m_outFrame = av_frame_alloc();
    m_outFrame->format = AV_PIX_FMT_YUV420P;
    m_outFrame->width = outW;
    m_outFrame->height = outH;
    av_frame_get_buffer(m_outFrame, 0);

    // create scalers (one per input)
    for (size_t i = 0; i < inputs.size(); i++) {
        auto* f = inputs[i].getFrame();
        m_scalers.emplace_back(std::make_unique<Scaler>(
            f->width,
            f->height,
            (AVPixelFormat)f->format,
            layout[i].w,
            layout[i].h,
            flags));
    }

    assert(m_scalers.size() == m_inputs.size()); // confirm each input has a scaler
}

VideoComposer::~VideoComposer() {
    av_frame_free(&m_outFrame);
}

void VideoComposer::process(double duration, double speed) {
    int64_t totalOutputFrames = std::ceil(duration * m_fps);

    for (int64_t outIdx = 0; outIdx < totalOutputFrames; outIdx++) {
        double outTime = outIdx / double(m_fps); // output frame time
        double inTime = outTime * speed;         // map output to input time

        clearFrame();
        composeFrame(inTime);

        m_outFrame->pts = outIdx;
        m_output.writeFrame(m_outFrame);

        Utils::showProgress(outIdx / double(totalOutputFrames) * 100, outTime, duration);
    }
}

void VideoComposer::clearFrame() {
    memset(m_outFrame->data[0], 0, m_outFrame->linesize[0] * m_outH);
    memset(m_outFrame->data[1], 128, m_outFrame->linesize[1] * (m_outH / 2));
    memset(m_outFrame->data[2], 128, m_outFrame->linesize[2] * (m_outH / 2));
}

void VideoComposer::composeFrame(double inTime) {
    for (size_t i = 0; i < m_inputs.size(); i++) {
        auto& v = m_inputs[i];
        auto& l = m_layout[i];

        int targetFrameIndex = int(std::floor(inTime * m_inputFPS[i]));

        // decode frames until target is reached
        while (v.getDecodedFrameIndex() < targetFrameIndex && v.decodeNextFrame()) {
        }
    }

// OpenMP parallelization only after decoding
#pragma omp parallel for
    for (size_t i = 0; i < m_inputs.size(); i++) {
        auto scaled = m_scalers[i]->scale(m_inputs[i].getFrame());
        copyToOutput(scaled, m_layout[i]);
    }
}

void VideoComposer::copyToOutput(AVFrame* src, const VideoLayout& l) {
    // YUV format
    // Y plane
    for (int y = 0; y < l.h; y++)
        memcpy(m_outFrame->data[0] + (y + l.y) * m_outFrame->linesize[0] + l.x,
               src->data[0] + y * src->linesize[0],
               l.w);

    // U and V planes
    for (int y = 0; y < l.h / 2; y++) {
        memcpy(m_outFrame->data[1] + (y + l.y / 2) * m_outFrame->linesize[1] + l.x / 2,
               src->data[1] + y * src->linesize[1],
               l.w / 2);

        memcpy(m_outFrame->data[2] + (y + l.y / 2) * m_outFrame->linesize[2] + l.x / 2,
               src->data[2] + y * src->linesize[2],
               l.w / 2);
    }
}