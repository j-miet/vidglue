#pragma once
extern "C" {
#include <libavformat/avformat.h>
}

#include <vector>

#include "structs.hpp"
#include "videoinput.hpp"

class Utils {
  public:
    static bool readInputConfig(InputConfig& config);
    static void showProgress(double percent, double current, double total);
    static void copyAudio(VideoInput& input, AVFormatContext* outFormat, double maxTime);
    static void copyAudioSequential(std::vector<VideoInput>& inputs, AVFormatContext* outFormat,
                                    int outAudioStreamIndex, double previewLimit, double speed, double pauseSeconds);
};
