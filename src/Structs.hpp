#pragma once
extern "C" {
#include "libavcodec/avcodec.h"
#include "libavformat/avformat.h"
#include "libswscale/swscale.h"
}

#include <string>

/// @brief Layout for a single input video
struct VideoLayout {
    int x{}, y{}, w{}, h{};
};

/// @brief Settings passed to VideoOutput constructor
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