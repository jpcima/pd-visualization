#
# Enable all warnings
#

include(CheckCCompilerFlag)
include(CheckCXXCompilerFlag)

macro(set_gnu_warning id)
  if ("${CMAKE_C_COMPILER_ID}" MATCHES GNU OR
      "${CMAKE_C_COMPILER_ID}" MATCHES Clang)
    check_c_compiler_flag("-W${id}" _test)
    if(_test)
      set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -W${id}")
    endif()
  endif()
  if ("${CMAKE_CXX_COMPILER_ID}" MATCHES GNU OR
      "${CMAKE_CXX_COMPILER_ID}" MATCHES Clang)
    check_cxx_compiler_flag("-W${id}" _test)
    if(_test)
      set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -W${id}")
    endif()
  endif()
  unset(_test)
endmacro()
