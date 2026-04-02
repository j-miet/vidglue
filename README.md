# vidglue

*Glue together* (= combine) multiple videos into a single output using free positioning and resizing.

Uses powerful [FFmpeg](#https://ffmpeg.org/documentation.html) libraries as its core and doesn't offer anything new 
that FFmpeg itself couldn't already do. 
Only reasons would be:

- you want a compact tool for combining multiple videos into one and don't care about other complex features
- you prefer simpler user interface: CLI commands + easy-to-use JSON config file
- smaller executable (zip with .exe + DLLs ~12MB, unzipped almost 30MB; FFmpeg is well over 100MB)

>Only Windows executable is provided, but code should be cross-platform compatible

For more info about FFmpeg, see [Licensing](#licencing) and [Other](#other) sections.

## Features

- supports any amount of input videos. 
- allows for grid and sequential video layouts:
    - **grid layouts** allows free positioning and resizing so you can place videos in rows, columns, NxN grids or in 
    any other formation really, just make sure they *fit the output resolution and don't overlap*. Output video has 
    length of longest input video.
    - **sequential layout** is similar and you can freely reposition and resize each video inside its own output window.
     Difference is each gets rendered in order one after another
- audio support:
    - for grids, audio is copied from the **first input** only -> no support for multiple audio streams. 
    - for sequences, audio is also copied, this time from each video individually and concatenated together. But this 
    means that even slightly different audio mismatches will get magnified in following videos e.g. quite minimal but 
    still noticeably pitch change 
        - fixing requires proper resampling which has not been implemented yet
- various settings to change output video 
    - quality, bitrate, compression, fps, speed
    - audio is currently not affected by speed multiplier
- can render only a part of video from the start e.g. first 30 seconds. Good for testing how layout looks in practice
- performance-wise pretty good and not too slow compared to FFmpeg in casual use
    - ffmpeg's internal threading for decode/encode
    - basic NVENC GPU hardware acceleration in encoding which can offer increase in speed, taking away some burden 
    from CPU
        - defaults to GPU use, but will automatically change to CPU if no support is detected. 
        - can also force CPU-only mode
    - CLI progress prints are displayed by default. They are not threaded which can also affect video composing speed 
    quite a bit. You can and might want to disable this for long high res videos renders, but otherwise just keep it
    enabled.

#### Possible future additions

- speed multiplier affects also audio
- speed multiplier can be controlled individually for each input (e.g. first video 0.5, second 2x, third 1.5 etc.)
- GPU decoding support + better threading
    - I tested splitting frame decoding to multiple threads with std::thread + basic GPU decoding, but both of these 
    ended up slowing the output process. Trying to fix these would eventually break my program so I just reverted back 
    to simpler and seemingly slower implementation. I guess there was just too much overhead which wasn't worth it.


## How to use

**--TODO--**

- add docstrings and better comments
- add cmd args
  - basic settings: res, fps, layouts
  - advanced settings (decode/encode)
  - other: video previews, disable audio (could also just use audio-free video as 1. input)
  - read from file (JSON config file should be fine)

## Building

Code should be cross-platform compatible, but only Windows has been tested as that's the main platform I use.

For Windows following DLLs are used:
- avcodec-62.dll
- avformat-62.dll
- avutil-60.dll
- swscale-9.dll
- libwinpthread-1.dll
- libgcc_s_seh-1.dll*
- libstdc++-6.dll*

*Last two are statically linked (-static-libstdc++ and -static-libgcc) so they are part of .exe, no separate DLL 
required

I use C++17 and compile on MSYS2 Mingw64 environment with gcc. Some build scripts are provided in ``scripts`` directory.
You can use these as a guide how code is build in Windows environment and extend it to other operating systems.

I might add Linux binaries eventually. MacOS however is too hard/unreliable to test without owning an actual Mac PC.

## Licencing

Original project code is licensed under MIT, see [LICENSE](LICENSE).

### FFmpeg

This project also uses [FFmpeg](https://ffmpeg.org/) version 8.1, licensed under the 
[LGPLv2.1 or later](https://ffmpeg.org/legal.html). 

- no modifications have been made to FFmpeg source code used in this project. Source code along with build.txt info 
file can be found in ``third-party/ffmpeg-src.7z``
- full LGPL license text can be found in ``third-party/FFmpeg-LGPL.md``

Following FFmpeg DLLs are dynamically linked to vidglue.exe and thus required to run the executable:
- avcodec-62.dll
- avformat-62.dll
- avutil-60.dll
- swscale-9.dll

## Other

Some additional FFmpeg links for reference:

Coding API examples written in C  
https://github.com/FFmpeg/FFmpeg/tree/master/doc/examples

Doxygen  
https://ffmpeg.org/doxygen/8.0/index.html

FFmpeg releases  
https://github.com/BtbN/FFmpeg-Builds/releases  
https://ffmpeg.org/releases/ (source .tar files)