# vidglue

*Glue* multiple videos together in a grid/sequence formation to produce a single output where each video can be positioned and resized freely.

Uses powerful [FFmpeg](https://ffmpeg.org/documentation.html) libraries as its core, these are licensed under FFmpeg project's LGPLv2.1; see [Licensing](#licensing). Vidglue doesn't offer anything new FFmpeg itself couldn't already do.
Only reasons would be:

- you want a compact tool for combining multiple videos into one and don't care about other complex features
- you prefer easy-to-use JSON config file instead of CLI commands
- smaller executable (zip with .exe + DLLs ~12MB, unzipped ~30MB; FFmpeg is well over 100MB)

For additional FFmpeg links, see [Other](#other). For Windows build instructions reference, see [Building](#building).


## Features

- supports any amount of input videos
- allows for grid and sequential video layouts:
    - **grid layouts** allows free positioning and resizing so you can place videos in rows, columns, NxN grids or in 
    any uneven formation really, just make sure they *fit the output resolution and don't overlap*. Output video has 
    length of longest input video.
    - **sequential layout** is similar and you can freely reposition and resize each video inside its own output window.
     Difference is, each gets rendered in order one after another
        - optional: can also add black pause frames between videos e.g. first video ends, 5 second black screen then
        second video begins etc.
- audio support:
    - for grids, audio is copied from the **first input** only -> no support for multiple audio streams. 
    - for sequences, audio is also copied, this time from each video individually and concatenated together. But this 
    means that even slightly audio format mismatches will get magnified in following videos e.g. small but noticeable pitch change 
        - fixing requires proper resampling which has not been implemented yet
- various settings to change output video 
    - quality, bitrate, compression, fps, speed  
        -> audio is currently not affected by speed multiplier
- set a video rendering limit counted from the start. For example using value 30 produces output of the first 30 seconds. Good for testing how layouts looks in practice.
- performance-wise pretty good and not too slow compared to FFmpeg in casual use
    - ffmpeg's internal threading for decode/encode
    - basic NVENC GPU hardware acceleration in encoding which can offer increase in speed, taking away some burden 
    from CPU
        - defaults to GPU use, but will automatically change to CPU if no support is detected. 
        - can also force CPU-only mode
    - CLI progress prints are displayed by default, but can be customized by setting any update frequency or disabling this feature entirely. **Printing is not threaded which can also affect video composing speed quite a bit if left unlimited.**

For better explanations, see [How to Use](#how-to-use)


#### Possible future additions

Nothing too big to keep tool scope small and compact:

- Audio:
    - combine differing audio streams without side effects
    - speed multiplier affects also audio

- Video:
    - speed multiplier can be controlled individually for each input (e.g. first video 0.5, second 2x, third 1.5 etc.)

- Other:
    - maybe GPU decoding support + proper decode threading with mutex & locks. I tested splitting frame decoding to 
    multiple threads with std::thread, also tested basic GPU decoding. However both of these ended up slowing the output
     process. Trying to fix these would eventually things so bad I would just reverted back to simpler and seemingly 
     slower implementation. I guess there was just too much overhead which wasn't worth it.


## How to use

CLI commands are not currently supported. Instead a JSON configuration file `config.json` is used for everything.

- if not there already, create a JSON file called `config.json` into same directory where your `vidglue.exe` is 
located. Easiest way is to just copy one from root dir
- edit the fields to your liking. Use the link below for explanations
- when you're done editing, save config
- make sure all your input videos are in the same directory with your `vidglue.exe` and run the executable

Check [this document](./docs/ConfigJSON.md) for full details


## Building

Code should be cross-platform compatible, but only Windows has been tested.

For Windows following DLLs are used:
- avcodec-62.dll
- avformat-62.dll
- avutil-60.dll
- swscale-9.dll
- libwinpthread-1.dll
- libgcc_s_seh-1.dll*
- libstdc++-6.dll*

*These are statically linked (-static-libstdc++ and -static-libgcc) and are part of .exe, no separate DLLs 
required

>FFmpeg license requires all its libraries to be dynamically linked so these are always included alongside the executable!

Code is written in C++17 and executable has been build on MSYS2 Mingw64 environment with gcc compiler. Some build scripts are provided in ``scripts`` directory.
You can use these as a guide how code is build in Windows environment and possibly extend it to other operating systems.


## Licensing

Original project code is licensed under MIT, see [LICENSE](LICENSE).


### FFmpeg

This project also uses [FFmpeg](https://ffmpeg.org/) version 8.1, licensed under the 
[LGPLv2.1 or later](https://ffmpeg.org/legal.html). 

- no modifications have been made to FFmpeg source code used in this project. Source code along with build.txt 
describing compile+build instructions can be found in ``third-party/ffmpeg-src.7z``
- full LGPL license text can be found in ``third-party/FFmpeg-LGPL.md``
- [github direct link to third-party directory](https://github.com/j-miet/vidglue/tree/main/third-party)

Following FFmpeg DLLs are dynamically linked to vidglue.exe and thus required to run the executable:
- avcodec-62.dll
- avformat-62.dll
- avutil-60.dll
- swscale-9.dll

These DLL files (+ others not related to ffmpeg but needed in compiling) can be found in `vidglue/releases`


## Other

#### Some additional FFmpeg links for reference:

Website  
https://ffmpeg.org/

Coding API examples written in C  
https://github.com/FFmpeg/FFmpeg/tree/master/doc/examples

Doxygen (for 8.0; 8.1 didn't one since I last checked)  
https://ffmpeg.org/doxygen/8.0/index.html

FFmpeg releases  
https://github.com/BtbN/FFmpeg-Builds/releases  
https://ffmpeg.org/releases/ (source .tar files)

#### Others:

Very useful and simple json library header (MIT license, can be just included in project files)
https://github.com/nlohmann/json/blob/develop/single_include/nlohmann/json.hpp