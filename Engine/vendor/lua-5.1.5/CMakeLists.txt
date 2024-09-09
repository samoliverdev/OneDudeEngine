cmake_minimum_required(VERSION 3.16)

project(LUA)

file(GLOB_RECURSE LUA_SOURCE_FILES src/*.c)
add_library(LUA STATIC ${LUA_SOURCE_FILES})
