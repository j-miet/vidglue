#pragma once
extern "C" {
#include <libavformat/avformat.h>
}

#include <vector>

#include "Scaler.hpp"
#include "Structs.hpp"
#include "VideoInput.hpp"
#include "VideoOutput.hpp"

class VideoComposer {
  public:
    VideoComposer(std::vector<VideoInput>& inputs,
                  const std::vector<VideoLayout>& layout,
                  VideoOutput& output,
                  int outW, int outH, int fps,
                  const std::vector<double>& inputFps,
                  int flags,
                  bool hideProgress);
    ~VideoComposer();

    void processLayout(double duration, double speed);
    void processSequential(double previewLimit, double speed, double pauseSeconds);

  private:
    void clearFrame();
    void composeFrame(double inTime);
    void copyToOutput(AVFrame* src, const VideoLayout& l);

    std::vector<VideoInput>& m_inputs;
    const std::vector<VideoLayout>& m_layout;
    VideoOutput& m_output;

    int m_outW, m_outH, m_fps;
    const std::vector<double>& m_inputFPS;
    bool m_hideProgress;

    AVFrame* m_outFrame{nullptr};
    std::vector<std::unique_ptr<Scaler>> m_scalers;
};