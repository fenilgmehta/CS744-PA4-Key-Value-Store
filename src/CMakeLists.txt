cmake_minimum_required(VERSION 3.8)
project("CS744-PA4-Key-Value-Store" CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_VERBOSE_MAKEFILE on)

# REFER: Introduction to the basics  -  https://cliutils.gitlab.io/modern-cmake/chapters/basics.html

# ---------------------------------------------------------------------------------------------------------------------

# TODO: comment this when doing performance testing
# Set BUILD TYPE to Debug
set(CMAKE_BUILD_TYPE Debug)

# REFER: https://stackoverflow.com/questions/41361631/optimize-in-cmake-by-default
# This condition will set the BUILD TYPE to Release if not defined earlier
if (NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE Release)
endif ()

set(CMAKE_CXX_FLAGS "-Wall -Wextra")

# -g  Produce debugging information in the operating
# system's native format (stabs, COFF, XCOFF, or DWARF).
# GDB can work with this debugging information.
set(CMAKE_CXX_FLAGS_DEBUG "-g")

# NOTE: -O3 is more optimized than -O2
set(CMAKE_CXX_FLAGS_RELEASE "-O3 -w")

# ---------------------------------------------------------------------------------------------------------------------

## REFER: https://stackoverflow.com/questions/4765222/how-do-i-produce-only-an-object-file-o-from-a-cmake-build-target
## REFER: https://cmake.org/cmake/help/latest/command/add_library.html#object-libraries

# REFER: https://stackoverflow.com/questions/1620918/cmake-and-libpthread
set(THREADS_PREFER_PTHREAD_FLAG ON)
find_package(Threads REQUIRED)


add_library(MyDebugger.o OBJECT MyDebugger.hpp)
add_library(MyMemoryPool.o OBJECT MyMemoryPool.hpp)

add_library(KVClientLibrary.o OBJECT KVClientLibrary.hpp)
add_library(KVMessage.o OBJECT KVMessage.hpp)
add_library(KVStoreFileNames.o OBJECT KVStoreFileNames.h KVStoreFileNames.cpp)
add_library(KVStore.o OBJECT KVStore.hpp)

add_library(KVCache.o OBJECT KVCache.hpp)

add_executable(KVServer KVServer.cpp $<TARGET_OBJECTS:KVStoreFileNames.o> )
#add_executable(
#        KVServer
#        KVServer.cpp
#        $<TARGET_OBJECTS:MyVector.o>
#        $<TARGET_OBJECTS:KVCache.o>
#        $<TARGET_OBJECTS:KVStore.o>
#        $<TARGET_OBJECTS:KVMessage.o>
#        $<TARGET_OBJECTS:MyDebugger.o>
#        $<TARGET_OBJECTS:KVStoreFileNames.o>
#)

target_link_libraries(KVServer PRIVATE Threads::Threads)

add_executable(KVClient KVClient.cpp)
#add_executable(
#        KVClient
#        KVClient.cpp
#        $<TARGET_OBJECTS:KVClientLibrary.o>
#        $<TARGET_OBJECTS:KVMessage.o>
#)

add_executable(Testing Testing.cpp)
add_executable(TestingDatabase TestingDatabase.cpp)

# ---------------------------------------------------------------------------------------------------------------------

# NOTE: apply compile time DEFINITION to debugger
# REFER: https://cmake.org/cmake/help/v3.0/command/if.html
if (CMAKE_BUILD_TYPE STREQUAL Debug)
    MESSAGE(STATUS "CMAKE_BUILD_TYPE = \"${CMAKE_BUILD_TYPE}\"")

    # REFER: https://stackoverflow.com/questions/9017573/define-preprocessor-macro-through-cmake
    # add_definitions(-DDEBUGGING_ON)

    # REFER: https://cmake.org/cmake/help/latest/command/target_compile_definitions.html
    target_compile_definitions(MyDebugger.o PUBLIC DEBUGGING_ON)  # TODO: uncomment this
    MESSAGE(STATUS "\"DEBUGGING_ON\" flag set :)")
endif ()

# ---------------------------------------------------------------------------------------------------------------------
