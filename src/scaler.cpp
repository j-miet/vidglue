#include <stdexcept>

#include "scaler.hpp"

Scaler::Scaler(int srcW, int srcH, AVPixelFormat srcFmt, int dstW, int dstH, int flags) {
    m_context.reset(sws_getContext(
        srcW, srcH, srcFmt,
        dstW, dstH, AV_PIX_FMT_YUV420P,
        flags,
        nullptr, nullptr, nullptr));

    if (!m_context)
        throw std::runtime_error("Failed to create SwsContext");

    m_frame.reset(av_frame_alloc());
    m_frame->format = AV_PIX_FMT_YUV420P;
    m_frame->width = dstW;
    m_frame->height = dstH;

    if (av_frame_get_buffer(m_frame.get(), 0) < 0)
        throw std::runtime_error("Failed to allocate frame buffer");
}

AVFrame* Scaler::scale(const AVFrame* input) {
    sws_scale(m_context.get(),
              input->data, input->linesize, 0, input->height,
              m_frame->data, m_frame->linesize);

    return m_frame.get();
}