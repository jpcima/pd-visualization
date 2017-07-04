# -*- mode: makefile; -*-
include mk/makefile-top.mk
common.sources += src/util/win32_socketpair.cc src/util/win32_argv.cc
gui.sources += src/util/win32_socketpair.cc src/util/win32_argv.cc
fltk_ldlibs := -Wl,--push-state,-Bstatic $(fltk_ldlibs) -Wl,--pop-state \
   -lgdi32 -lole32 -luuid -lcomctl32
fftwf_ldlibs := -Wl,--push-state,-Bstatic $(fftwf_ldlibs) -Wl,--pop-state
sys_cflags = -D_WINVER=0x600 -D_WIN32_WINNT=0x600
sys_ldlibs = -lws2_32 -lshlwapi
sys_ldflags = -static-libgcc -static-libstdc++ \
  -Wl,--push-state,-Bstatic,--whole-archive -lwinpthread -Wl,--pop-state
exe_ext = .exe
include mk/makefile-bottom.mk
include Makefile.pdlibbuilder
