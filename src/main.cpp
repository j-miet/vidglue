// vidglue source code, finally working as expected
// Requires C++ structuring with files, classes and other language-specific improvements
// Also requires proper freeing of allocated ffmpeg data structure pointers with their corresponding functions;
// these are easier to handle inside classes

extern "C" {
#include <libavcodec/avcodec.h>
#include <libavformat/avformat.h>
#include <libavutil/opt.h>
#include <libswscale/swscale.h>
}

#include <string>
#include <vector>

// layout
struct Layout {
    int x, y, w, h;
};

// video input
struct Input {
    AVFormatContext* fmt = nullptr;
    AVCodecContext* dec = nullptr;
    int stream = -1;
    AVFrame* frame = nullptr;
    bool eof = false;
    SwsContext* sws = nullptr;
};

// output
std::vector<const char*> INPUT_VIDEOS = {"vod.mp4", "chat.mp4"};
std::vector<Layout> VIDEO_LAYOUT = {{0, 0, 1580, 1080}, {1580, 0, 340, 1080}};
const char* OUTPUT_FILENAME = "output.mp4";
const int OUTPUT_W = 1920;
const int OUTPUT_H = 1080;
const int OUTPUT_FPS = 60;
const bool GPU = false;             // gpu = encoding/decoding uses gpu; larger file size when set to true
const double PREVIEW_SECONDS = 0.0; // set 0 for full render, > 0 for any custom length

// GPU
const char* GPU_PRESET = "p3"; // p1-p7; p1 slow/best compression -> p7 fast/weak compression
const int GPU_CQ = 23;         // 0-51; default is 23. Lower value = higher quality, larger file size
const char* GPU_RC = "cqp";    // Values: cqp, vbr, cbr, vbr_hq -> bitrate = // quality but larger file size

// CPU; see GPU for how these work
const char* CPU_PRESET = "veryfast"; // veryslow/slower/slow/medium/fast/faster/veryfast/superfast/ultrafast
const int CPU_CRF = 23;

// B-frames
const int MAX_B_FRAMES = 2; // 0-2 is default range for GPU, CPU often uses 3-4. Thus 2 is good default.

/// @brief Prints current progress of codec process
/// @param pct Raw completion percentage value (0.0-1.0)
/// @param cur Current time
/// @param total Total time (= output video length)
void print_progress(double pct, double cur, double total) {
    pct = std::min(100.0, std::round(pct * 10.0) / 10.0); // round to a single decimal

    printf("\rProgress: [");
    int w = 30;
    int pos = int(pct / 100.0 * w);

    for (int i = 0; i < w; i++)
        printf(i < pos ? "=" : i == pos ? ">"
                                        : " ");

    printf("] %.1f%% | %.2fs / %.2fs", pct, cur, total);
    fflush(stdout);
}

/// @brief Decodes a input packet into a raw frame.
/// @param in Video input
/// @param tmp Temporary frame allocated with av_frame_alloc()
/// @return If frame was allocated successfully, return true. If decoding error or EOF, return false
bool decode_next(Input& in, AVFrame* tmp) {
    if (in.eof)
        return false;

    AVPacket pkt{};

    while (true) {
        int ret = av_read_frame(in.fmt, &pkt);

        if (ret < 0) {
            avcodec_send_packet(in.dec, nullptr);
            if (avcodec_receive_frame(in.dec, tmp) == 0) {
                av_frame_unref(in.frame);
                av_frame_move_ref(in.frame, tmp);
                return true;
            }
            in.eof = true;
            return false;
        }

        if (pkt.stream_index != in.stream) {
            av_packet_unref(&pkt);
            continue;
        }

        avcodec_send_packet(in.dec, &pkt);
        av_packet_unref(&pkt);

        ret = avcodec_receive_frame(in.dec, tmp);
        if (ret == AVERROR(EAGAIN))
            continue;
        if (ret < 0) {
            in.eof = true;
            return false;
        }

        av_frame_unref(in.frame);
        av_frame_move_ref(in.frame, tmp);
        return true;
    }
}

