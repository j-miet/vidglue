#include <iomanip>
#include <iostream>

#include "Utils.hpp"
#include "VideoInput.hpp"

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

    std::cout << "] "
              << std::fixed << std::setprecision(1) << percent << "% | "
              << std::fixed << std::setprecision(2) << current << "s / "
              << std::fixed << std::setprecision(2) << total << "s";

    std::cout.flush();
}

void Utils::copyAudio(VideoInput& input, AVFormatContext* outFormat, double maxTime) {
    int audioStreamIndex = input.getAudioStreamIndex();
    if (audioStreamIndex < 0) {
        std::cout << "No audio stream found, skipping audio\n";
        return;
    }

    AVFormatContext* inputFormat = input.getFormatContext();
    AVStream* inputAudio = inputFormat->streams[audioStreamIndex];

    // Use existing output audio stream
    AVStream* outputAudio = outFormat->streams[audioStreamIndex];

    av_seek_frame(inputFormat, audioStreamIndex, 0, AVSEEK_FLAG_BACKWARD);

    AVPacket pkt = {0};

    while (av_read_frame(inputFormat, &pkt) >= 0) {
        if (pkt.stream_index != audioStreamIndex) {
            av_packet_unref(&pkt);
            continue;
        }

        if (pkt.pts != AV_NOPTS_VALUE) {
            double pktTime = pkt.pts * av_q2d(inputAudio->time_base);
            if (pktTime > maxTime) {
                av_packet_unref(&pkt);
                break;
            }
        }

        pkt.stream_index = outputAudio->index;
        av_packet_rescale_ts(&pkt, inputAudio->time_base, outputAudio->time_base);

        int ret = av_interleaved_write_frame(outFormat, &pkt);
        if (ret < 0)
            std::cerr << "Warning: failed to write audio packet: " << ret << "\n";

        av_packet_unref(&pkt);
    }
}