#pragma once
extern "C" {
#include <libavcodec/avcodec.h>
#include <libavformat/avformat.h>
#include <libswscale/swscale.h>
}

#include <condition_variable>
#include <memory>
#include <mutex>
#include <queue>
#include <thread>

#include "Structs.hpp"

class VideoInput {
  public:
    explicit VideoInput(const std::string& filename);
    ~VideoInput(); // default constructor is used, but this also removes default copy/move behavior

    // disable copying; threading member objects are not allowed to be copied/moved around
    VideoInput(const VideoInput&) = delete;
    VideoInput& operator=(const VideoInput&) = delete;

    // disable moving
    VideoInput(VideoInput&&) = delete;
    VideoInput& operator=(VideoInput&&) = delete;

    bool decodeNextFrame();
    AVFrame* getFrameBlocking();
    double getDuration() const;
    int getVideoStreamIndex() const;
    int getAudioStreamIndex() const;

    AVFrame* getFrame() const { return m_frame.get(); }
    AVFormatContext* getFormatContext() const { return m_format.get(); }

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

    static AVPixelFormat get_hw_format(AVCodecContext* ctx,
                                       const AVPixelFormat* pix_fmts);

    // decoder threading
    std::thread m_decodeThread;
    std::queue<AVFrame*> m_frameQueue;

    std::mutex m_mutex;
    std::condition_variable m_cv;

    bool m_stop{false};
    bool m_finished{false};

    static constexpr size_t MAX_QUEUE_SIZE = 10;
};