#!/bin/bash
chmod +x build_dev.sh
export PATH=/mingw64/bin:$PATH
cd "../"
g++ $(find src -name '*.cpp') \
-o bin/vidglue \
-fopenmp \
 $(pkg-config --cflags --libs libavformat libavcodec libavutil libswscale)