#pragma once
extern "C" {
#include <libavcodec/avcodec.h>
#include <libavformat/avformat.h>
#include <libswscale/swscale.h>
}

#include "Structs.hpp"

#include <memory>

class VideoInput {
  public:
    explicit VideoInput(const std::string& filename);

    bool decodeNextFrame();

    AVFrame* getFrame() const { return m_frame.get(); }

    double getDuration() const;
    int getVideoStreamIndex() const;
    int getAudioStreamIndex() const;

    AVFormatContext* getFormatContext() const { return m_format.get(); }
    int getDecodedFrameIndex() const { return m_decodedFrameIndex; }

  private:
    std::unique_ptr<AVFormatContext, FormatDeleter> m_format;
    std::unique_ptr<AVCodecContext, CodecDeleter> m_decoder;
    std::unique_ptr<AVFrame, FrameDeleter> m_frame;
    std::unique_ptr<AVFrame, FrameDeleter> m_tempFrame;
    std::unique_ptr<SwsContext, SwsDeleter> m_sws;

    int m_decodedFrameIndex{0};
    int m_streamIndex{1};
    bool m_eof{false};
};