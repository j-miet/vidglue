#pragma once
extern "C" {
#include "libavformat/avformat.h"
#include "libswscale/swscale.h"
}

#include <memory>

#include "structs.hpp"

/// @brief Input video resizing
class Scaler {
  public:
    Scaler(int srcW, int srcH, AVPixelFormat srcFmt, int dstW, int dstH, int flags);

    AVFrame* scale(const AVFrame* input);

  private:
    std::unique_ptr<SwsContext, SwsDeleter> m_context;
    std::unique_ptr<AVFrame, FrameDeleter> m_frame;
};
