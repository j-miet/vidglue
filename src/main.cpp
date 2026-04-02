#include <algorithm>
#include <execution>
#include <iostream>
#include <vector>

#include "Structs.hpp"
#include "Utils.hpp"
#include "VideoComposer.hpp"
#include "VideoInput.hpp"
#include "VideoOutput.hpp"

enum class RenderMode {
    LAYOUT,
    SEQUENTIAL
};

const RenderMode MODE = RenderMode::SEQUENTIAL;

const int OUTPUT_W = 1920;
const int OUTPUT_H = 1080;
const int FPS = 60;

// order matters here:
// LAYOUT: audio comes only from the first input
// SEQUENTIAL: defines order of videos in output, left -> right = first -> last
const std::vector<const char*> INPUT_VIDEOS = {"vid1.mp4", "vid2.mp4"};

// only for LAYOUT: position and size of each video in output
const std::vector<VideoLayout> VIDEO_LAYOUT = {
    {0, 0, 1580, 1080},
    {1580, 0, 340, 1080}};

// only for SEQUENTIAL
const std::vector<VideoLayout> VIDEO_LAYOUT_SEQ = {
    {0, 0, OUTPUT_W, OUTPUT_H},
    {0, 0, OUTPUT_W / 2, OUTPUT_H}};
const double PAUSE_DURATION = 3.0; // only for SEQUENTIAL: black pause screen duration between videos

// other
const int SCALER_FLAGS = SWS_FAST_BILINEAR;
const double PREVIEW_SECONDS = 30.0; // 0 = full duration. With SEQUENTIAL renders *all videos* using this length
// <1.0 = slow down, >1.0 = speed up. Doesn't affect output audio! Note that speed multiplier affects video length and
// therefore also preview time: if PREVIEW_SECONDS is 30.0 and speed 2.0, output is going to be 15 seconds
const double SPEED_MULTIPLIER = 1.0;
// set to true to disable progress prints; increases output render speed because these run on main thread!
const bool HIDE_PROGRESS = false; // TEST THIS ON true: base speed with GPU ~3x, this increases it about 67% -> ~5x

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

    /*
    if (MODE == RenderMode::LAYOUT) {
        std::cout << "Using layout mode ";
    } else {
        std::cout << "Using sequential mode ";
    }
    std::cout << "with following settings -> \n"
              << "Files: \n\n";
    */

    std::vector<VideoInput> inputs;
    std::vector<double> inputFPS;
    double maxDuration = 0.0;

    // open inputs, setup decoders and compute FPS + duration
    for (auto f : INPUT_VIDEOS) {
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

    // use first audio stream for stream creation
    int audioStream = inputs[0].getAudioStreamIndex();
    if (audioStream >= 0) {
        AVStream* inAudio = inputs[0].getFormatContext()->streams[audioStream];
        out.addAudioStream(inAudio);
    }

    out.writeHeader();

    if (MODE == RenderMode::LAYOUT) {
        VideoComposer composer(inputs, VIDEO_LAYOUT, out, OUTPUT_W, OUTPUT_H, FPS, inputFPS, SCALER_FLAGS,
                               HIDE_PROGRESS);
        composer.processLayout(adjustedDuration, SPEED_MULTIPLIER);
        Utils::copyAudio(inputs[0], out.getFormatContext(), adjustedDuration); // copy audio from the first input
        Utils::showProgress(100.0, adjustedDuration, adjustedDuration, HIDE_PROGRESS);
    } else {
        VideoComposer composer(inputs, VIDEO_LAYOUT_SEQ, out, OUTPUT_W, OUTPUT_H, FPS, inputFPS, SCALER_FLAGS,
                               HIDE_PROGRESS);
        composer.processSequential(PREVIEW_SECONDS, SPEED_MULTIPLIER, PAUSE_DURATION);

        // lazy audio copying + create silence during pauses by skipping timestamps
        std::cout << "\nCopying audio...\n";
        int outAudioIndex = 1; // usually video=0, audio=1
        Utils::copyAudioSequential(
            inputs,
            out.getFormatContext(),
            outAudioIndex,
            adjustedDuration,
            SPEED_MULTIPLIER,
            PAUSE_DURATION);
    }

    std::cout << "\nFinishing...\n";

    out.finish();

    std::cout << "\nDone!\n";
    return 0;
}