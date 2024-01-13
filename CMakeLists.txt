cmake_minimum_required(VERSION 3.16)

enable_testing()

set(CMAKE_CXX_STANDARD 17)

set(GLFW_BUILD_DOCS OFF CACHE BOOL "" FORCE)
set(GLFW_BUILD_TESTS OFF CACHE BOOL "" FORCE)
set(GLFW_BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)
set(GLFW_INSTALL OFF CACHE BOOL "" FORCE)
set(BUILD_SHARED_LIBS OFF)

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

set(YAML_BUILD_SHARED_LIBS  OFF CACHE BOOL "" FORCE)
set(YAML_CPP_BUILD_TOOLS  OFF CACHE BOOL "" FORCE)
set(YAML_CPP_INSTALL  OFF CACHE BOOL "" FORCE)

set(FT_WITH_BZIP2  OFF CACHE BOOL "" FORCE)
set(FT_WITH_HARFBUZZ  OFF CACHE BOOL "" FORCE)
set(FT_WITH_PNG  OFF CACHE BOOL "" FORCE)
set(FT_WITH_ZLIB  OFF CACHE BOOL "" FORCE)

set(ozz_build_samples  OFF CACHE BOOL "" FORCE)
set(ozz_build_howtos  OFF CACHE BOOL "" FORCE)
set(ozz_build_tests  OFF CACHE BOOL "" FORCE)
#set(ozz_build_msvc_rt_dll  ON CACHE BOOL "" FORCE)
#set(ozz_build_simd_ref  ON CACHE BOOL "" FORCE)

add_subdirectory(engine/vendor/glfw EXCLUDE_FROM_ALL)
add_subdirectory(engine/vendor/assimp EXCLUDE_FROM_ALL)
add_subdirectory(engine/vendor/bullet3 EXCLUDE_FROM_ALL)
add_subdirectory(engine/vendor/soloud EXCLUDE_FROM_ALL)
add_subdirectory(engine/vendor/freetype EXCLUDE_FROM_ALL)
#add_subdirectory(engine/vendor/ozz-animation)
#add_subdirectory(engine/vendor/ozz-animation/ ozz_animation EXCLUDE_FROM_ALL)

#----------------Engine------------------
project(engine)

file(GLOB_RECURSE ENGINE_SOURCE_FILES 
    engine/vendor/tinyfiledialogs/*.c
    engine/vendor/glad/*.c 
    engine/vendor/stb/*.c 
    engine/vendor/cgltf/*.c
    engine/vendor/glm/*.cpp
    engine/vendor/imgui/*.cpp 
    engine/vendor/ImGuizmo/*.cpp 
    engine/vendor/entt/*.cpp
    engine/src/*.cpp
)
add_library(engine STATIC ${ENGINE_SOURCE_FILES})

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
    engine/src/
)

target_link_libraries(engine PUBLIC 
    glfw 
    assimp
    soloud
    freetype
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