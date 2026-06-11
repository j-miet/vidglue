## vidglue config file setup

- [config.json fields](#configjson-fields)
- [TL;DR on quality controls](#tldr-on-quality-controls)
- [config.json template](#configjson-template)

### config.json fields


`mode` (string)
- defines rendering mode: grid or sequential. 
    - **Grid** renders all inputs into a grid layout
    - **Sequential** renders them one after another into a sequence
- **Values**: `"grid"` for grids, `"sequence"` for sequences 
    - to be precise, any value != "sequence" defaults to grid mode instead

---

#### Grid vs sequence visualization



Each video's individual position in output layout is defined as `[top_left_x, top_left_y, width, height]`.

**Here we use output resolution of 1920x1080**.

Let's have a following layout:         
- vid1: [0, 0, 1500, 600]        
- vid2: [0, 600, 1500, 480]  
- vid3: [1500, 600, 420, 0]

(in actual config file inputs are separate from layouts. Therefore layouts must be listed in same order as inputs; see [config template](#configjson-template) at the end of this document)

With grid mode enabled, we get a single grid where each video is part of a single output and as such, progresses simultaneously:

```
            .--------------------.
            |               |    |
            |      vid1     |vid3|
start  =>   |_______________|____|  => end after vid1 ends
            |      vid2     |    |
            |_______________|____|
```
- vid1 starts from (0,0) -> bottom-right corner is (0+1500, 0+600) = (1500, 600)
- vid2 starts from (0, 600) -> bottom-right corner is (0+1500, 600+480) = (1500, 1080)
- vid3 starts from (1500, 600) -> bottom-right corner is (1500+420, 600+0) = (1920, 600)
- bottom-right rectangle with top-left (1500, 600) and bottom right (1920, 1080) stays empty and will be displayed as black texture
- output video audio is copied from **vid1** only -> make sure to have longest video as first!


With sequential mode enabled, output video becomes a sequence of all input videos. Each plays one after another and is separated with black pause frames to serve as transition:

```
            .--------------------.     .--------------------.     .--------------------.
            |               |    |     |                    |     |               |    |
            |      vid1     |    | [P] |                    | [P] |               |vid3|    
start  =>   |_______________|    | ->  |_______________     | ->  |               |____|  => end after final video ends
            |                    |     |      vid2     |    |     |                    |
            |____________________|     |_______________|____|     |____________________|
```

- layout is the same as above in grid, but now only a single video is embedded into output frame of 1920x1080 at a time. Any unused area is filled with black texture again.
- each video has it's own original audio

---
<br>

`inputs` (string, multiple values allowed)
- video inputs to be part of rendered output. Prefer common formats like `.mp4`.
- order is important as inputs are processed in order:
    - grid: only first input is used as audio source so if you want to have audio from any video, place that first into inputs list
    - sequential: video order in final output is same as input list order
- **Values:**
Path of each input video. If you place videos into same folder as vidglue.exe, only video name is needed e.g. myvideo.mp4

`output` (string)
- **Values**: Output video path. If only name is given, output is generated into same directory as vidglue.exe


`layout` (integer-value list with 4 entries, multiple values allowed)
- position and size of each input in the final output
- follows input order so make sure each layout corresponds to desired input
- syntax is [top-left x, top-left y, width, height]. Then each layout entry forms a bounding rectangle with coordinates
    - (x, y) top-left
    - (x+width, y) top-right
    - (x, y+height) bottom-left
    - (x+width, y+height) bottom-right
- **Values:** Layout of each input video as a list [x, y, w, h] where x, y, w, h are integers


`outWidth` (integer)
- width of output video
- **Values:** integers >= 0


`outHeight` (integer)
- height of output video
- **Values:** integers >= 0


`fps` (integer)
- output video frame rate
- **Values:** integers >= 0


`audioEnabled` (boolean)
- enable or disable audio
- **Values:** true (use audio), false (disable audio)


`previewDuration` (decimal)
- how many seconds are rendered into output starting at the beginning of video
- e.g. 30 would render first 30 seconds, 10.5 would render 10 and half seconds etc.
- works a bit differently for grid and sequential:
    - for grids, produces exact length output
    - for sequential, <u>each input gets rendered with preview length into output</u>. So if you had 3 videos and preview is 10 seconds: first video renders first 10 seconds, then second renders first 10 and finally third renders first 10 -> output is 30 seconds instead of 10
- **Values:** decimals >= 0


`pauseDuration` (decimal, **Sequential layouts only**)
- pause interval between sequentially rendered inputs. When video ends, displays black screen for X seconds before next one begins. Useful for clearly separating video segments
- **Values:** decimals >= 0


`scalerFlags` (integer)
- scaling algorithm primary flag (optional secondary flags not implemented)
- controls speed, quality and method of video resolution scaling
- ffmpeg supports a lot of flags. Only a small subset of these are available here, more could be added in the future
- **Values:**
    - 0 (SWS_FAST_BILINEAR -> fast, low quality)
    - 1 (SWS_BILINEAR -> balanced speed+quality)
    - 2 (SWS_BICUBIC -> better quality, slower)
    - 3 (SWS_LANCZOS -> high-quality, much slower)
    - using any other integer defaults to 1


`speedMultiplier` (decimal)
- video speed multiplier. Video length is thus inversely scaled (e.g. 2x speed, 1/2 length)
- if values > 1, uses frame skipping. 
- <u>doesn't affect audio</u> as current implementation just copies audio from inputs without resampling
- **Values:** decimals >= 0


`progressTimestamps` (integer)
- how frequently is progress updated during rendering or equivalently, to how many update slices is total progress cut into into
- because progress updates are not threaded and thus are processed between frames, these affect processing speed significantly!
    - if your output video will be long and uses high res, you should definitely limit timestamps. Otherwise rendering could take a lot longer
    - for long videos, I would recommend values anything <= 1000; this would the roughly update between 0.1%. But 100 (1%) or 10 (10%) are also very good

    => For short videos, doesn't really matter. For long hd quality videos, use low values (<= 1000, preferably even <= 100) 
- **Values:**
    - 0 or greater (int N > 0) -> splits progress updates into N segments. E.g. N=10 would update progress on every tenth segment i.e. every 10% (10%, 20%, ..., 90%, 100%) whereas N=3 would means every 33% (roughly 33%, 66%, 99%)
    - 0 -> no pauses, progress gets updated every frame (not recommended for long high quality renders)
    - -1 -> disables progress displaying entirely (max performance)


`cpuPreset` (string)
- controls compression efficiency
- usually "medium"/"fast"/veryfast" should be enough. For boundary values:
    - "veryslow" = very slow -> best compression and small files
    - "ultrafast" = very fast -> bad compression, large files
- **Values:** There are more values, but these should give good variety:
    - "veryslow"
    - "slow"
    - "medium"
    - "fast"
    - "veryfast"
    - "ultrafast"


`cpuCrf` (integer)
- constant rate factor: visual quality + bitrate targeting. Doesn't rely on hardware limits (unlike gpu), making it very efficient. 
- lower values increase quality but increase file size and higher vice versa
- **Values:** integers 0-51
    - 18 is good lower boundary for max quality (lower values give diminishable changes)
    - 23 is default
    - values higher from here start to quickly get more compressed


`useGpu` (boolean)
- if available, use GPU functions to speed up processing. If set to **true**, vidglue will automatically detect gpu with this setting: it it fails, defaults to CPU.
- currently only advantage is GPU encoding which is still very useful. 
    - **be aware that this produces much larger output files on average than cpu-only render**. Probably not an issue unless you render multi-hour length hd videos
    - so use this if 1. you need speed 2. want to take some load off from CPU 3. don't mind larger file sizes
    - CPU uses libx264 encoder, GPU uses h264_nvenc encoder (so Nvidia GPUs only)
- **Values:** true (use gpu) or false (force CPU,) 


`gpuPreset` (string)
- control compression efficiency 
- not same as cpu preset setting: more compression = smaller output file size, but slower rendering while also affecting quality because of hardware dependency
- uses simple 7-step scale p1-p7: 
    - p1 is fastest with weakest compression, lowest quality, p7 slowest with best compression, high quality
    - good default is p3
- **Values:** string values "p1", "p2", ..., "p6", "p7"


`gpuRc` (string)
- rate control mode: bitrate/quality control based on mode selected. This setting controls whether *QP*, quality or bitrate stays constant
- uses *quantization parameter* (QP) underneath which adjusts the data precision i.e. how much detail will encoder discard. This then affects both bitrate and output quality. In the end, 
    - QP is the deciding force, not bitrate/quality
    - when bitrate/quality are targeted, encoder adjusts QP dynamically to fit target goal
- because gpu algorithms are hardware dependent, there's no single CRF-like setting and instead different modes and quality controls (see gpuCq field below this part) are needed

> TL;DR: treat gpuRc as the bitrate controlling part of cpu's CRF when using GPU
- **Values:**
    - for **constant quality** (<u>always recommended for normal use</u>), you need
        1. mode with variable bitrate -> simply use "vbr" or "vbr_hq", no need to consider others
        2. quality target by setting gpuCq value (see below)

        => if you don't want to think about these, just use  
        `gpuRc="vbr"` and `gpuCq=23`, OR  
        `gpuRc="vbr_hq` and `gpuCq=23`

    - "cqp" = **constant quantization** (same level of reduction in precision. Bitrate & quality vary a lot. Not useful in practice)
    - "cbr" = **constant bitrate** (only for streaming, useless here)
    - "vbr"/"vbr_hq" = **variable bitrate** (aims for average bitrate, quality varies. Using hq aims for better quality)


`gpuCq` (integer)
- constant quality: visual quality target value, auto-adjusts bitrate around it
- **works only on certain RC modes like vbr/vbr_hq where bitrate is not a constant** e.g. with gpuRc="cbr" this would have no effect
- lower value = better quality but larger file size
- similar idea to cpu's CRF, but algorithm is simpler and limited by hardware thus not as effective. Uses same value scale but corresponds to values loosely (e.g cpuCrf=30 is not the same as gpuCq=30)
> TL;DR: treat gpuCq as the quality controlling part of cpu's CRF value
- **Values:** integers 0-51


`bFrames` (integer)
- bi-directional frames: compression efficiency + quality increase while keeping bitrate similar vs latency (= time between fetching a frame vs displaying it) and processing (like encoding speed)
- frames use 3 types: I, P, B
    - I-frames are full images
    - P-frames are predicted from previous frames
    - B-frames are produced from both previous and upcoming frames   
    - increasing B-frames only affect encode/decode delay
> because delay doesn't matter in offline environments, you should always use some b-frames for "free" quality and compression improvements
- **Values:** integers >= 0, keep this value around 2-4 for normal use. Avoid overly aggressive compression with values >= 10.


### TL;DR on quality controls:
- preset = how aggressive processing should encoder use
- CRF = CPU quality AND bitrate under a single parameter
- CQ = loosely controls GPU bitrate
- RC = controls GPU quality if CQ allows it, otherwise has no effect
- B-frames = compression efficiency vs latency, which in offline use simply means better quality and compression without a real downside as long used value is relatively small

In general it comes down to finding right balance for these values:
- bitrate  = amount of data used per second to produce output approximation
- quality  = how similar/detailed the encoded output looks compared to inputs
- compression = how efficiently can the available data be used to approximate desired output
    - high compression = small file, but possible loss of bitrate/detail
    - low compression = large file, too much bitrate/detail compared to what's needed
- processing speed = amount of time it takes to produce output


### config.json template

    {
        "mode": 0,
        "inputs": ["vid1.mp4", "vid2.mp4"],
        "output": "output.mp4",
        "layout": [
            [0, 0, 1560, 1080],
            [1560, 0, 360, 1080]
        ],
        "outWidth": 1920,
        "outHeight": 1080,
        "fps": 60,
        "audioEnabled": true,
        "previewDuration": 0,
        "pauseDuration": 0,
        "scalerFlags": 0,
        "speedMultiplier": 1.0,
        "progressTimestamps": 100,
        "useGpu": true,
        "gpuPreset": "p3",
        "gpuRc": "vbr",
        "gpuCq": 23,
        "cpuPreset": "veryfast",
        "cpuCrf": 23,
        "bFrames": 2
    }

Simple template for different use cases:
- don't need to touch cpu & gpu specific values (gpuPreset=... and below)
- can disable gpu with `"useGpu": false` if you prefer smaller file sizes and don't mind the increase in rendering time + cpu usage
- other than these, you can freely change values and test outputs by setting `"previewDuration"` value > 0 instead of rendering entire videos
- also remember to keep `"progressTimestamps` value low relative to output video length as this could **significantly** slow down your rendering speed. Don't use value 0 generally: it will print progress on every single frame!
