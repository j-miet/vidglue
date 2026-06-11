#pragma once
extern "C" {
#include <libavformat/avformat.h>
}

#include <vector>

#include "scaler.hpp"
#include "structs.hpp"
#include "videoinput.hpp"
#include "videooutput.hpp"

/// @brief Compose inputs into a single output
class VideoComposer {
  public:
    VideoComposer(std::vector<VideoInput>& inputs, const std::vector<VideoLayout>& layout, VideoOutput& output,
                  int outW, int outH, int fps, const std::vector<double>& inputFps, int flags, int progressTimeStamp);
    ~VideoComposer();

    void processGrid(double duration, double speed);
    void processSequential(double previewLimit, double speed, double pauseSeconds);

  private:
    void m_clearFrame();
    void m_composeFrame(double inTime);
    void m_copyToOutput(AVFrame* src, const VideoLayout& l);

    std::vector<VideoInput>& m_inputs;
    const std::vector<VideoLayout>& m_layout;
    VideoOutput& m_output;

    int m_outW, m_outH, m_fps;
    const std::vector<double>& m_inputFPS;
    int m_progressTT;

    AVFrame* m_outFrame{nullptr};
    std::vector<std::unique_ptr<Scaler>> m_scalers;
};