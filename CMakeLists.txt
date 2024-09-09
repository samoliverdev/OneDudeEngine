cmake_minimum_required(VERSION 3.16)

#----------------Engine------------------

set(CMAKE_CXX_STANDARD 17)
#set(CMAKE_WINDOWS_EXPORT_ALL_SYMBOLS ON)
#set(CMAKE_MSVC_RUNTIME_LIBRARY)

set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR})
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR})
#set(CMAKE_LIBRARY_OUTPUT_DIRECTORY_RELEASE ${CMAKE_CURRENT_SOURCE_DIR})
#set(CMAKE_RUNTIME_OUTPUT_DIRECTORY_RELEASE ${CMAKE_CURRENT_SOURCE_DIR})

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

set(YAML_BUILD_SHARED_LIBS OFF CACHE BOOL "" FORCE)
set(YAML_CPP_BUILD_TOOLS OFF CACHE BOOL "" FORCE)
set(YAML_CPP_INSTALL OFF CACHE BOOL "" FORCE)

set(FT_WITH_BZIP2 OFF CACHE BOOL "" FORCE)
set(FT_WITH_HARFBUZZ OFF CACHE BOOL "" FORCE)
set(FT_WITH_PNG OFF CACHE BOOL "" FORCE)
set(FT_WITH_ZLIB OFF CACHE BOOL "" FORCE)

set(RECASTNAVIGATION_DEMO OFF CACHE BOOL "" FORCE)
set(RECASTNAVIGATION_TESTS OFF CACHE BOOL "" FORCE)
set(RECASTNAVIGATION_EXAMPLES OFF CACHE BOOL "" FORCE)
set(RECASTNAVIGATION_DEMO OFF CACHE BOOL "" FORCE)

#set(TF_BUILD_TESTS OFF CACHE BOOL "" FORCE)
#set(TF_BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)

set(SOL2_ENABLE_INSTALL OFF CACHE BOOL "" FORCE)

add_subdirectory(Engine/vendor/lua-5.1.5)
add_subdirectory(Engine/vendor/sol2)
add_subdirectory(Engine/vendor/glfw)
add_subdirectory(Engine/vendor/assimp)
add_subdirectory(Engine/vendor/bullet3)
add_subdirectory(Engine/vendor/soloud)
add_subdirectory(Engine/vendor/freetype)
add_subdirectory(Engine/vendor/recastnavigation)
#add_subdirectory(engine/vendor/taskflow)

option(ENGINE_SHARED_LIBRARY "" TRUE)

set(DEFAULT_PROJECT_PATH "${CMAKE_CURRENT_SOURCE_DIR}/Sandbox" CACHE STRING "")

MESSAGE("DEFAULT_PROJECT_PATH? " ${DEFAULT_PROJECT_PATH})

project(Engine)

if(CMAKE_CXX_COMPILER_ID MATCHES "Clang")
    #using Clang
else() 
    add_compile_options(/bigobj)
    add_compile_options(/MP)
endif()

