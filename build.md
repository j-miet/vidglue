## Windows

On Windows, building is not really necessary as pre-build dlls already exist:

Use either:

https://www.gyan.dev/ffmpeg/builds/#release-builds

- release builds -> release_full_shared (dlls + x264 & nvenc support)

OR

https://github.com/BtbN/FFmpeg-Builds

- pick release (latest probably the best)
- pick gpl-shared: this includes the dlls in bin + again supports x264 & nvenc

Simply extract the required dlls to vidglue's bin or release folders OR if this doesn't work for some reason 
(but it should as it searches locally for dlls first), put them in 
`C:/msys64/mingw64/bin`

Self-build configurations can be less efficient, especially when both Cpu and Gpu (Nvidia) need to be supported. 
Therefore this is the preferred way.


## Linux

- for dependencies, just do

    ```bash
    sudo apt install libavcodec-dev libavformat-dev libavutil-dev libswscale-dev pkg-config libx264-dev
    ```
    libx264 enables performant CPU encoding (and program would crash without it anyway)


- also make sure you have fopenmp support available with
`sudo apt install libgomp1 libgomp-plugin-dev`, probably already there with GCC

- then just compile with build_dev.sh script


#### Using executable

on Linux user should install actual versions instead of dev packages. Easiest way is to just install ffmpeg itself:

```bash
sudo apt install ffmpeg
```

Because packages depend on used distro, this command gets up-to-date ones instead of needing to find specific versions.

Otherwise use this:

```bash
sudo apt install libavcodec62 libavformat62 libavutil60 libswscale9 libx264-165
```


This matches the Windows Dll dynamic linking logic.

With these vidglue should operate as expected.

