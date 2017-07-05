
all:
clean:
.PHONY: all clean

PKG_CONFIG ?= pkg-config

lib.name = visu~
class.sources = src/wfvisu~.cc src/sgvisu~.cc
common.sources = src/visu~.cc src/visu~-remote.cc src/util/self_path.cc
gui.sources = src/visu~-gui.cc src/gui/w_dft_waterfall.cc src/gui/w_dft_spectrogram.cc

all_cflags = -std=gnu++14 -fvisibility=hidden -D__STDC_FORMAT_MACROS=1 \
  -Isrc -Ithirdparty/color/src -DCOLOR_USE_PP2FILE=1

fltk_cflags =
fltk_ldlibs = -lfltk
# fft_cflags = $(shell $(PKG_CONFIG) fftw3f --cflags) -DUSE_FFTW=1
# fft_ldlibs = $(shell $(PKG_CONFIG) fftw3f --libs)
fft_cflags = -Ithirdparty/kfr/include
fft_ldlibs =
