#pragma once
extern "C" {
#include "libavcodec/avcodec.h"
#include "libavformat/avformat.h"
#include "libswscale/swscale.h"
}

#include <string>
#include <vector>

enum class RenderMode {
    GRID,
    SEQUENTIAL
};

/// @brief Layout for a single input video
struct VideoLayout {
    int x{}, y{}, w{}, h{};
};

/// @brief Data read from input config file
struct InputConfig {
    RenderMode mode;
    std::vector<std::string> inputs;
    std::string output;
    std::vector<VideoLayout> layout;
    int outW;
    int outH;
    int fps;
    double previewDuration;
    double pauseDuration;
    int scalerFlags;
    double speedMultiplier;
    int progressTimestamp;
    bool useGpu;
    std::string gpuPreset;
    std::string gpuRc;
    int gpuCq;
    std::string cpuPreset;
    int cpuCrf;
    int bFrames;
};

/// @brief Settings passed to VideoOutput constructor
struct OutputSettings {
    const std::string FILENAME;
    const int OUTPUT_W;
    const int OUTPUT_H;
    const int FPS;
    const bool GPU;

    // GPU
    const std::string GPU_PRESET; // p1-p7; p1 slow/best compression -> p7 fast/weak compression
    const int GPU_CQ;             // 0-51; default is 23. Lower value = higher quality, larger file size
    const std::string GPU_RC;     // Values: cqp, vbr, cbr, vbr_hq -> bitrate = // quality but larger file size

    // CPU; similar to CPU
    const std::string CPU_PRESET; // veryslow/slower/slow/medium/fast/faster/veryfast/superfast/ultrafast
    const int CPU_CRF;

    // B-frames
    const int MAX_B_FRAMES; // 0-2 is default range for GPU, CPU often uses 3-4. Thus 2 is good default.
};

// deleters for unique_ptr
struct OutputFormatDeleter {
    void operator()(AVFormatContext* ctx) const {
        if (!ctx)
            return;

        if (ctx->pb)
            avio_closep(&ctx->pb);

        avformat_free_context(ctx);
    }
};

struct FormatDeleter {
    void operator()(AVFormatContext* ctx) const {
        if (ctx)
            avformat_close_input(&ctx);
    }
};

struct CodecDeleter {
    void operator()(AVCodecContext* ctx) const {
        if (ctx)
            avcodec_free_context(&ctx);
    }
};

struct FrameDeleter {
    void operator()(AVFrame* f) const {
        if (f)
            av_frame_free(&f);
    }
};

struct SwsDeleter {
    void operator()(SwsContext* s) const {
        if (s)
            sws_freeContext(s);
    }
};