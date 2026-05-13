# vidglue

*Glue* multiple videos together in grid or sequence formation to produce a single output where each video can be 
positioned and resized freely.

Uses [FFmpeg](https://ffmpeg.org/documentation.html) libraries as its core.

Vidglue doesn't offer anything new FFmpeg itself couldn't already do.
Only reasons would be:

- you want a compact tool for combining multiple videos into one and don't care about other complex features
- you prefer easy-to-use JSON config file instead of CLI commands
- smaller executable (zip with .exe + DLLs ~12MB, unzipped ~30MB; FFmpeg is well over 100MB)


## Features

- supports any amount of input videos in grid or sequential layouts:
    - **grid** allows free positioning and resizing so you can place videos in rows, columns, NxN grids or in 
    any other uneven formation. Any unfilled space stays black. <u>Also make sure that regions don't overlap!</u>
    - **sequential** is similar and you can freely position and resize each video inside its own output window.
     Final video is then a sequence of all videos in one single output, with optional black pause/transition frames 
     between each to clearly separate them.
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

## How to use

CLI commands are not currently supported. Instead a JSON configuration file `config.json` is used for everything.

- if not there already, create a JSON file called `config.json` into same directory where your `vidglue.exe` is 
located. Easiest way is to just copy one from root dir
- edit the fields to your liking. Use the link below for explanations
- when you're done editing, save config
- make sure all your input videos are in the same directory with your `vidglue.exe` and run the executable

Check [this document](./docs/ConfigJSON.md) for full details


## Licensing

Original project code is licensed under MIT, see [LICENSE](LICENSE).


### FFmpeg

This project also uses [FFmpeg](https://ffmpeg.org/) version 8.1.

Following FFmpeg DLLs are dynamically linked to vidglue.exe and thus required to run the executable:
- avcodec-62.dll
- avformat-62.dll
- avutil-60.dll
- swscale-9.dll

## Additional stuff

#### Some FFmpeg links for reference:

Home   
https://ffmpeg.org/

Coding API examples written in C  
https://github.com/FFmpeg/FFmpeg/tree/master/doc/examples

Doxygen (for 8.0; 8.1 didn't have one since I last checked)  
https://ffmpeg.org/doxygen/8.0/index.html

FFmpeg releases  
https://github.com/BtbN/FFmpeg-Builds/releases  
https://ffmpeg.org/releases/ (source .tar files)

#### Other third-party dependencies:

Simple-to-use json library for parsing config file (MIT license allows this to be included as json.hpp in vidglue src 
files)
https://github.com/nlohmann/json/blob/develop/single_include/nlohmann/json.hpp