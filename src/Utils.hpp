#pragma once
extern "C" {
#include <libavformat/avformat.h>
}

#include "VideoInput.hpp"

class Utils {
  public:
    static void showProgress(double percent, double current, double total);
    static void copyAudio(VideoInput& input, AVFormatContext* outFormat, double maxTime);
};
