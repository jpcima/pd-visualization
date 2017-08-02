#
# Enable all warnings
#

if ("${CMAKE_C_COMPILER_ID}" MATCHES GNU OR
    "${CMAKE_C_COMPILER_ID}" MATCHES Clang)
  set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -Wall")
endif()

if ("${CMAKE_CXX_COMPILER_ID}" MATCHES GNU OR
    "${CMAKE_CXX_COMPILER_ID}" MATCHES Clang)
  set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall")
endif()
