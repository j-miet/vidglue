#pragma once
extern "C" {
#include <libavcodec/avcodec.h>
#include <libavformat/avformat.h>
#include <libswscale/swscale.h>
}

#include <memory>

#include "Structs.hpp"

class VideoInput {
  public:
    explicit VideoInput(const std::string& filename);
    ~VideoInput(); // default constructor is used, but this overrides default move behavior i.e. removes it completely

    // copy constructor must be overridden to prevent copying unique_ptr and causing errors
    VideoInput(const VideoInput&) = delete;
    VideoInput& operator=(const VideoInput&) = delete;

    // move constructor must now be defined manually: copying is not allowed so default move behavior is desired
    VideoInput(VideoInput&&) = default;
    VideoInput& operator=(VideoInput&&) = default;

    AVFrame* getFrame() const { return m_frame.get(); }
    AVFormatContext* getFormatContext() const { return m_format.get(); }
    int getDecodedFrameIndex() const { return m_decodedFrameIndex; }

    double getDuration() const;
    int getVideoStreamIndex() const;
    int getAudioStreamIndex() const;

    bool decodeNextFrame();
    void checkDecoderHW();

  private:
    std::unique_ptr<AVFormatContext, FormatDeleter> m_format;
    std::unique_ptr<AVCodecContext, CodecDeleter> m_decoder;
    std::unique_ptr<AVFrame, FrameDeleter> m_frame;
    std::unique_ptr<AVFrame, FrameDeleter> m_tempFrame;

    int m_decodedFrameIndex{0};
    int m_streamIndex{-1};
    bool m_eof{false};

    AVBufferRef* m_hwDeviceContext{nullptr};
    bool m_useHW{false};

    static AVPixelFormat get_hw_format(AVCodecContext* ctx,
                                       const AVPixelFormat* pix_fmts);
};