#pragma once
extern "C" {
#include "libavformat/avformat.h"
#include "libswscale/swscale.h"
}

#include <memory>

// deleters for Scaler's unique_ptr
struct ScalerSwsDeleter {
    void operator()(SwsContext* s) const {
        if (s)
            sws_freeContext(s);
    }
};

struct ScalerFrameDeleter {
    void operator()(AVFrame* f) const {
        if (f)
            av_frame_free(&f);
    }
};

class Scaler {
  public:
    Scaler(int srcW, int srcH, AVPixelFormat srcFmt, int dstW, int dstH);
    // default destructor is fine

    AVFrame* scale(const AVFrame* input);

  private:
    std::unique_ptr<SwsContext, ScalerSwsDeleter> m_context;
    std::unique_ptr<AVFrame, ScalerFrameDeleter> m_frame;
};