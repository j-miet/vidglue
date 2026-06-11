#pragma once
extern "C" {
#include <libavformat/avformat.h>
}

#include <vector>

#include "structs.hpp"
#include "video_input.hpp"

/// @brief Utility functions, handles also audio copying.
class Utils {
  public:
    static bool readInputConfig(char* path, InputConfig& config);
    static void showProgress(double percent, double current, double total);
    static void copyAudio(VideoInput& input, AVFormatContext* outFormat, double maxTime);
    static void copyAudioSequential(std::vector<VideoInput>& inputs, AVFormatContext* outFormat,
                                    int outAudioStreamIndex, double previewLimit, double speed, double pauseSeconds);
};