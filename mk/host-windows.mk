# -*- mode: makefile; -*-
include mk/makefile-top.mk
common.sources += src/util/win32_socketpair.cc src/util/win32_argv.cc
gui.sources += src/util/win32_socketpair.cc src/util/win32_argv.cc
fltk_ldlibs := -Wl,--push-state,-Bstatic $(fltk_ldlibs) -Wl,--pop-state \
   -lgdi32 -lole32 -luuid -lcomctl32
fft_ldlibs := -Wl,--push-state,-Bstatic $(fft_ldlibs) -Wl,--pop-state
sys_ldlibs = -lws2_32 -lshlwapi
sys_ldflags = -static-libgcc -static-libstdc++ \
  -Wl,--push-state,-Bstatic,--whole-archive -lwinpthread -Wl,--pop-state
exe_ext = .exe
include mk/makefile-bottom.mk
include Makefile.pdlibbuilder
