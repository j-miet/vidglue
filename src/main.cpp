#include <algorithm>
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
    {0, 0, 1560, 1080},
    {1560, 0, 360, 1080}};

// these will be used a lot, keep them outside settings struct
const int OUTPUT_W = 1920;
const int OUTPUT_H = 1080;
const int FPS = 60;

// other
const int SCALER_FLAGS = SWS_FAST_BILINEAR;
const double PREVIEW_SECONDS = 30.0; // 0 = full duration

// <1.0 = slow down, >1.0 = speed up. Doesn't affect output audio! Note that speed multiplier affects video length and
// therefore also preview time: if PREVIEW_SECONDS is 30.0 and speed 2.0, output is going to be 15 seconds
const double SPEED_MULTIPLIER = 2.0;

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

    std::vector<std::unique_ptr<VideoInput>> inputPointers; // input pointers
    std::vector<std::reference_wrapper<VideoInput>> inputs; // input references to also prevent moving
    std::vector<double> inputFPS;
    double maxDuration = 0.0;

    // open inputs, setup decoders and compute FPS + duration
    for (auto vid : INPUT_VIDEOS) {
        inputPointers.emplace_back(std::make_unique<VideoInput>(vid));

        auto& v = *inputPointers.back();
        inputs.push_back(v);

        AVStream* videoStream = v.getFormatContext()->streams[v.getVideoStreamIndex()];
        double fps = av_q2d(videoStream->avg_frame_rate);
        if (fps <= 0.0)
            fps = 30.0; // fallback
        inputFPS.push_back(fps);

        maxDuration = std::max(maxDuration, v.getDuration());
    }

    // threaded decoding init; wait for first frames
    for (auto& vref : inputs) {
        auto& v = vref.get(); // get reference from wrapper

        AVFrame* f = nullptr;
        while (!(f = v.getFrameBlocking())) {
        }
        av_frame_free(&f);
    }

    double displayMax = (PREVIEW_SECONDS > 0) ? std::min(maxDuration, PREVIEW_SECONDS) : maxDuration;
    double adjustedDuration = displayMax / SPEED_MULTIPLIER;

    // setup output streams and write header
    VideoOutput out(SETTINGS);

    int audioStream = inputs[0].get().getAudioStreamIndex();
    if (audioStream >= 0) {
        AVStream* inAudio = inputs[0].get().getFormatContext()->streams[audioStream];
        out.addAudioStream(inAudio);
    }

    out.writeHeader();

    VideoComposer composer(inputs, VIDEO_LAYOUT, out, OUTPUT_W, OUTPUT_H, FPS, inputFPS, SCALER_FLAGS);
    composer.process(adjustedDuration, SPEED_MULTIPLIER);

    // copy audio from the first input
    Utils::copyAudio(inputs[0].get(), out.getFormatContext(), adjustedDuration);

    Utils::showProgress(100.0, adjustedDuration, adjustedDuration);
    printf("\nFinishing...\n");

    out.finish();

    std::cout << "\nDone!\n";
    return 0;
}