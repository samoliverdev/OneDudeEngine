cmake_minimum_required(VERSION 3.16)

project(soloud)

add_definitions(-DWITH_MINIAUDIO)
#add_definitions(-DWITH_WASAPI)
#add_definitions(-DWITH_WINMM)

file(GLOB_RECURSE SOLOUD_SOURCE_FILES 
    src/c_api/*.c*
    src/audiosource/*.c*
    src/core/*.c*
    src/filter/*.c*
    src/backend/miniaudio/*.c*
    #src/backend/wasapi/*.c*
    #src/backend/winmm/*.c*
)
add_library(soloud STATIC ${SOLOUD_SOURCE_FILES})
target_include_directories(soloud PUBLIC include/ src/)

#target_link_libraries(engine PUBLIC glfw )