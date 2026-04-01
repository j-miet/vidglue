#pragma once
extern "C" {
#include <libavcodec/avcodec.h>
#include <libavformat/avformat.h>
#include <libavutil/opt.h>
}

#include <memory>

#include "Structs.hpp"

class VideoOutput {
  public:
    VideoOutput(const OutputSettings& settings);
    ~VideoOutput();

    void addAudioStream(AVStream* inAudio);
    void writeHeader();
    void writeFrame(AVFrame* frame);
    void finish();

    AVFormatContext* getFormatContext() const { return m_outFormat.get(); }
    AVStream* getVideoStream() const { return m_outVideo; }

  private:
    const AVCodec* m_selectEncoder(bool gpuRequested, bool& usingGPU);

    std::unique_ptr<AVFormatContext, OutputFormatDeleter> m_outFormat;
    std::unique_ptr<AVCodecContext, CodecDeleter> m_encoder;

    AVStream* m_outVideo{nullptr};
    bool m_finished{false};
};