file(GLOB_RECURSE ENGINE_SOURCE_FILES 
    Engine/vendor/tinyfiledialogs/*.c
    Engine/vendor/glad/*.c 
    Engine/vendor/stb/*.c 
    Engine/vendor/cgltf/*.c
    #engine/vendor/glm/*.cpp
    Engine/vendor/imgui/*.cpp 
    Engine/vendor/ImGuizmo/*.cpp 
    Engine/vendor/entt/*.cpp
    Engine/src/*.cpp
)
if(ENGINE_SHARED_LIBRARY)
add_library(Engine SHARED ${ENGINE_SOURCE_FILES})
else()
add_library(Engine STATIC ${ENGINE_SOURCE_FILES})
endif()
#target_precompile_headers(engine INTERFACE engine/src/OD/pch.h)
target_compile_definitions(Engine PUBLIC OD_BUILD_DLL)
target_include_directories(Engine PUBLIC 
    Engine/vendor/
    Engine/vendor/lua-5.1.5/src/
    Engine/vendor/sol2/include/
    Engine/vendor/cgltf/
    Engine/vendor/cereal/include/
    Engine/vendor/magic_enum/include/
    Engine/vendor/imgui/
    Engine/vendor/ImGuizmo/
    Engine/vendor/glfw/include/
    Engine/vendor/assimp/include/
    Engine/vendor/bullet3/src
    Engine/vendor/entt/
    Engine/vendor/soloud/include/
    Engine/vendor/freetype/include/
    Engine/vendor/recastnavigation/Recast/Include/
    Engine/vendor/recastnavigation/Detour/Include/
    Engine/vendor/recastnavigation/DetourCrowd/Include/
    Engine/vendor/recastnavigation/Detour/Include/
    Engine/vendor/taskflow/
    Engine/src/
)
target_link_libraries(Engine PUBLIC 
    LUA
    sol2
    glfw 
    assimp
    soloud
    freetype
    DebugUtils Detour DetourCrowd DetourTileCache Recast
    Bullet3Common Bullet3Collision Bullet3Dynamics BulletDynamics BulletCollision BulletSoftBody LinearMath
)

#----------------Sandbox------------------
project(Editor)

file(GLOB_RECURSE EDITOR_SOURCE_FILES Editor/src/*.cpp)
add_executable(Editor ${EDITOR_SOURCE_FILES})
target_include_directories(Editor PUBLIC Engine/src/)
target_include_directories(Editor PUBLIC Editor/src/)
target_link_libraries(Editor Engine)
target_compile_definitions(Editor PUBLIC RESOURCES_PATH="${DEFAULT_PROJECT_PATH}")

#----------------Sandbox------------------
option(BUILD_SANDBOX "BUILD_SANDBOX" TRUE)

if(BUILD_SANDBOX)

project(Sandbox)

file(GLOB_RECURSE SANDBOX_SOURCE_FILES Sandbox/src/*.cpp)
add_executable(Sandbox ${SANDBOX_SOURCE_FILES})
target_include_directories(Sandbox PUBLIC Engine/src/)
target_include_directories(Sandbox PUBLIC Sandbox/src/)
target_link_libraries(Sandbox Engine)
target_compile_definitions(Sandbox PUBLIC RESOURCES_PATH="${DEFAULT_PROJECT_PATH}")

#----------------Dynamic module------------------

project(DynamicModule)
file(GLOB_RECURSE DYNAMIC_MODULE_SOURCE_FILES DynamicModule/src/*.cpp)
add_library(DynamicModule SHARED ${DYNAMIC_MODULE_SOURCE_FILES})
#target_include_directories(dynamic_module PUBLIC engine/vendor/)
#target_include_directories(dynamic_module PUBLIC engine/vendor/cereal/include/)
#target_include_directories(dynamic_module PUBLIC engine/vendor/magic_enum/include/)
#target_include_directories(DynamicModule PUBLIC Engine/vendor/taskflow/)
target_include_directories(DynamicModule PUBLIC Engine/src/)
#target_include_directories(dynamic_module PUBLIC game/src/)
target_link_libraries(DynamicModule Engine)
#target_compile_definitions(DynamicModule PUBLIC RESOURCES_PATH="${CMAKE_CURRENT_SOURCE_DIR}")

endif()

# Enable Disable Tests
if(OFF) 

enable_testing()

project(Tests)

add_subdirectory(Tests/googletest EXCLUDE_FROM_ALL)
add_library(GTest::GTest INTERFACE IMPORTED)
target_link_libraries(GTest::GTest INTERFACE gtest_main)

file(GLOB_RECURSE TESTS_SOURCE_FILES Tests/src/*.cpp)
add_executable(Tests ${TESTS_SOURCE_FILES})

target_include_directories(Tests PUBLIC Engine/src/)
target_link_libraries(Tests GTest::GTest Engine)

add_test(tests_gtests Tests)

endif()