int main() {
    av_log_set_level(AV_LOG_QUIET);

    std::vector<const char*> files = INPUT_VIDEOS;
    std::vector<Layout> layout = VIDEO_LAYOUT;

    int N = files.size();
    std::vector<Input> in(N);

    bool use_gpu = GPU;
    std::string gpu_type = "none";

    auto choose_encoder = [&](bool use_gpu_requested, std::string& gpu_type) {
        const AVCodec* enc = nullptr;

        if (use_gpu_requested) {
            enc = avcodec_find_encoder_by_name("h264_nvenc"); // try GPU encoder
            if (enc) {
                gpu_type = "nvenc";
                printf("Using NVENC GPU encoder\n");
                return enc;
            }
            printf("GPU requested but not found, falling back to CPU\n"); // fallback if GPU not available
        }

        gpu_type = "cpu";
        printf("Using CPU encoder\n");
        return avcodec_find_encoder(AV_CODEC_ID_H264);
    };

    const AVCodec* enc = choose_encoder(use_gpu, gpu_type);
    if (gpu_type != "nvenc")
        use_gpu = false;

    // open file and setup decoded
    for (int i = 0; i < N; i++) {
        avformat_open_input(&in[i].fmt, files[i], nullptr, nullptr);
        avformat_find_stream_info(in[i].fmt, nullptr);

        const AVCodec* dec;
        in[i].stream = av_find_best_stream(in[i].fmt, AVMEDIA_TYPE_VIDEO, -1, -1, &dec, 0);

        in[i].dec = avcodec_alloc_context3(dec);
        avcodec_parameters_to_context(in[i].dec, in[i].fmt->streams[in[i].stream]->codecpar);

        avcodec_open2(in[i].dec, dec, nullptr);
        in[i].frame = av_frame_alloc();
    }

    double max_duration = 0;
    for (int i = 0; i < N; i++) {
        AVStream* st = in[i].fmt->streams[in[i].stream];
        double d = st->duration * av_q2d(st->time_base);
        max_duration = std::max(max_duration, d);
    }

    double display_max = (PREVIEW_SECONDS > 0) ? std::min(max_duration, PREVIEW_SECONDS) : max_duration;

    int audio_stream_index = av_find_best_stream(in[0].fmt, AVMEDIA_TYPE_AUDIO, -1, -1, nullptr, 0);

    // allocate raw frames
    AVFrame* tmp = av_frame_alloc();
    std::vector<AVFrame*> scaled(N);
    for (int i = 0; i < N; i++) {
        scaled[i] = av_frame_alloc();
        scaled[i]->format = AV_PIX_FMT_YUV420P;
        scaled[i]->width = layout[i].w;
        scaled[i]->height = layout[i].h;
        av_frame_get_buffer(scaled[i], 32);
    }

    // setup encoder
    AVCodecContext* encCtx = avcodec_alloc_context3(enc);
    encCtx->width = OUTPUT_W;
    encCtx->height = OUTPUT_H;
    encCtx->pix_fmt = AV_PIX_FMT_YUV420P;
    encCtx->time_base = {1, OUTPUT_FPS};
    encCtx->max_b_frames = MAX_B_FRAMES;

    if (use_gpu) {
        av_opt_set(encCtx->priv_data, "preset", GPU_PRESET, 0);
        av_opt_set(encCtx->priv_data, "rc", GPU_RC, 0);
        av_opt_set_int(encCtx->priv_data, "cq", GPU_CQ, 0);
    } else {
        av_opt_set(encCtx->priv_data, "preset", CPU_PRESET, 0);
        av_opt_set_int(encCtx->priv_data, "crf", CPU_CRF, 0);
    }

    avcodec_open2(encCtx, enc, nullptr);

    // setup output
    AVFormatContext* outFmt;
    avformat_alloc_output_context2(&outFmt, nullptr, nullptr, OUTPUT_FILENAME);

    AVStream* outVideo = avformat_new_stream(outFmt, nullptr);
    avcodec_parameters_from_context(outVideo->codecpar, encCtx);
    outVideo->time_base = encCtx->time_base;

    AVStream* outAudio = nullptr;

    if (audio_stream_index >= 0) {
        AVStream* inAudio = in[0].fmt->streams[audio_stream_index];
        outAudio = avformat_new_stream(outFmt, nullptr);
        avcodec_parameters_copy(outAudio->codecpar, inAudio->codecpar);
        outAudio->time_base = inAudio->time_base;
    }

    if (avio_open(&outFmt->pb, OUTPUT_FILENAME, AVIO_FLAG_WRITE) < 0) {
        fprintf(stderr, "Failed to open output\n");
        return -1;
    }
    if (avformat_write_header(outFmt, nullptr) < 0) {
        fprintf(stderr, "Header write error\n");
        return -1;
    }

    AVFrame* outFrame = av_frame_alloc();
    outFrame->format = AV_PIX_FMT_YUV420P;
    outFrame->width = OUTPUT_W;
    outFrame->height = OUTPUT_H;
    av_frame_get_buffer(outFrame, 32);

    // --codec and writing output--

    // decode first frame in advance; see comment below
    for (int i = 0; i < N; i++)
        decode_next(in[i], tmp);

    double current_time = 0.0;
    int64_t frame_index = 0;

    // main loop: sws (resize, repositioning) -> encode -> decode next frame for the next iteration
    // this is just a convention with ffmpeg to ensure all inputs hold a frame before entering the processing loop
    // it's also recommended for more complex pipelines for initial error dependency checks, but for current state of
    // this program, there would be no difference if decode_next was moved from the end of while loop to start
    while (current_time < display_max) {
        for (int y = 0; y < OUTPUT_H; y++)
            memset(outFrame->data[0] + y * outFrame->linesize[0], 0, OUTPUT_W);

        for (int y = 0; y < OUTPUT_H / 2; y++) {
            memset(outFrame->data[1] + y * outFrame->linesize[1], 128, OUTPUT_W / 2);
            memset(outFrame->data[2] + y * outFrame->linesize[2], 128, OUTPUT_W / 2);
        }

        for (int i = 0; i < N; i++) {
            if (!in[i].sws) {
                in[i].sws = sws_getContext(in[i].frame->width, in[i].frame->height, (AVPixelFormat)in[i].frame->format,
                                           layout[i].w, layout[i].h, AV_PIX_FMT_YUV420P, SWS_FAST_BILINEAR,
                                           nullptr, nullptr, nullptr);
            }

            sws_scale(in[i].sws,
                      in[i].frame->data, in[i].frame->linesize, 0, in[i].frame->height,
                      scaled[i]->data, scaled[i]->linesize);

            auto& r = layout[i];

            for (int y = 0; y < r.h; y++) {
                memcpy(outFrame->data[0] + (y + r.y) * outFrame->linesize[0] + r.x,
                       scaled[i]->data[0] + y * scaled[i]->linesize[0],
                       r.w);
            }

            for (int y = 0; y < r.h / 2; y++) {
                memcpy(outFrame->data[1] + (y + r.y / 2) * outFrame->linesize[1] + r.x / 2,
                       scaled[i]->data[1] + y * scaled[i]->linesize[1],
                       r.w / 2);

                memcpy(outFrame->data[2] + (y + r.y / 2) * outFrame->linesize[2] + r.x / 2,
                       scaled[i]->data[2] + y * scaled[i]->linesize[2],
                       r.w / 2);
            }
        }

        outFrame->pts = frame_index++;

        // encode frame into packet
        AVPacket pkt{};
        avcodec_send_frame(encCtx, outFrame);

        while (avcodec_receive_packet(encCtx, &pkt) == 0) {
            av_packet_rescale_ts(&pkt, encCtx->time_base, outVideo->time_base);
            pkt.stream_index = outVideo->index;
            av_interleaved_write_frame(outFmt, &pkt);
            av_packet_unref(&pkt);
        }

        // decode next frame
        for (int i = 0; i < N; i++)
            decode_next(in[i], tmp);

        current_time += 1.0 / OUTPUT_FPS;
        print_progress(current_time / display_max * 100, current_time, display_max); // update progress
    }

    // copy audio from first video into output
    if (outAudio) {
        av_seek_frame(in[0].fmt, audio_stream_index, 0, AVSEEK_FLAG_BACKWARD);

        AVPacket pkt;
        while (av_read_frame(in[0].fmt, &pkt) >= 0) {
            if (pkt.stream_index != audio_stream_index) {
                av_packet_unref(&pkt);
                continue;
            }

            double pkt_time = pkt.pts * av_q2d(in[0].fmt->streams[audio_stream_index]->time_base);
            if (pkt_time > display_max) {
                av_packet_unref(&pkt);
                break;
            }

            pkt.stream_index = outAudio->index;
            av_packet_rescale_ts(&pkt, in[0].fmt->streams[audio_stream_index]->time_base, outAudio->time_base);

            av_interleaved_write_frame(outFmt, &pkt);
            av_packet_unref(&pkt);
        }
    }

    // finally, flush the encoder and process any remaining packets
    print_progress(100.0, display_max, display_max);
    printf("\nFlushing encoder...\n");

    AVPacket pkt{};
    avcodec_send_frame(encCtx, nullptr);

    while (avcodec_receive_packet(encCtx, &pkt) == 0) {
        av_packet_rescale_ts(&pkt, encCtx->time_base, outVideo->time_base);
        pkt.stream_index = outVideo->index;
        av_interleaved_write_frame(outFmt, &pkt);
        av_packet_unref(&pkt);
    }

    av_write_trailer(outFmt);

    printf("Done!\n");
    return 0;
}