#
# Puredata externals
#

include(StaticLinking)

if(CMAKE_SYSTEM_NAME MATCHES "Windows")
  if(NOT PD_WINDOWS_PROGRAM_DIR)
    message(FATAL_ERROR "Please set PD_WINDOWS_PROGRAM_DIR on the Windows platform")
  endif()
  set(PD_INCLUDE_DIR "${PD_WINDOWS_PROGRAM_DIR}/src")
  set(PD_LIBRARIES "${PD_WINDOWS_PROGRAM_DIR}/bin/pd.lib")
else()
  find_path(PD_INCLUDE_DIR "m_pd.h")
  set(PD_LIBRARIES)
endif()

if(NOT PD_INCLUDE_DIR OR NOT EXISTS "${PD_INCLUDE_DIR}/m_pd.h")
  message(FATAL_ERROR "Cannot find the Puredata headers")
endif()

message(STATUS "Found Puredata headers: ${PD_INCLUDE_DIR}")

if(CMAKE_SYSTEM_NAME MATCHES "Linux")
  set(PD_EXTERNAL_SUFFIX ".pd_linux")
elseif(CMAKE_SYSTEM_NAME MATCHES "Darwin")
  set(PD_EXTERNAL_SUFFIX ".pd_darwin")
elseif(CMAKE_SYSTEM_NAME MATCHES "Windows")
  set(PD_EXTERNAL_SUFFIX ".dll")
else()
  message(FATAL_ERROR "Unrecognized platform")
endif()

macro(add_pd_external TARGET NAME)
  add_library(${TARGET} MODULE ${ARGN})
  target_include_directories(${TARGET}
    PRIVATE "${PD_INCLUDE_DIR}")
  target_compile_definitions(${TARGET}
    PRIVATE "PD")
  target_link_libraries(${TARGET}
    ${PD_LIBRARIES})
  set_target_properties(${TARGET} PROPERTIES
    OUTPUT_NAME "${NAME}"
    LIBRARY_OUTPUT_DIRECTORY "${PROJECT_SOURCE_DIR}"
    PREFIX ""
    SUFFIX "${PD_EXTERNAL_SUFFIX}")
  if(LINK_STATICALLY)
    target_static_link(${TARGET})
  endif()
endmacro()
