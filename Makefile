CXX := g++
PROJECT_ROOT := $(shell pwd)
SRC_DIR := $(PROJECT_ROOT)/src
BIN_DIR := $(PROJECT_ROOT)/bin
RELEASE_DIR := $(PROJECT_ROOT)/release

VERSION := v0.1.2

# platform specific configurations
OS_NAME := $(shell uname -s 2>/dev/null)
OS_NAME := $(strip $(OS_NAME))

ifeq ($(OS),Windows_NT)
    PLATFORM := win64
else ifeq ($(OS_NAME),Linux)
    PLATFORM := linux64
else ifeq ($(findstring MINGW,$(OS_NAME)),MINGW)
    PLATFORM := win64
else
    PLATFORM := unknown
endif

BASE_NAME := vidglue

ifeq ($(PLATFORM),win64)
    EXE_EXT := .exe
else
    EXE_EXT :=
endif

VERSION := $(strip $(VERSION))
PLATFORM := $(strip $(PLATFORM))
TARGET := $(BASE_NAME)$(EXE_EXT)

RELEASE_VERSION := $(strip $(VERSION))-$(strip $(PLATFORM))
RELEASE_NAME := $(BASE_NAME)-$(RELEASE_VERSION)
RELEASE_PATH := $(RELEASE_DIR)/$(RELEASE_NAME)
SRCS := $(wildcard $(SRC_DIR)/*.cpp)

ARCHIVE_NAME := vidglue-$(RELEASE_VERSION)
TAR_FILE := $(RELEASE_DIR)/$(ARCHIVE_NAME).tar.gz
ZIP_FILE := $(RELEASE_DIR)/$(ARCHIVE_NAME).zip


# pkg-config for ffmpeg + x264
FFMPEG_CFLAGS := $(shell pkg-config --cflags libavformat libavcodec libavutil libswscale)
FFMPEG_LIBS_LINUX := -lavformat -lavcodec -lavutil -lswscale -lx264 -lpthread -lm -ldl
FFMPEG_LIBS_WIN := -lavformat -lavcodec -lavutil -lswscale -lx264 -lpthread -lm

ifeq ($(PLATFORM),win64)
    FFMPEG_LIBS := $(FFMPEG_LIBS_WIN)
else
    FFMPEG_LIBS := $(FFMPEG_LIBS_LINUX)
endif

# builder flags
CXXFLAGS_DEV := -fopenmp -std=c++17 -g
CXXFLAGS_REL := -fopenmp -std=c++17 -O2

# Windows static runtime tweak (only MinGW) + add dll vars
ifeq ($(PLATFORM),winx64)
    LDFLAGS_EXTRA := -static-libgcc -static-libstdc++
else
    LDFLAGS_EXTRA :=
endif

LDFLAGS := $(FFMPEG_LIBS)

DLLS := avcodec-*.dll avformat-*.dll avutil-*.dll swscale-*.dll libwinpthread-1.dll
FFMPEG_BIN := /mingw64/bin

# targets

.PHONY: dev release clean run

dev: 
	mkdir -p $(BIN_DIR)
	$(CXX) $(SRCS) \
		-o $(BIN_DIR)/$(TARGET) \
		$(CXXFLAGS_DEV) \
		$(FFMPEG_CFLAGS) \
		$(LDFLAGS)

run: dev
	$(BIN_DIR)/$(TARGET)

release:
	rm -rf $(RELEASE_PATH)
	mkdir -p $(RELEASE_PATH)

	$(CXX) $(SRCS) \
		-o $(BIN_DIR)/$(TARGET) \
		$(CXXFLAGS_REL) \
		$(FFMPEG_CFLAGS) \
		$(LDFLAGS)

	cp $(BIN_DIR)/$(TARGET) $(RELEASE_PATH)/
	cp $(PROJECT_ROOT)/config.json $(RELEASE_PATH)/ || true

ifeq ($(PLATFORM),win64)
	@echo "Copying FFmpeg DLLs..."
	for dll in $(DLLS); do \
		cp $(FFMPEG_BIN)/$$dll $(RELEASE_PATH)/ 2>/dev/null || true; \
	done
endif

# zipping (.tar.gz or .zip)
ifeq ($(PLATFORM),linux64)
	cd $(RELEASE_DIR) && tar -czf $(ARCHIVE_NAME).tar.gz $(ARCHIVE_NAME) 2>/dev/null || true
endif

ifeq ($(PLATFORM),winx64)
	cd $(RELEASE_DIR) && zip -r $(ARCHIVE_NAME).zip $(ARCHIVE_NAME) 2>/dev/null || true
endif

clean:
	rm -rf $(BIN_DIR) $(RELEASE_DIR)