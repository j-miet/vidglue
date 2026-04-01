#pragma once
extern "C" {
#include <libavformat/avformat.h>
}

#include <functional>
#include <vector>

#include "Scaler.hpp"
#include "Structs.hpp"
#include "VideoInput.hpp"
#include "VideoOutput.hpp"

class VideoComposer {
  public:
    VideoComposer(std::vector<std::reference_wrapper<VideoInput>> inputs,
                  const std::vector<VideoLayout>& layout,
                  VideoOutput& output,
                  int outW, int outH, int fps,
                  const std::vector<double>& inputFps,
                  int flags);
    ~VideoComposer();

    void process(double duration, double speed);

  private:
    void clearFrame();
    void composeFrame(double inTime);
    void copyToOutput(AVFrame* src, const VideoLayout& l);

    std::vector<std::reference_wrapper<VideoInput>> m_inputs;
    const std::vector<VideoLayout>& m_layout;
    VideoOutput& m_output;

    int m_outW, m_outH, m_fps;
    const std::vector<double>& m_inputFPS;

    AVFrame* m_outFrame{nullptr};
    std::vector<std::unique_ptr<Scaler>> m_scalers;
};