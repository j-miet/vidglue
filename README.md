# vidglue

**Glue/combine multiple videos together in grid or sequence formation to produce a single output where each video can be 
positioned and resized freely.**

Uses powerful FFmpeg libraries as its core; see [FFmpeg](#ffmpeg) sections for more info.

Vidglue doesn't offer anything new FFmpeg itself couldn't already do.
Only reasons would be:

- you want a compact tool for combining multiple videos into one and don't care about other complex features
- you prefer easy-to-use JSON config file instead of CLI commands
- much smaller executable size


## Table of contents

- [<u>Features</u>](#features)
- [<u>Installation</u>](#installation)
    - [<u>Linux (Ubuntu/Debian)</u>](#linux-ubuntudebian)
    - [<u>Windows</u>](#windows)
- [<u>How to use</u>](#how-to-use)
- [<u>Dependencies</u>](#dependencies)
- [<u>Licensing</u>](#licensing)


## Features

- supports any amount of input videos in grid or sequential layouts:
    - **grid** allows free positioning and resizing so you can place videos in rows, columns, NxN grids or in 
    any other uneven formation. Any unfilled space stays black. <u>Also make sure that regions don't overlap!</u>
        - video length is longest video's length
    - **sequential** is similar and you can freely position and resize each video inside its own output window.
     Final video is then a sequence of all videos in one single output, with optional black pause/transition frames 
     between each to clearly separate them.
        - video length is sum of all video lengths + pause_timer * (total_video_count-1)

    For a simple visual image, check [here](docs/ConfigJSON.md#grid-vs-sequence-visualization)

- audio support
    - for grids, audio is copied from the **first input** only -> no support for multiple audio streams. 
    - for sequences, audio is also copied but this time from each video individually and concatenated together. Now this 
    means that audio format mismatches will get magnified in following videos e.g. small but noticeable 
    pitch change (fixing requires proper resampling, this hasn't been implemented)
- config json file to edit output video 
    - basic configs such as size, fps, speed etc. Also some advanced options to target quality, bitrate and 
    compression. **Audio is currently not affected by speed multiplier**
    - can also limit rendering length. Useful for previewing what output looks like for long videos
- performance-wise pretty good
    - ffmpeg internal multithreaded decode/encode for cpu, also optional nvidia gpu encoding support
    - probably biggest resource cost is CLI progress prints.
    **Progress print calculations are not threaded and will directly affect video composing speed quite a bit if done 
    on every single frame.** These are displayed by default, but limited a lot. Frequency can and should be controlled 
    with *progressTimestamps* field in config file, especially when rendering longer, high quality videos


## Installation via Makefile


### Linux (Ubuntu/Debian)

Install ffmpeg and x264:

```bash
sudo apt update

sudo apt install \
    ffmpeg \
    libavcodec-dev \
    libavformat-dev \
    libavutil-dev \
    libswscale-dev \
    libx264-dev \
    build-essential
```

Dev packages are for installation, ffmpeg itself for running vidglue.  
You can verify version with `ffmpeg -version`

You most likely don't need to install these manually, but in case you do:  
`sudo apt install libgomp1 libgomp-plugin-dev`

After installing dependencies, in MINGW64 shell, use command `make release` to generate the release 
build in `release/vidglue-{VERSION}-linux64`.


### Windows

First make sure you have installed [MSYS2](https://www.msys2.org/) MINGW64 shell.

Following FFmpeg DLLs are dynamically linked to vidglue.exe and thus required to run the executable:
- avcodec-62.dll
- avformat-62.dll
- avutil-60.dll
- swscale-9.dll


To obtain these, use either:

> https://www.gyan.dev/ffmpeg/builds/#release-builds

- release builds -> release_full_shared (dlls + x264 & nvenc support)

OR

> https://github.com/BtbN/FFmpeg-Builds

- pick release (latest probably the best)
- pick gpl-shared: this includes the dlls in bin + again supports x264 & nvenc

> Self-build configurations can be less efficient, especially when both Cpu and Gpu (Nvidia) need to be supported.  
> Therefore downloading the dlls this way is preferred.

You can build executable via makefile:
- put dlls into `C:/msys64/mingw64/bin` or similar, depending where you installed your msys
- open MSYS2 MINGW64 terminal
- run command `make release`

This builds the release version into `release/vidglue-{VERSION}-winx64`. All dlls are includes and should always be kept in the same location with the vidglue.exe


## How to use

A JSON configuration file is used for everything.

- if not there already, create a JSON file e.g. `config.json`. Easiest way is to just copy one from root dir and then optionally rename it
- edit the fields to your liking. Use the link below for explanations
    - full paths are supported so you don't need to have inputs and/or output in same dir with the executable
- when you're done editing, save config
- make sure all your input videos are in the same directory with your `vidglue.exe` and run the executable with `-c <configPath>` flag:

    ```bash
    vidglue -c config.json
    ```

    You can also add optional `--wopen` flag which leaves process window open after completion and required user to press enter.

    ```bash
    vidglue -c config.json --wopen
    ```

Check [this document](./docs/ConfigJSON.md) for full details on what each config file field means


## Dependencies

### FFmpeg

This project requires [FFmpeg](https://ffmpeg.org/) version 8.1, build with GPL components enabled. This includes also libx264 support.

Following FFmpeg libraries are required:
- libavcodec62
- libavformat62
- libavutil60
- libswscale9
- libx264


Of course you are free to use different versions e.g. if ones listed are not available for your operating system.

For example,
different Linux distributions may provide different x264 versions e.g. libx264-164 or libx264-165. Any compatible build
should still work.


#### Additional ffmpeg links

- Coding API examples written in C  
https://github.com/FFmpeg/FFmpeg/tree/master/doc/examples

- Doxygen (for 8.0; 8.1 didn't have one since I last checked)  
https://ffmpeg.org/doxygen/8.0/index.html

- FFmpeg releases  
https://github.com/BtbN/FFmpeg-Builds/releases  
https://ffmpeg.org/releases/ (source .tar files)



### Optional GPU Acceleration

This project optionally supports NVIDIA NVENC hardware encoding (no decoding, at least yet)
when available on supported NVIDIA GPUs.

This feature requires NVIDIA proprietary drivers.


## Licensing
This project is licensed under GPLv2-or-later.

It uses FFmpeg configured with GPL components, including libx264.

- FFmpeg: https://ffmpeg.org
- x264: https://www.videolan.org/developers/x264.html
