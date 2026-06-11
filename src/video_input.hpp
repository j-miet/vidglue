#pragma once
extern "C" {
#include <libavcodec/avcodec.h>
#include <libavformat/avformat.h>
#include <libswscale/swscale.h>
}

#include <memory>

#include "structs.hpp"

/// @brief Input video
class VideoInput {
  public:
    explicit VideoInput(const std::string& filename);
    ~VideoInput(); // default constructor is used. This however prevents auto-generation of move constructor

    // copy constructor gets generated, but for safety reasons prevent copying unique_ptr and causing errors
    VideoInput(const VideoInput&) = delete;
    VideoInput& operator=(const VideoInput&) = delete;
    // move constructor must be defined manually
    VideoInput(VideoInput&&) = default;
    VideoInput& operator=(VideoInput&&) = default;

    bool decodeNextFrame();

    AVFrame* getFrame() const { return m_frame.get(); }
    int getAudioStreamIndex() const;
    double getDuration() const;
    AVFormatContext* getFormatContext() const { return m_format.get(); }
    int getDecodedFrameIndex() const { return m_decodedFrameIndex; }
    int getVideoStreamIndex() const { return m_streamIndex; }

  private:
    void m_checkDecoderHW();

    std::unique_ptr<AVFormatContext, FormatDeleter> m_format;
    std::unique_ptr<AVCodecContext, CodecDeleter> m_decoder;
    std::unique_ptr<AVFrame, FrameDeleter> m_frame;
    std::unique_ptr<AVFrame, FrameDeleter> m_tempFrame;

    int m_decodedFrameIndex{0};
    int m_streamIndex{-1};
    bool m_eof{false};

    // decoder GPU hardware acceleration (unfinished)
    AVBufferRef* m_hwDeviceContext{nullptr};
    bool m_useHW{false};

    static AVPixelFormat get_hw_format(AVCodecContext* ctx, const AVPixelFormat* pix_fmts);
};