## FFmpeg basic usage

This is for FFmpeg itself, **not vidglue**!

- Download executable from https://github.com/BtbN/FFmpeg-Builds/releases
- then run on CLI with ``your/path/ffmpeg <flags>``

## Example

``ffmpeg -hwaccel cuda -i vid1.mp4 -i vid2.mp4 -filter_complex "[0:v]scale=1580:1080[v0]; [1:v]scale=340:1080[v1]; [v0][v1]hstack=inputs=2[v]" -map "[v]" -map 0:a -c:v h264_nvenc -c:a aac -b:a 192k -cq 23 -preset p3 output.mp4``

- Two videos, side by side with custom resolutions. Uses audio from both files by default.
- uses GPU encoding (-hwaccel cuda) and GPU decoding (-c:v h264_nvenc)
- default quality (-preset p3) and compression (-cq 23)


## Useful flags

``-hwaccel cuda``  
For cuda hardware acceleration when decoding 

``-i vid1.mp4 vid2.mp4``  
Add video to output. Can take multiple inputs.

``-filter_complex "[0:v]scale=1580:-1[v0]; [1:v]scale=340:-1[v1]; [v0][v1]hstack=inputs=2[v]"``  
Custom layout for output
  1. ``[N:v]`` for Nth video
  2. ``scale=X:Y`` for resize; can use -1 for output value e.g. 1600:-1 with 1920x1080 output would scale it to 1600:1080.
  3. ``[var_name]`` for alias/variable e.g. ``[v0]`` above refers to first video ``[0:v]``
  4. ``hstack`` for horizontal stacking (side by side), ``vstack`` for vertical. Can combine hstacks/vstacks to create complex layouts.

``-map "[v]" -map 0:a``  
Included output streams
  1. ``[v]`` refers to custom variable 'v' for all layout videos (see filter_complex)
  2. ``0:a``refer to first videos audio stream
      - here you can also use ``-0:a`` (see the minus sign included) to mute specific audio streams

 Remember to map all streams individually (i.e. usually just two: one for output video, one for single audio output)

``-c:v h264_nvenc``  
Video codec for encoding
- Use h264_nvenc for NVIDIA GPU, libx264 for CPU

``-c:a aac``  
Audio codec; for MP4 use aac i.e. Advanced Audio Codec

``-c:b 192k``  
Audio bitrate. Roughly: 96k = low, 128k = decent, 192k = good, 320k = high 

``-cq 23 / -crf 23``  
cq = Constant Quality, crf = Constant Rate Factor. 
- Use cq for GPU, crf for CPU
- Controls output quality, this changes its size
- Use values in range 0-51. Lower value means higher quality but slower processing. Default value is 23.

``-preset p3``
How aggressively encoder compresses packets. Faster = bigger file, less CPU use.
- CPU common values if you use -crf flag: 

    ``veryslow, slower, slow, medium, fast, faster, veryfast, superfast, ultrafast``
    - usually fast/faster/veryfast is sufficient
- GPU values if you use -cq flag:  ``p7-p1``
    - p7 slowest, p1 fastest. 
    - usually p3-p4 is sufficient.

``-t 5``  
Render first N seconds of output video. Good for previewing.

``-an``  
Disables audio from output file

``output.mp4``  
this is the last argument and takes path of desired output video