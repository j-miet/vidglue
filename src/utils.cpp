#include <algorithm>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <stdexcept>
#include <string>

#include "json.hpp" // https://github.com/nlohmann/json/blob/develop/single_include/nlohmann/json.hpp, is MIT licensed
#include "utils.hpp"
#include "video_input.hpp"

using json = nlohmann::json;

/// @brief Reads external config json file and fills InputConfig with data
/// @param config InputConfig to pass data
/// @return Whether loading inputs was successful or not
bool Utils::readInputConfig(char* path, InputConfig& config) {
    std::string configFile{path};

    std::ifstream file(configFile);

    if (!file) {
        std::cerr << "Config file " << configFile << " could not be opened\n";
        return false;
    }

    json j;
    file >> j;

    // setup config using JSON values:
    // rendering mode
    config.mode = (j.at("mode") == "sequence") ? RenderMode::SEQUENTIAL : RenderMode::GRID;

    // inputs and output
    std::vector<std::string> inputs{};
    for (auto input : j.at("inputs")) {
        inputs.push_back(input);
    }
    config.inputs = inputs;
    config.output = j.at("output");

    // layout
    std::vector<VideoLayout> vidLayouts{};
    for (auto l : j.at("layout")) {
        VideoLayout vLayout = {l[0], l[1], l[2], l[3]};
        vidLayouts.push_back(vLayout);
    }
    config.layout = vidLayouts;

    config.outW = j.at("outWidth");
    config.outH = j.at("outHeight");
    config.fps = j.at("fps");
    config.audioEnabled = j.at("audioEnabled");
    config.previewDuration = j.at("previewDuration");
    config.pauseDuration = j.at("pauseDuration");

    // scaling algorithm flags (primary, optional flags could be added later)
    int flags = j.at("scalerFlags");
    int selected = 1;
    switch (flags) {
        case 0:
            selected = SWS_FAST_BILINEAR;
        case 1:
            selected = SWS_BILINEAR;
        case 2:
            selected = SWS_BICUBIC;
        case 3:
            selected = SWS_LANCZOS;
        default:
            selected = SWS_BILINEAR;
    }
    config.scalerFlags = selected;

    config.speedMultiplier = j.at("speedMultiplier");
    config.progressTimestamp = j.at("progressTimestamps");
    config.useGpu = j.at("useGpu");
    config.gpuPreset = j.at("gpuPreset");
    config.gpuRc = j.at("gpuRc");
    config.gpuCq = j.at("gpuCq");
    config.cpuPreset = j.at("cpuPreset");
    config.cpuCrf = j.at("cpuCrf");
    config.bFrames = j.at("bFrames");

    file.close();
    return true;
}

/// @brief Display a progress message
/// @param percent Percentage of completion (0.0-100.0)
/// @param current Current time (out of total)
/// @param total Total time expectancy
void Utils::showProgress(double percent, double current, double total) {
    percent = std::min(100.0, std::round(percent * 10.0) / 10.0); // round to a single decimal

    std::cout << "\rProgress: [";
    int w = 30; // progress bar width
    int pos = int(percent / 100.0 * w);

    for (int i = 0; i < w; i++) {
        if (i < pos)
            std::cout << "=";
        else if (i == pos)
            std::cout << ">";
        else
            std::cout << " ";
    }

    std::cout << "] " << std::fixed << std::setprecision(1) << percent << "% | " << std::fixed << std::setprecision(2)
              << current << "s / " << std::fixed << std::setprecision(2) << total << "s";

    std::cout.flush();
}

