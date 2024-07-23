cmake_minimum_required(VERSION 3.16)

enable_testing()

set(CMAKE_CXX_STANDARD 17)
#set(CMAKE_WINDOWS_EXPORT_ALL_SYMBOLS ON)
#set(CMAKE_MSVC_RUNTIME_LIBRARY)

set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR})
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR})

set(GLFW_BUILD_DOCS OFF CACHE BOOL "" FORCE)
set(GLFW_BUILD_TESTS OFF CACHE BOOL "" FORCE)
set(GLFW_BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)
set(GLFW_INSTALL OFF CACHE BOOL "" FORCE)

set(BUILD_CPU_DEMOS OFF CACHE BOOL "" FORCE)
set(BUILD_BULLET2_DEMOS OFF CACHE BOOL "" FORCE)
set(BUILD_BULLET3_DEMOS OFF CACHE BOOL "" FORCE)
set(BUILD_OPENGL3_DEMOS OFF CACHE BOOL "" FORCE)
set(BUILD_OPENGL2_DEMOS OFF CACHE BOOL "" FORCE)
set(ENABLE_VHACD OFF CACHE BOOL "" FORCE)
set(BUILD_PYBULLET OFF CACHE BOOL "" FORCE)
set(BUILD_EXTRAS OFF CACHE BOOL "" FORCE)
set(INSTALL_LIBS OFF CACHE BOOL "" FORCE)
set(BUILD_UNIT_TESTS OFF CACHE BOOL "" FORCE)
set(INSTALL_CMAKE_FILES OFF CACHE BOOL "" FORCE)
set(USE_GLUT OFF CACHE BOOL "" FORCE)
set(USE_GRAPHICAL_BENCHMARK OFF CACHE BOOL "" FORCE)
#set(BULLET2_MULTITHREADING OFF CACHE BOOL "" FORCE)
#set(USE_MSVC_RUNTIME_LIBRARY_DLL OFF CACHE BOOL "" FORCE)

set(BUILD_SHARED_LIBS OFF CACHE BOOL "" FORCE)
set(ASSIMP_BUILD_TESTS OFF CACHE BOOL "" FORCE)
set(ASSIMP_INSTALL OFF CACHE BOOL "" FORCE)
set(ASSIMP_BUILD_ALL_EXPORTERS_BY_DEFAULT  OFF CACHE BOOL "" FORCE)
set(ASSIMP_NO_EXPORT  ON CACHE BOOL "" FORCE)

set(YAML_BUILD_SHARED_LIBS  OFF CACHE BOOL "" FORCE)
set(YAML_CPP_BUILD_TOOLS  OFF CACHE BOOL "" FORCE)
set(YAML_CPP_INSTALL  OFF CACHE BOOL "" FORCE)

set(FT_WITH_BZIP2  OFF CACHE BOOL "" FORCE)
set(FT_WITH_HARFBUZZ  OFF CACHE BOOL "" FORCE)
set(FT_WITH_PNG  OFF CACHE BOOL "" FORCE)
set(FT_WITH_ZLIB  OFF CACHE BOOL "" FORCE)

set(RECASTNAVIGATION_DEMO  OFF CACHE BOOL "" FORCE)
set(RECASTNAVIGATION_TESTS  OFF CACHE BOOL "" FORCE)
set(RECASTNAVIGATION_EXAMPLES  OFF CACHE BOOL "" FORCE)
set(RECASTNAVIGATION_DEMO  OFF CACHE BOOL "" FORCE)

add_subdirectory(engine/vendor/glfw)
add_subdirectory(engine/vendor/assimp)
add_subdirectory(engine/vendor/bullet3)
add_subdirectory(engine/vendor/soloud)
add_subdirectory(engine/vendor/freetype)
add_subdirectory(engine/vendor/recastnavigation)

#----------------Engine------------------
project(engine)

if(CMAKE_CXX_COMPILER_ID MATCHES "Clang")
    #using Clang
