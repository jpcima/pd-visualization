#
# Puredata externals
#

if(CMAKE_SYSTEM_NAME MATCHES "Linux")
  set(PD_EXTERNAL_SUFFIX ".pd_linux")
elseif(CMAKE_SYSTEM_NAME MATCHES "Darwin")
  set(PD_EXTERNAL_SUFFIX ".pd_darwin")
elseif(CMAKE_SYSTEM_NAME MATCHES "Windows")
  set(PD_EXTERNAL_SUFFIX ".dll")
else()
  message(FATAL_ERROR "unrecognized platform")
endif()

macro(add_pd_external TARGET NAME)
  add_library(${TARGET} MODULE ${ARGN})
  target_compile_definitions(${TARGET}
    PRIVATE "PD")
  set_target_properties(${TARGET} PROPERTIES
    OUTPUT_NAME "${NAME}"
    LIBRARY_OUTPUT_DIRECTORY "${PROJECT_SOURCE_DIR}"
    PREFIX ""
    SUFFIX "${PD_EXTERNAL_SUFFIX}")
endmacro()
