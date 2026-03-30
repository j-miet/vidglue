#pragma once

#include <string>

/// @brief Layout for a single input video
struct VideoLayout {
    int x{}, y{}, w{}, h{};
};

/// @brief
struct OutputSettings {
    const std::string FILENAME{"output.mp4"};
    const int OUTPUT_W{0};
    const int OUTPUT_H{0};

    const int FPS{30};
    const bool GPU{true};

    // GPU
    const char* GPU_PRESET{"p3"}; // p1-p7; p1 slow/best compression -> p7 fast/weak compression
    const int GPU_CQ{23};         // 0-51; default is 23. Lower value = higher quality, larger file size
    const char* GPU_RC{"cqp"};    // Values: cqp, vbr, cbr, vbr_hq -> bitrate = // quality but larger file size

    // CPU; similar to CPU
    const char* CPU_PRESET{"veryfast"}; // veryslow/slower/slow/medium/fast/faster/veryfast/superfast/ultrafast
    const int CPU_CRF{23};

    // B-frames
    const int MAX_B_FRAMES{2}; // 0-2 is default range for GPU, CPU often uses 3-4. Thus 2 is good default.
};