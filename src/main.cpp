#include <algorithm>
#include <execution>
#include <iostream>
#include <vector>

#include "structs.hpp"
#include "utils.hpp"
#include "videocomposer.hpp"
#include "videoinput.hpp"
#include "videooutput.hpp"

// TODO add CLI args (config file is used with -config)
int main() {
    av_log_set_level(AV_LOG_QUIET);

    // load configuration from config.json
    InputConfig config{};
    if (!Utils::readInputConfig(config))
        return 1;

    // pass to VideoOutput constructor
    const OutputSettings SETTINGS = {
        config.output,
        config.outW,
        config.outH,
        config.fps,
        config.useGpu,
        config.gpuPreset,
        config.gpuCq,
        config.gpuRc,
        config.cpuPreset,
        config.cpuCrf,
        config.bFrames};

    std::vector<VideoInput> inputs;
    std::vector<double> inputFPS;
    double maxDuration = 0.0;

    // open inputs, setup decoders and compute FPS + duration
    for (auto f : config.inputs) {
        inputs.emplace_back(f);
        auto& v = inputs.back();

        AVStream* videoStream = v.getFormatContext()->streams[v.getVideoStreamIndex()];
        double fps = av_q2d(videoStream->avg_frame_rate);
        if (fps <= 0.0)
            fps = 30.0; // fallback
        inputFPS.push_back(fps);

        maxDuration = std::max(maxDuration, v.getDuration()); // get duration of longest input

        v.decodeNextFrame(); // pre-decode first frame
    }

    // setup output streams and write header
    VideoOutput out(SETTINGS);

    // because audio will be copied from first input without resampling, use it for stream creation
    int audioStream = inputs[0].getAudioStreamIndex();
    if (audioStream >= 0) {
        AVStream* inAudio = inputs[0].getFormatContext()->streams[audioStream];
        out.addAudioStream(inAudio);
    }

    out.writeHeader();

    if (config.mode == RenderMode::GRID) {
        double displayMax = (config.previewDuration > 0) ? std::min(maxDuration, config.previewDuration) : maxDuration;
        double adjustedDuration = displayMax / config.speedMultiplier;

        VideoComposer composer(inputs, config.layout, out, config.outW, config.outH, config.fps, inputFPS,
                               config.scalerFlags, config.progressTimestamp);
        composer.processGrid(adjustedDuration, config.speedMultiplier);
        Utils::showProgress(100.0, adjustedDuration, adjustedDuration);

        std::cout << "\nCopying audio...\n";
        Utils::copyAudio(inputs[0], out.getFormatContext(), adjustedDuration); // copy audio from the first input
    } else {
        VideoComposer composer(inputs, config.layout, out, config.outW, config.outH, config.fps, inputFPS,
                               config.scalerFlags, config.progressTimestamp);
        composer.processSequential(config.previewDuration, config.speedMultiplier, config.pauseDuration);

        // lazy audio copying + create silence during pauses by skipping timestamps
        std::cout << "\nCopying audio...\n";
        int outAudioIndex = 1; // video is usually in index 0, audio in 1
        Utils::copyAudioSequential(
            inputs,
            out.getFormatContext(),
            outAudioIndex,
            config.previewDuration,
            config.speedMultiplier,
            config.pauseDuration);
    }

    std::cout << "\nFinishing...\n";

    out.finish();

    std::cout << "\nDone!\n";
    return 0;
}