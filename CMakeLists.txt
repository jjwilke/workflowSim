cmake_minimum_required (VERSION 3.4.3)
# 3.3.2 is not sufficient; not sure about stuff between that and 3.4.3

# Silence an error about the version option to project
# this will need to be changed if we ever bump the version to 3.0 and put the
# version stuff in the project command itself
if (NOT CMAKE_VERSION VERSION_LESS 3.0)
  cmake_policy(SET CMP0048 OLD)
endif()

INCLUDE (CheckIncludeFileCXX)
INCLUDE (CheckIncludeFile)

project (PinSimBuild)
# The version number.
set (PinSimBuild_VERSION_MAJOR 0)
set (PinSimBuild_VERSION_MINOR 1)

include(CTest)
enable_testing()

set(SST_CORE "" CACHE STRING "Path to install prefix of SST-core")
set(PIN_ROOT "" CACHE STRING "Path to root of Intel PIN distribution")

if ("${SST_CORE}" STREQUAL "")
  message(FATAL_ERROR "must set -DSST_CORE= to install prefix of sst-core")
endif()

if ("${PIN_ROOT}" STREQUAL "")
  message(FATAL_ERROR "must set -DPIN_ROOT= to root of Intel PIN distribution")
endif()

set(PIN_INCLUDE_DIRS 
  ${PIN_ROOT}/source/include/pin 
  ${PIN_ROOT}/source/include/pin/gen 
  ${PIN_ROOT}/extras/components/include 
  ${PIN_ROOT}/extras/xed-intel64/include/xed 
  ${PIN_ROOT}/source/tools/InstLib 
  ${PIN_ROOT}/extras/crt/include)

set(PIN_SYSTEM_INCLUDES
  ${PIN_ROOT}/extras/stlport/include 
  ${PIN_ROOT}/extras/libstdc++/include 
  ${PIN_ROOT}/extras/crt/include 
  ${PIN_ROOT}/extras/crt/include/arch-x86_64 
  ${PIN_ROOT}/extras/crt/include/kernel/uapi 
  ${PIN_ROOT}/extras/crt/include/kernel/uapi/asm-x86)

set(PIN_CXX_FLAGS
  -fPIC 
  -fno-stack-protector 
  -fno-exceptions 
  -funwind-tables 
  -fno-rtti 
  -fomit-frame-pointer 
  -fno-strict-aliasing)

set(PIN_DEFINES 
  TARGET_MAC
  __PIN__=1 
  PIN_CRT=1 
  TARGET_IA32E
  HOST_IA32E   
  BIGARRAY_MULTIPLIER=1 
  __DARWIN_ONLY_UNIX_CONFORMANCE=1 
  __DARWIN_UNIX03=0)

CHECK_INCLUDE_FILE_CXX( sst/core/interprocess/sstmutex.h FOUND_SST -I${SST_CORE}/include )
set(CMAKE_REQUIRED_FLAGS "")
foreach (PTH ${PIN_INCLUDE_DIRS})
  set (CMAKE_REQUIRED_FLAGS "${CMAKE_REQUIRED_FLAGS} -I${PTH}")
endforeach()
foreach (PTH ${PIN_SYSTEM_INCLUDES})
  set (CMAKE_REQUIRED_FLAGS "${CMAKE_REQUIRED_FLAGS} -isystem ${PTH}")
endforeach()
foreach (DEF ${PIN_DEFINES})
  set (CMAKE_REQUIRED_FLAGS "${CMAKE_REQUIRED_FLAGS} -D${DEF}")
endforeach()
foreach (FLG ${PIN_CXX_FLAGS})
  set (CMAKE_REQUIRED_FLAGS "${CMAKE_REQUIRED_FLAGS} ${FLG}")
endforeach()
CHECK_INCLUDE_FILE_CXX( pin.H FOUND_PIN )

if (NOT FOUND_SST)
  message(FATAL_ERROR "cannot locate valid SST header files at given SST_CORE path")
endif()

if (NOT FOUND_PIN)
  message(FATAL_ERROR "cannot locate valid PIN header files at given PIN_ROOT path")
endif()

add_subdirectory(exe)
add_subdirectory(pintools)