else() 
    add_compile_options(/bigobj)
    add_compile_options(/MP)
endif()

file(GLOB_RECURSE ENGINE_SOURCE_FILES 
    engine/vendor/tinyfiledialogs/*.c
    engine/vendor/glad/*.c 
    engine/vendor/stb/*.c 
    engine/vendor/cgltf/*.c
    #engine/vendor/glm/*.cpp
    engine/vendor/imgui/*.cpp 
    engine/vendor/ImGuizmo/*.cpp 
    engine/vendor/entt/*.cpp
    engine/src/*.cpp
)
add_library(engine SHARED ${ENGINE_SOURCE_FILES})
#add_library(engine STATIC ${ENGINE_SOURCE_FILES})
target_precompile_headers(engine INTERFACE engine/src/OD/pch.h)
target_compile_definitions(engine PUBLIC OD_BUILD_DLL)

target_include_directories(engine PUBLIC 
    engine/vendor/
    engine/vendor/cgltf/
    engine/vendor/cereal/include/
    engine/vendor/magic_enum/include/
    engine/vendor/imgui/
    engine/vendor/ImGuizmo/
    engine/vendor/glfw/include/
    engine/vendor/assimp/include/
    engine/vendor/bullet3/src
    engine/vendor/entt/
    engine/vendor/soloud/include/
    engine/vendor/freetype/include/
    engine/vendor/recastnavigation/Recast/Include/
    engine/vendor/recastnavigation/Detour/Include/
    engine/vendor/recastnavigation/DetourCrowd/Include/
    engine/vendor/recastnavigation/Detour/Include/
    engine/src/
)

target_link_libraries(engine PUBLIC 
    glfw 
    assimp
    soloud
    freetype
    DebugUtils Detour DetourCrowd DetourTileCache Recast
    Bullet3Common Bullet3Collision Bullet3Dynamics BulletDynamics BulletCollision BulletSoftBody LinearMath
)

#----------------Sandbox------------------
project(game)

file(GLOB_RECURSE GAME_SOURCE_FILES game/src/*.cpp)
add_executable(game ${GAME_SOURCE_FILES})
target_include_directories(game PUBLIC engine/src/)
target_include_directories(game PUBLIC game/src/)
target_link_libraries(game engine)
target_compile_definitions(game PUBLIC RESOURCES_PATH="${CMAKE_CURRENT_SOURCE_DIR}")
#target_compile_definitions(game PUBLIC RESOURCES_PATH="../../res/")

#----------------Dynamic module------------------

project(dynamic_module)
file(GLOB_RECURSE DYNAMIC_MODULE_SOURCE_FILES dynamic_module/src/*.cpp)
add_library(dynamic_module SHARED ${DYNAMIC_MODULE_SOURCE_FILES})
#target_include_directories(dynamic_module PUBLIC engine/vendor/)
#target_include_directories(dynamic_module PUBLIC engine/vendor/cereal/include/)
#target_include_directories(dynamic_module PUBLIC engine/vendor/magic_enum/include/)
target_include_directories(dynamic_module PUBLIC engine/src/)
#target_include_directories(dynamic_module PUBLIC game/src/)
target_link_libraries(dynamic_module engine)
target_compile_definitions(dynamic_module PUBLIC RESOURCES_PATH="${CMAKE_CURRENT_SOURCE_DIR}")


# Enable Disable Tests
if(OFF) 

project(tests)

add_subdirectory(tests/googletest EXCLUDE_FROM_ALL)
add_library(GTest::GTest INTERFACE IMPORTED)
target_link_libraries(GTest::GTest INTERFACE gtest_main)

file(GLOB_RECURSE TESTS_SOURCE_FILES tests/src/*.cpp)
add_executable(tests ${TESTS_SOURCE_FILES})

target_include_directories(tests PUBLIC engine/src/)
target_link_libraries(tests GTest::GTest engine)

add_test(tests_gtests tests)

endif()