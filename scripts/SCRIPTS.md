## Scripts

**Run these only inside the *scripts* directory, don't move them outside.**

### win-dev

- Builds executable from .cpp files and runs it. 
- Inputs are read from root folder so place any test videos here.
- the executable which is found in vidglue/bin is run using MSYS2 MinGW64 terminal. Thus it requires all linked libaries
installed in MinGW and cannot be simply run by double clicking the executable.

### win-release

- Copies required libraries from mingw64/bin directory to vidglue/release/vidglue
- builds executable into release/vidglue. This can be run as all required dlls are found in same directory now.
    - tested static linking, but it was a mess so .exe + dlls is the way to go
- creates a zip file containing vidglue.exe + required dlls

### linux-dev

- same as win-dev, just make sure you have installed ffmpeg dev dependencies + pkg-config + libx264

### linux-release

- same as win-release, again requires ffmpeg dev, pkg-config and libx264 dependencies
- for user, these are required (similarly to Windows dynamic dll dependencies):

```bash
sudo apt install ffmpeg
```

or specifically

```bash
sudo apt install libavcodec62 libavformat62 libavutil60 libswscale9 libx264-165
```