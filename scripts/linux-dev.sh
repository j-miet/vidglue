#!/bin/bash
cd "../"
g++ $(find src -name '*.cpp') \
-o bin/vidglue \
-fopenmp \
 $(pkg-config --cflags --libs libavformat libavcodec libavutil libswscale)