cmake_minimum_required(VERSION 3.8)
project("PA_4_Key_Value_Store" CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_VERBOSE_MAKEFILE on)

# REFER: Introduction to the basics  -  https://cliutils.gitlab.io/modern-cmake/chapters/basics.html

# ---------------------------------------------------------------------------------------------------------------------

# TODO: comment this when doing performance testing
set(CMAKE_BUILD_TYPE Debug)

# REFER: https://stackoverflow.com/questions/41361631/optimize-in-cmake-by-default
if (NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE Release)
endif ()

set(CMAKE_C_FLAGS "-Wall -Wextra")
set(CMAKE_C_FLAGS_DEBUG "-g")
set(CMAKE_C_FLAGS_RELEASE "-O3")

# ---------------------------------------------------------------------------------------------------------------------

## REFER: https://stackoverflow.com/questions/4765222/how-do-i-produce-only-an-object-file-o-from-a-cmake-build-target
## REFER: https://cmake.org/cmake/help/latest/command/add_library.html#object-libraries

add_library(MyVector.o OBJECT MyVector.hpp)
add_library(KVMessage.o OBJECT KVMessage.hpp)
add_library(KVMessageFormat.o OBJECT KVMessageFormat.hpp)
add_library(MyDebugger.o OBJECT MyDebugger.hpp)
add_library(MyMemoryPool.o OBJECT MyMemoryPool.hpp)

add_library(KVCache.o OBJECT KVCache.hpp)
add_library(KVStore.o OBJECT KVStore.hpp)
add_executable(
        KVServer
        KVServer.cpp
        $<TARGET_OBJECTS:MyVector.o>
        $<TARGET_OBJECTS:KVCache.o>
        $<TARGET_OBJECTS:KVStore.o>
        $<TARGET_OBJECTS:KVMessage.o>
        $<TARGET_OBJECTS:KVMessageFormat.o>
        $<TARGET_OBJECTS:MyDebugger.o>
)

add_library(KVClientLibrary.o OBJECT KVClientLibrary.hpp)
add_executable(
        KVClient
        KVClient.cpp
        $<TARGET_OBJECTS:KVClientLibrary.o>
        $<TARGET_OBJECTS:KVMessage.o>
        $<TARGET_OBJECTS:KVMessageFormat.o>
)

add_executable(Testing Testing.cpp)

# ---------------------------------------------------------------------------------------------------------------------

# NOTE: apply compile time DEFINITION to debugger
# REFER: https://cmake.org/cmake/help/v3.0/command/if.html
if (CMAKE_BUILD_TYPE STREQUAL Debug)
    MESSAGE(STATUS "CMAKE_BUILD_TYPE = \"${CMAKE_BUILD_TYPE}\"")

    # REFER: https://stackoverflow.com/questions/9017573/define-preprocessor-macro-through-cmake
    # add_definitions(-DDEBUGGING_ON)

    # REFER: https://cmake.org/cmake/help/latest/command/target_compile_definitions.html
    # target_compile_definitions(MyDebugger.o PUBLIC DEBUGGING_ON)  # TODO: uncomment this
    MESSAGE(STATUS "\"DEBUGGING_ON\" flag set :)")

endif ()

# ---------------------------------------------------------------------------------------------------------------------

#add_library(MY_all_1.o     OBJECT       clock.c clock.h config.h fcyc.c fcyc.h fsecs.c fsecs.h ftimer.c ftimer.h mdriver.c memlib.c memlib.h mm1.c mm.h)
#add_library(MY_all_1.o     OBJECT       clock.c clock.h config.h fcyc.c fcyc.h fsecs.c fsecs.h ftimer.c ftimer.h mdriver.c memlib.c memlib.h mm1.c mm.h)
#add_library(MY_all_1.o     OBJECT       clock.c clock.h config.h fcyc.c fcyc.h fsecs.c fsecs.h ftimer.c ftimer.h mdriver.c memlib.c memlib.h mm1.c mm.h)
#add_library(MY_all_1.o     OBJECT       clock.c clock.h config.h fcyc.c fcyc.h fsecs.c fsecs.h ftimer.c ftimer.h mdriver.c memlib.c memlib.h mm1.c mm.h)
#add_executable(KVServer $<TARGET_OBJECTS:MY_all_1.o>)
#add_executable(KVServer $<TARGET_OBJECTS:MY_all_1.o>)
#add_executable(mdriver2 $<TARGET_OBJECTS:MY_all_2.o>)


#add_library(MyObj OBJECT ${CMAKE_CURRENT_SOURCE_DIR}/src/object.c)
#add_library(MyLibrary STATIC $<TARGET_OBJECTS:MyObj> MyClass1.cpp Helpers.cpp)

# add_library(MY_mdriver.o OBJECT mdriver.c fsecs.h fcyc.h clock.h memlib.h config.h mm.h)
# add_library(MY_memlib.o  OBJECT memlib.c memlib.h)
# add_library(MY_mm.o      OBJECT mm.c mm.h memlib.h)
# add_library(MY_mm1.o     OBJECT mm1.c mm.h memlib.h)
# add_library(MY_mm2.o     OBJECT mm2.c mm.h memlib.h)
# add_library(MY_fsecs.o   OBJECT fsecs.c fsecs.h config.h)
# add_library(MY_fcyc.o    OBJECT fcyc.c fcyc.h)
# add_library(MY_ftimer.o  OBJECT ftimer.c ftimer.h config.h)
# add_library(MY_clock.o   OBJECT clock.c clock.h)
# add_executable(
#     mdriver      
#     $<TARGET_OBJECTS:MY_mdriver.o>
#     $<TARGET_OBJECTS:MY_memlib.o>
#     $<TARGET_OBJECTS:MY_mm.o>
#     $<TARGET_OBJECTS:MY_mm1.o>
#     $<TARGET_OBJECTS:MY_mm2.o>
#     $<TARGET_OBJECTS:MY_fsecs.o>
#     $<TARGET_OBJECTS:MY_fcyc.o>
#     $<TARGET_OBJECTS:MY_ftimer.o>
#     $<TARGET_OBJECTS:MY_clock.o>
# )

#--------------------------------------------------------------------------------------------------------------------------------------------------------------------

#file(GLOB sourceFILES *.cpp *.c *.o)
#
#FOREACH (sourceFile ${sourceFILES})
#    get_filename_component(sourceFile_Path ${sourceFile} PATH)
#    MESSAGE(STATUS "Process file, path: ${sourceFile}")
#
#    get_filename_component(sourceFile_Name ${sourceFile} NAME)
#    string(REPLACE " " "_" sourceFile_Name ${sourceFile_Name})
#    string(REPLACE "'" "" sourceFile_Name ${sourceFile_Name})
#    string(REPLACE "\"" "" sourceFile_Name ${sourceFile_Name})
#    string(REPLACE "(" "_" sourceFile_Name ${sourceFile_Name})
#    string(REPLACE ")" "_" sourceFile_Name ${sourceFile_Name})
#    MESSAGE(STATUS "Process file, name: \"${sourceFile_Name}\"")
#
#    add_executable("${sourceFile_Name}" "${sourceFile}")
#
#endforeach (sourceFile)
