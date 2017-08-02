#
# Find static library
#

macro(target_static_link TARGET)
  if ("${CMAKE_C_COMPILER_ID}" MATCHES GNU OR
      "${CMAKE_C_COMPILER_ID}" MATCHES Clang)
    set_property(TARGET ${TARGET}
      APPEND_STRING
      PROPERTY LINK_FLAGS " -static")
  else()
    message(WARNING "Static linking not supported on this compiler")
  endif()
endmacro()

macro(find_static_library)
  set(_orig_CMAKE_FIND_LIBRARY_SUFFIXES "${CMAKE_FIND_LIBRARY_SUFFIXES}")
  if (WIN32)
    set(CMAKE_FIND_LIBRARY_SUFFIXES ".lib" ".a")
  else()
    set(CMAKE_FIND_LIBRARY_SUFFIXES ".a")
  endif()
  find_library(${ARGN})
  set(CMAKE_FIND_LIBRARY_SUFFIXES "${_orig_CMAKE_FIND_LIBRARY_SUFFIXES}")
  unset(_orig_CMAKE_FIND_LIBRARY_SUFFIXES)
endmacro()

macro(find_static_library_of_shared_library VAR LIBRARY)
  set(_lib "${LIBRARY}")
  if(NOT _lib)
    set(${VAR} "${VAR}-NOTFOUND")
  elseif(NOT IS_ABSOLUTE "${_lib}")
    find_static_library(${VAR} "${_lib}")
  else()
    get_filename_component(_dir "${_lib}" DIRECTORY)
    get_filename_component(_name "${_lib}" NAME_WE)
    set(_staticlib "${_dir}/${_name}${CMAKE_STATIC_LIBRARY_SUFFIX}")
    if(NOT EXISTS "${_lib}")
      set(${VAR} "${VAR}-NOTFOUND")
    else()
      set(${VAR} "${_staticlib}")
    endif()
  endif()
  unset(_lib)
  unset(_dir)
  unset(_name)
  unset(_staticlib)
endmacro()
