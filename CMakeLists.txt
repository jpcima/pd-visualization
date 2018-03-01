cmake_minimum_required(VERSION 3.6)

project(pd-visualization VERSION 0.1 LANGUAGES C CXX)

set(STATIC_LINK_DEFAULT OFF)
if(CMAKE_SYSTEM_NAME MATCHES "Windows")
  set(STATIC_LINK_DEFAULT ON)
endif()

option(USE_FFTW "Use FFTW" ON)
option(USE_STATIC_FLTK "Use static FLTK" "${STATIC_LINK_DEFAULT}")
option(LINK_STATICALLY "Use static linking" "${STATIC_LINK_DEFAULT}")

include(FindPkgConfig)
list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/cmake")
include(GNUWarnings)
include(PdExternal)
include(StaticLinking)

set_gnu_warning("all")
set_gnu_warning("no-unused-local-typedefs")

set(FLTK_SKIP_OPENGL TRUE)
set(FLTK_SKIP_FORMS TRUE)
set(FLTK_SKIP_IMAGES TRUE)
set(FLTK_SKIP_FLUID TRUE)

find_package(Threads REQUIRED)
find_package(FLTK REQUIRED)

if(USE_STATIC_FLTK OR LINK_STATICALLY)
  find_package(StaticFLTK REQUIRED)
  set(PROJECT_FLTK_LIBRARIES "${StaticFLTK_LIBRARIES}")
else()
  set(PROJECT_FLTK_LIBRARIES "${FLTK_LIBRARIES}")
endif()

if(USE_FFTW)
  pkg_check_modules(FFTW3F fftw3f)
endif()

link_directories(${FLTK_LIBRARY_DIRS})
if(USE_FFTW)
  link_directories(${FFTW3F_LIBRARY_DIRS})
endif()

set(CMAKE_CXX_STANDARD "14")
add_definitions("-D__STDC_FORMAT_MACROS=1")

set(visu_names "wfvisu~" "ogvisu~" "sgvisu~")

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
target_include_directories(visu_common
  PUBLIC src
  PUBLIC "${PD_INCLUDE_DIR}")
target_link_libraries(visu_common
  PUBLIC ${PD_LIBRARIES} PUBLIC ${CMAKE_THREAD_LIBS_INIT})
if(CMAKE_SYSTEM_NAME MATCHES "Linux")
  target_link_libraries(visu_common
    PUBLIC dl)
endif()
set_target_properties(visu_common PROPERTIES
  POSITION_INDEPENDENT_CODE TRUE)
if(CMAKE_SYSTEM_NAME MATCHES "Windows")
  target_link_libraries(visu_common PUBLIC ws2_32 shlwapi)
endif()

add_pd_external(wfvisu_tilde "wfvisu~" src/wfvisu~.cc)
target_link_libraries(wfvisu_tilde visu_common)

add_pd_external(sgvisu_tilde "sgvisu~" src/sgvisu~.cc)
target_link_libraries(sgvisu_tilde visu_common)

add_pd_external(ogvisu_tilde "ogvisu~" src/ogvisu~.cc)
target_link_libraries(ogvisu_tilde visu_common)

###

add_library(Fl_Knob STATIC
  thirdparty/Fl_Knob/Fl_Knob/Fl_Knob.cxx)
target_include_directories(Fl_Knob
  PUBLIC thirdparty/Fl_Knob
  PUBLIC ${FLTK_INCLUDE_DIRS})
target_link_libraries(Fl_Knob ${PROJECT_FLTK_LIBRARIES})

set(visu_gui_SOURCES
  src/visu~-gui.cc
  src/gui/w_dft_spectrogram.cc
  src/gui/w_dft_waterfall.cc
  src/gui/w_ts_oscillogram.cc
  src/gui/fl_widgets_ex.cc
  src/util/unit_format.cc)
if(CMAKE_SYSTEM_NAME MATCHES "Windows")
  set(visu_gui_SOURCES ${visu_gui_SOURCES}
    src/util/win32_argv.cc
    src/util/win32_socketpair.cc)
endif()

add_executable(visu_gui ${visu_gui_SOURCES})
set_target_properties(visu_gui PROPERTIES
  OUTPUT_NAME "visu~-gui"
  RUNTIME_OUTPUT_DIRECTORY "${PROJECT_SOURCE_DIR}")
target_compile_definitions(visu_gui
  PRIVATE COLOR_USE_PP2FILE=1)
target_include_directories(visu_gui
  PRIVATE src
  PRIVATE thirdparty/color/src
  PRIVATE ${FLTK_INCLUDE_DIRS})
target_link_libraries(visu_gui Fl_Knob ${PROJECT_FLTK_LIBRARIES})
if(LINK_STATICALLY)
  target_static_link(visu_gui)
endif()

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

pkg_check_modules(JACK jack)
if(JACK_FOUND)
  link_directories(${JACK_LIBRARY_DIRS})
  include_directories(${JACK_INCLUDE_DIRS})
  add_executable(visu_jack
    src/jack-main.cc)
  target_link_libraries(visu_jack
    PRIVATE visu_common PRIVATE ${JACK_LIBRARIES})
  set_property(TARGET visu_jack PROPERTY
    OUTPUT_NAME "visu~-jack")
  install(TARGETS visu_jack DESTINATION bin)
  install(TARGETS visu_gui DESTINATION bin)
  foreach(visu_name ${visu_names})
    install(FILES "scripts/run-jack-visu~.sh"
      PERMISSIONS
        OWNER_READ OWNER_WRITE OWNER_EXECUTE
        GROUP_READ GROUP_EXECUTE
        WORLD_READ WORLD_EXECUTE
      DESTINATION bin
      RENAME "${visu_name}")
  endforeach()
endif()
