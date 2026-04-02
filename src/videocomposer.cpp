#include <assert.h>
#include <iostream>

#include "Utils.hpp"
#include "VideoComposer.hpp"

VideoComposer::VideoComposer(std::vector<VideoInput>& inputs,
                             const std::vector<VideoLayout>& layout,
                             VideoOutput& output,
                             int outW, int outH, int fps,
                             const std::vector<double>& inputFPS,
                             int flags,
                             bool hideProgres)
    : m_inputs(inputs),
      m_layout(layout),
      m_output(output),
      m_outW(outW),
      m_outH(outH),
      m_fps(fps),
      m_inputFPS(inputFPS),
      m_hideProgress(hideProgres) {

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

void VideoComposer::processLayout(double duration, double speed) {
    int64_t totalOutputFrames = std::ceil(duration * m_fps);

    for (int64_t outIdx = 0; outIdx < totalOutputFrames; outIdx++) {
        double outTime = outIdx / double(m_fps); // output frame time
        double inTime = outTime * speed;         // map output to input time

        clearFrame();
        composeFrame(inTime);

        m_outFrame->pts = outIdx;
        m_output.writeFrame(m_outFrame);

        Utils::showProgress(outIdx / double(totalOutputFrames) * 100, outTime, duration, m_hideProgress);
    }
}

void VideoComposer::processSequential(double previewLimit, double speed, double pauseSeconds) {
    int64_t outFrameIndex = 0;
    int pauseFrames = int(pauseSeconds * m_fps);

    for (size_t i = 0; i < m_inputs.size(); i++) {
        std::cout << "Composing video " << i << ":\n";
        auto& input = m_inputs[i];
        auto& layout = m_layout[i];
        auto& scaler = m_scalers[i];

        double duration = input.getDuration();
        // in sequential outputs, preview is applied to each input
        // E.g. 2x videos -> previewLimit = 5.0 -> render first 5 secs of both -> output is 10 sec + pauses
        double displayMax = (previewLimit > 0) ? std::min(duration, previewLimit) : duration;
        double adjustedDuration = displayMax / speed;

        int64_t totalFrames = int64_t(std::ceil(adjustedDuration * m_fps));

        for (int64_t f = 0; f < totalFrames; f++) {
            double outTime = f / double(m_fps);
            double inTime = outTime * speed;

            int targetFrame = int(std::floor(inTime * m_inputFPS[i]));

            // decode frames until target is reached
            while (input.getDecodedFrameIndex() < targetFrame && input.decodeNextFrame()) {
            }

            clearFrame();

            AVFrame* scaled = scaler->scale(input.getFrame());
            copyToOutput(scaled, layout);

            m_outFrame->pts = outFrameIndex++;
            m_output.writeFrame(m_outFrame);
            Utils::showProgress(f / double(totalFrames) * 100, outTime, displayMax, m_hideProgress);
        }

        if (i < m_inputs.size() - 1) { // don't add pause frames at the end of final video
            clearFrame();
            for (int p = 0; p < pauseFrames; p++) {
                m_outFrame->pts = outFrameIndex++;
                m_output.writeFrame(m_outFrame);
            }
        }
        Utils::showProgress(100.0, displayMax, displayMax, m_hideProgress);
        std::cout << "\n";
    }
}

void VideoComposer::clearFrame() {
    memset(m_outFrame->data[0], 0, m_outFrame->linesize[0] * m_outH);
    memset(m_outFrame->data[1], 128, m_outFrame->linesize[1] * (m_outH / 2));
    memset(m_outFrame->data[2], 128, m_outFrame->linesize[2] * (m_outH / 2));
}

// for layouts only
void VideoComposer::composeFrame(double inTime) {
    for (size_t i = 0; i < m_inputs.size(); i++) {
        auto& input = m_inputs[i];

        int targetFrame = int(std::floor(inTime * m_inputFPS[i]));

        while (input.getDecodedFrameIndex() < targetFrame && input.decodeNextFrame()) {
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