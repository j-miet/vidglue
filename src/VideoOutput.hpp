#pragma once
extern "C" {
#include <libavcodec/avcodec.h>
#include <libavformat/avformat.h>
#include <libavutil/opt.h>
}

#include <memory>

#include "Structs.hpp"

// deleters for VideoOutput's unique_ptr
struct OutputFormatDeleter {
    void operator()(AVFormatContext* ctx) const {
        if (!ctx)
            return;

        if (ctx->pb)
            avio_closep(&ctx->pb);

        avformat_free_context(ctx);
    }
};

struct EncoderDeleter {
    void operator()(AVCodecContext* ctx) const {
        if (ctx)
            avcodec_free_context(&ctx);
    }
};

class VideoOutput {
  public:
    VideoOutput(const OutputSettings& settings);
    ~VideoOutput();

    void addAudioStream(AVStream* inAudio);
    void writeHeader();
    void writeFrame(AVFrame* frame);
    void finish();

    AVFormatContext* getFormatContext() { return m_outFormat.get(); }
    AVStream* getVideoStream() { return m_outVideo; }

  private:
    const AVCodec* selectEncoder(bool gpuRequested, bool& usingGPU);

    std::unique_ptr<AVFormatContext, OutputFormatDeleter> m_outFormat;
    std::unique_ptr<AVCodecContext, EncoderDeleter> m_encoder;

    AVStream* m_outVideo{nullptr};
    bool m_finished{false};
};