/// @brief Copies audio from input video to output
/// @param input Video input
/// @param outFormat Output video format pointer
/// @param maxTime How much audio is copied in seconds. Should match video length.
void Utils::copyAudio(VideoInput& input, AVFormatContext* outFormat, double maxTime) {
    int audioStreamIndex = input.getAudioStreamIndex();
    if (audioStreamIndex < 0) {
        std::cout << "No audio stream found, skipping audio\n";
        return;
    }

    AVFormatContext* inputFormat = input.getFormatContext();
    AVStream* inputAudio = inputFormat->streams[audioStreamIndex];

    // use existing output audio stream
    AVStream* outputAudio = outFormat->streams[audioStreamIndex];

    av_seek_frame(inputFormat, audioStreamIndex, 0, AVSEEK_FLAG_BACKWARD);

    AVPacket* pkt{av_packet_alloc()};

    while (av_read_frame(inputFormat, pkt) >= 0) {
        if (pkt->stream_index != audioStreamIndex) {
            av_packet_unref(pkt);
            continue;
        }

        if (pkt->pts != AV_NOPTS_VALUE) {
            double pktTime = pkt->pts * av_q2d(inputAudio->time_base);
            if (pktTime > maxTime) {
                av_packet_unref(pkt);
                break;
            }
        }

        av_packet_rescale_ts(pkt, inputAudio->time_base, outputAudio->time_base);
        pkt->stream_index = outputAudio->index;

        int ret = av_interleaved_write_frame(outFormat, pkt);
        if (ret < 0)
            std::cerr << "Warning: failed to write audio packet: " << ret << "\n";

        av_packet_unref(pkt);
    }
}

/// @brief Copy audio from sequence of video inputs into one. Can insert phantom noise (= no audio) between videos.
/// @param inputs Videos to be sequenced in order
/// @param outFormat Output format
/// @param outAudioStreamIndex Output audio index (1 should always work here)
/// @param previewLimit How many seconds of each video gets rendered starting from the beginning
/// @param speed Video speed multiplier
/// @param pauseSeconds Seconds of black frames (transition) between videos
void Utils::copyAudioSequential(std::vector<VideoInput>& inputs, AVFormatContext* outFormat, int outAudioStreamIndex,
                                double previewLimit, double speed, double pauseSeconds) {
    int64_t ptsOffset = 0;

    AVStream* outputAudio = outFormat->streams[outAudioStreamIndex];

    for (size_t i = 0; i < inputs.size(); i++) {
        auto& input = inputs[i];

        int audioStreamIndex = input.getAudioStreamIndex();
        if (audioStreamIndex < 0) {
            std::cout << "No audio in input " << i << ", skipping\n";
            continue;
        }

        AVFormatContext* inputFormat = input.getFormatContext();
        AVStream* inputAudio = inputFormat->streams[audioStreamIndex];

        av_seek_frame(inputFormat, audioStreamIndex, 0, AVSEEK_FLAG_BACKWARD);

        AVPacket* pkt{av_packet_alloc()};

        // similar to video inputs, calculate duration by taking preview time into account
        double inputDuration = input.getDuration();
        double displayMax = (previewLimit > 0) ? std::min(inputDuration, previewLimit) : inputDuration;
        double adjustedDuration = displayMax / speed;

        // enforce monotonic increase for pts values because different videos could change spacing of frames
        int64_t firstPts = AV_NOPTS_VALUE;
        int64_t lastPts = ptsOffset;

        while (av_read_frame(inputFormat, pkt) >= 0) {
            if (pkt->stream_index != audioStreamIndex) {
                av_packet_unref(pkt);
                continue;
            }

            if (pkt->pts != AV_NOPTS_VALUE) {
                if (firstPts == AV_NOPTS_VALUE)
                    firstPts = pkt->pts;

                double pktTime = (pkt->pts - firstPts) * av_q2d(inputAudio->time_base);

                if (pktTime > adjustedDuration) {
                    av_packet_unref(pkt);
                    break;
                }
            }

            av_packet_rescale_ts(pkt, inputAudio->time_base, outputAudio->time_base);

            pkt->pts += ptsOffset;
            pkt->dts += ptsOffset;

            if (pkt->pts <= lastPts)
                pkt->pts = lastPts + 1;

            if (pkt->dts <= lastPts)
                pkt->dts = pkt->pts;

            lastPts = pkt->pts;

            pkt->stream_index = outAudioStreamIndex;

            if (av_interleaved_write_frame(outFormat, pkt) < 0)
                std::cerr << "Failed to write audio packet\n";

            av_packet_unref(pkt);
        }

        av_packet_free(&pkt);

        // duration in output time_base
        int64_t durationPts = adjustedDuration / av_q2d(outputAudio->time_base);
        ptsOffset += durationPts;

        // pause offset
        int64_t pausePts = pauseSeconds / av_q2d(outputAudio->time_base);
        ptsOffset += pausePts;
    }
}