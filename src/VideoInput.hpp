#pragma once
extern "C" {
#include <libavcodec/avcodec.h>
#include <libavformat/avformat.h>
#include <libswscale/swscale.h>
}

#include <memory>

// deleters for VideoInput's unique_ptr
struct InputFormatDeleter {
    void operator()(AVFormatContext* ctx) const {
        if (ctx)
            avformat_close_input(&ctx);
    }
};

struct InputDecoderDeleter {
    void operator()(AVCodecContext* ctx) const {
        if (ctx)
            avcodec_free_context(&ctx);
    }
};

struct InputFrameDeleter {
    void operator()(AVFrame* f) const {
        if (f)
            av_frame_free(&f);
    }
};

struct InputSwsDeleter {
    void operator()(SwsContext* s) const {
        if (s)
            sws_freeContext(s);
    }
};

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
    std::unique_ptr<AVFormatContext, InputFormatDeleter> m_format;
    std::unique_ptr<AVCodecContext, InputDecoderDeleter> m_decoder;
    std::unique_ptr<AVFrame, InputFrameDeleter> m_frame;
    std::unique_ptr<AVFrame, InputFrameDeleter> m_tempFrame;
    std::unique_ptr<SwsContext, InputSwsDeleter> m_sws;

    int m_decodedFrameIndex{0};
    int m_streamIndex{1};
    bool m_eof{false};
};