#include <algorithm>
#include <execution>
#include <iostream>
#include <vector>

#include "Structs.hpp"
#include "Utils.hpp"
#include "VideoComposer.hpp"
#include "VideoInput.hpp"
#include "VideoOutput.hpp"

// TODO add CLI args + data reading from file -> move these inside main
const std::vector<const char*> INPUT_VIDEOS = {"vid1.mp4", "vid2.mp4"};
const std::vector<VideoLayout> VIDEO_LAYOUT = {
    {0, 0, 1580, 1080},
    {1580, 0, 340, 1080}};

// these will be used a lot, keep them outside settings struct
const int OUTPUT_W = 1920;
const int OUTPUT_H = 1080;
const int FPS = 60;

const double SPEED_MULTIPLIER = 1.0; // <1.0 = slow down, >1.0 = speed up. Doesn't affect output audio!
const double PREVIEW_SECONDS = 30.0; // 0 = full duration

// passed to VideoOutput constructor
const OutputSettings SETTINGS = {
    "output.mp4", // output file name
    OUTPUT_W,     // output width
    OUTPUT_H,     // output height
    FPS,          // output video fps
    true,         // use GPU for encoding; should use 'true' as this defaults to CPU if unsupported
    "p3",         // GPU preset
    23,           // GPU CQ
    "cqp",        // GPU RC
    "veryfast",   // CPU PRESET
    23,           // CPU CRF
    2             // Maximum B-Frames
};

int main() {
    av_log_set_level(AV_LOG_QUIET);

    std::vector<VideoInput> inputs;
    std::vector<double> inputFPS;
    double maxDuration = 0.0;

    // open inputs, setup decoders and compute FPS + duration
    for (auto f : INPUT_VIDEOS) {
        // here it's very important input pointer fields are unique (e.g. unique_ptr): they get copied by emplace_back
        // and original pointer gets nulled which in turn nulls the copy
        inputs.emplace_back(f);
        auto& v = inputs.back();

        AVStream* videoStream = v.getFormatContext()->streams[v.getVideoStreamIndex()];
        double fps = av_q2d(videoStream->avg_frame_rate);
        if (fps <= 0.0)
            fps = 30.0; // fallback
        inputFPS.push_back(fps);

        maxDuration = std::max(maxDuration, v.getDuration());

        v.decodeNextFrame(); // pre-decode first frame
    }

    double displayMax = (PREVIEW_SECONDS > 0) ? std::min(maxDuration, PREVIEW_SECONDS) : maxDuration;
    double adjustedDuration = displayMax / SPEED_MULTIPLIER;

    // setup output streams and write header
    VideoOutput out(SETTINGS);

    int audioStream = inputs[0].getAudioStreamIndex();
    if (audioStream >= 0) {
        AVStream* inAudio = inputs[0].getFormatContext()->streams[audioStream];
        out.addAudioStream(inAudio);
    }

    out.writeHeader();

    // raw output frame
    AVFrame* outFrame = av_frame_alloc();
    outFrame->format = AV_PIX_FMT_YUV420P;
    outFrame->width = OUTPUT_W;
    outFrame->height = OUTPUT_H;
    av_frame_get_buffer(outFrame, 0);

    VideoComposer composer(inputs, VIDEO_LAYOUT, out, OUTPUT_W, OUTPUT_H, FPS, inputFPS);
    composer.process(adjustedDuration, SPEED_MULTIPLIER);

    // copy audio from the first input
    Utils::copyAudio(inputs[0], out.getFormatContext(), displayMax);

    Utils::showProgress(100.0, adjustedDuration, adjustedDuration);
    printf("\nFinishing...\n");

    out.finish();
    av_frame_free(&outFrame);

    std::cout << "\nDone!\n";
    return 0;
}