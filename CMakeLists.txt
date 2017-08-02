cmake_minimum_required(VERSION 3.6)

project(pd-visualization VERSION 0.1 LANGUAGES C CXX)

option(USE_FFTW "Use FFTW" ON)

include(FindPkgConfig)
list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/cmake")
include(EnableAllWarnings)
include(PdExternal)

find_package(Threads REQUIRED)
find_package(FLTK REQUIRED)
if(USE_FFTW)
  pkg_check_modules(FFTW3F fftw3f)
endif()

link_directories(${FLTK_LIBRARY_DIRS})
if(USE_FFTW)
  link_directories(${FFTW3F_LIBRARY_DIRS})
endif()

set(visu_common_SOURCES
  src/visu~.cc
  src/visu~-remote.cc
  src/util/self_path.cc)
if(CMAKE_SYSTEM_NAME MATCHES "Windows")
  set(visu_common_SOURCES ${visu_common_SOURCES}
    src/util/win32_argv.cc
    src/util/win32_socketpair.cc)
endif()

add_library(visu_common STATIC ${visu_common_SOURCES})
target_include_directories(visu_common PUBLIC src)
set_target_properties(visu_common PROPERTIES
  POSITION_INDEPENDENT_CODE TRUE)

add_pd_external(wfvisu_tilde "wfvisu~" src/wfvisu~.cc)
target_link_libraries(wfvisu_tilde visu_common ${CMAKE_THREAD_LIBS_INIT})

add_pd_external(sgvisu_tilde "sgvisu~" src/sgvisu~.cc)
target_link_libraries(sgvisu_tilde visu_common ${CMAKE_THREAD_LIBS_INIT})

add_pd_external(ogvisu_tilde "ogvisu~" src/ogvisu~.cc)
target_link_libraries(ogvisu_tilde visu_common ${CMAKE_THREAD_LIBS_INIT})

add_executable(visu_gui
  src/visu~-gui.cc
  src/gui/w_dft_spectrogram.cc
  src/gui/w_dft_waterfall.cc
  src/gui/w_ts_oscillogram.cc)
set_target_properties(visu_gui PROPERTIES
  OUTPUT_NAME "visu~-gui"
  RUNTIME_OUTPUT_DIRECTORY "${PROJECT_SOURCE_DIR}")
target_compile_definitions(visu_gui
  PRIVATE COLOR_USE_PP2FILE=1)
target_include_directories(visu_gui
  PRIVATE src
  PRIVATE thirdparty/color/src
  PRIVATE ${FLTK_INCLUDE_DIRS})
target_link_libraries(visu_gui
  ${FLTK_LIBRARIES})

if(USE_FFTW)
  target_compile_definitions(visu_gui
    PRIVATE "USE_FFTW=1")
  target_include_directories(visu_gui
    PRIVATE ${FFTW3F_INCLUDE_DIRS})
  target_link_libraries(visu_gui
    ${FFTW3F_LIBRARIES})
else()
  target_include_directories(visu_gui
    PRIVATE thirdparty/kfr/include)
endif()
