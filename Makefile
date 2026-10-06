# torqalc.exe MAKEFILE for compiling under MinGW32

# Compiler and tools
CC       := i686-w64-mingw32-gcc
CXX      := i686-w64-mingw32-g++
LD       := i686-w64-mingw32-g++
DLLTOOL  := i686-w64-mingw32-dlltool
GENDEF   := gendef
RC       := i686-w64-mingw32-windres

# Targets
NAME     := torqalc_win32
TARGET   := torqalc.exe

# Build type: choose Debug or Release
# Default is Debug
BUILDTYPE ?= Debug

# Directories
SRC_DIR  := ./src
LOGGING_DIR  := $(SRC_DIR)/logging

# Automatically discover source files
HEADERS     := $(wildcard $(SRC_DIR)/*.h $(LOGGING_DIR)/*.h)
SRC_C       := $(wildcard $(SRC_DIR)/*.c)
SRC_CPP     := $(wildcard $(SRC_DIR)/*.cc $(LOGGING_DIR)/*.cc)
SRC_RC      := $(wildcard $(SRC_DIR)/*.rc)

# Objects
OBJ_C    := $(SRC_C:.c=.o)
OBJ_CPP  := $(SRC_CPP:.cc=.o)
OBJ_RC   := $(SRC_RC:.rc=.res)

# Compiler flags #
DEFINES  := -DUNICODE -D_UNICODE -D_WINDOWS -DWINVER=0x0500 -D_WIN32_WINNT=0x0500 -D_WIN64_WINNT=0x0502 -D_WIN32_IE=0x501
# Project-specific include dirs
INCLUDE_DIRS = -Isrc -Isrc/logging
# Show all errors, compile everything static, ensure src dir is included, -municode
# since this is a GUI windows app, ensure relocatable data
CFLAGS   := $(DEFINES) -Wall -Wunused-function -static-libgcc -municode -finput-charset=UTF-8 -fexec-charset=UTF-8 -MMD -MP $(INCLUDE_DIRS)

# March flags
SIMDFLAGS = -mfpmath=sse -mfxsr -msse

# Compiler optimization and architecture flags
ifeq ($(BUILDTYPE), Release)
CFLAGS   += -O3 -g0 -s $(SIMDFLAGS) -DNDEBUG -D_NDEBUG
endif
ifeq ($(BUILDTYPE), Debug)
CFLAGS   += -Og -g $(SIMDFLAGS) -DDEBUG -D_DEBUG
endif

# C++ only flags
CXXFLAGS := $(CFLAGS) -std=c++17 -static-libstdc++

# Libraries
LIBS     := -lkernel32 -luser32 -lgdi32 -lshell32 -lcomctl32 -lcomdlg32
# Linker flags
LDFLAGS  := -L$(LOGGING_DIR) -municode -Wl,--subsystem,windows:5.00

# Pin the default goal: the dependency -includes below would otherwise
# make the first .d rule (src/convert.o) the default target.
.DEFAULT_GOAL := all

# Include generated dependency files
-include $(OBJ_C:.o=.d)
-include $(OBJ_CPP:.o=.d)
-include $(OBJ_RC:.res=.d)

# Build Commands #
all: $(NAME)

$(NAME): $(TARGET)

# The .exe
$(TARGET): $(OBJ_C) $(OBJ_CPP) $(OBJ_RC)
	$(LD) $(LDFLAGS) -static $(OBJ_C) $(OBJ_CPP) $(OBJ_RC) $(LIBS) -o $(TARGET)

# Compilation Rules #
# Compile C sources
%.o: %.c
	$(CC) $(CFLAGS) -static -c $< -o $@

# Compile C++ sources
%.o: %.cc
	$(CXX) $(CXXFLAGS) -static -c $< -o $@

# Compile .rc → .o or .res
%.res: %.rc
	$(RC) $< -O coff $@

# Cleaning Rules #
clean:
	rm -f -v $(OBJ_C) $(OBJ_CPP) $(OBJ_RC) $(OBJ_C:.o=.d) $(OBJ_CPP:.o=.d) $(TARGET)

# Testing rules #
test:
	cat $(TARGET)

# PHONY targets for build deps tracking
.PHONY: all $(NAME) clean test
