cmake_minimum_required(VERSION 3.16)

option(ENGINE_SHARED_LIBRARY "" TRUE)
option(BUILD_SANDBOX "BUILD_SANDBOX" TRUE)
option(BUILD_EDITOR "BUILD_EDITOR" TRUE)
option(BUILD_DYNAMIC_MODULE "BUILD_DYNAMIC_MODULE" TRUE)
option(UseWebGPU "UseWebGPU" TRUE)
#option(EMSCRIPTEN "EMSCRIPTEN" OFF)

#----------------Engine------------------
set(CMAKE_CXX_STANDARD 17)

if(EMSCRIPTEN)
    message("Using emscripten!")
    #option(ENGINE_SHARED_LIBRARY "" OFF)
    set(ENGINE_SHARED_LIBRARY OFF CACHE BOOL "" FORCE)
    set(BUILD_EDITOR OFF CACHE BOOL "" FORCE)
    set(BUILD_DYNAMIC_MODULE OFF CACHE BOOL "" FORCE)
    set(UseWebGPU OFF CACHE BOOL "" FORCE)
    add_compile_options($<$<COMPILE_LANGUAGE:CXX>:-Wno-nontrivial-memaccess>)
elseif(WIN32)
    set(CMAKE_MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>")
    add_compile_options(/bigobj)
    add_compile_options(/MP)
    #add_compile_options(/arch:AVX2)
elseif(UNIX)
    set(CMAKE_POSITION_INDEPENDENT_CODE ON)
endif()

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

#set(TF_BUILD_TESTS OFF CACHE BOOL "" FORCE)
#set(TF_BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)

set(SOL2_ENABLE_INSTALL OFF CACHE BOOL "" FORCE)

set(ENABLE_OPT OFF CACHE BOOL "" FORCE)

if(EMSCRIPTEN)
    message("Using emscripten!2")
    message(${ENGINE_SHARED_LIBRARY})
else()  
    add_subdirectory(Engine/ThirdParty/glfw)
endif()

add_subdirectory(Engine/ThirdParty/lua-5.1.5)
add_subdirectory(Engine/ThirdParty/sol2)
add_subdirectory(Engine/ThirdParty/assimp)
add_subdirectory(Engine/ThirdParty/bullet3)
add_subdirectory(Engine/ThirdParty/soloud)
add_subdirectory(Engine/ThirdParty/freetype)
add_subdirectory(Engine/ThirdParty/recastnavigation)
add_subdirectory(Engine/ThirdParty/glslang)
#add_subdirectory(Engine/ThirdParty/SPIRV-Cross)
#add_subdirectory(Engine/ThirdParty/EASTL)
#add_subdirectory(engine/ThirdParty/taskflow)

if(UseWebGPU) 
    #FetchContent_Declare(
    #webgpu
    #GIT_REPOSITORY https://github.com/eliemichel/WebGPU-distribution
    #GIT_TAG        main #<branch_name>
    #)
    #FetchContent_MakeAvailable(webgpu)
    add_subdirectory(Engine/ThirdParty/webgpu)
    add_subdirectory(Engine/ThirdParty/glfw3webgpu)
endif()

#set(CMAKE_CXX_STANDARD 17)
set(DEFAULT_PROJECT_PATH "${CMAKE_CURRENT_SOURCE_DIR}/Sandbox/" CACHE STRING "")
MESSAGE("DEFAULT_PROJECT_PATH? " ${DEFAULT_PROJECT_PATH})

set(CMAKE_DEBUG_POSTFIX "")
set(CMAKE_RELEASE_POSTFIX "")
set(CMAKE_RELWITHDEBINFO_POSTFIX "")
set(CMAKE_MINSIZEREL_POSTFIX "")
set(RUN_TREE ${CMAKE_CURRENT_SOURCE_DIR}/Bin)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${RUN_TREE})
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY_DEBUG ${RUN_TREE})
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY_RELEASE ${RUN_TREE})
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY_RELWITHDEBINFO ${RUN_TREE})
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY_MINSIZEREL ${RUN_TREE})
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${RUN_TREE})
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY_DEBUG ${RUN_TREE})
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY_RELEASE ${RUN_TREE})
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY_RELWITHDEBINFO ${RUN_TREE})
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY_MINSIZEREL ${RUN_TREE})
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${RUN_TREE})
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY_DEBUG ${RUN_TREE})
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY_RELEASE ${RUN_TREE})
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY_RELWITHDEBINFO ${RUN_TREE})
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY_MINSIZEREL ${RUN_TREE})

project(Engine)

file(GLOB_RECURSE ENGINE_SOURCE_FILES 
    Engine/ThirdParty/tinyfiledialogs/*.c
    #Engine/ThirdParty/glad/*.c 
    Engine/ThirdParty/stb/*.c 
    Engine/ThirdParty/cgltf/*.c
    #engine/ThirdParty/glm/*.cpp
    
    Engine/ThirdParty/imgui/*.cpp 

    Engine/ThirdParty/ImGuizmo/*.cpp 
    Engine/ThirdParty/entt/*.cpp
    Engine/Source/*.cpp
)
if(ENGINE_SHARED_LIBRARY)
add_library(Engine SHARED ${ENGINE_SOURCE_FILES})
else()
add_library(Engine STATIC ${ENGINE_SOURCE_FILES})
endif()
#set_target_properties(Engine PROPERTIES
#    CXX_STANDARD 17
#    CXX_STANDARD_REQUIRED ON
#    CXX_EXTENSIONS OFF
#    COMPILE_WARNING_AS_ERROR ON
#)
target_precompile_headers(Engine INTERFACE Engine/Source/OD/pch.h)
target_compile_definitions(Engine PUBLIC OD_BUILD_DLL)
target_compile_definitions(Engine PUBLIC OPENGL_SUPPORT)
if(UseWebGPU)
    target_compile_definitions(Engine PUBLIC WEBGPU_SUPPORT)
endif()
target_include_directories(Engine PUBLIC 
    Engine/ThirdParty/
    Engine/ThirdParty/lua-5.1.5/src/
    Engine/ThirdParty/sol2/include/
    Engine/ThirdParty/cgltf/
    Engine/ThirdParty/cereal/include/
    Engine/ThirdParty/magic_enum/include/
    Engine/ThirdParty/imgui/
    Engine/ThirdParty/ImGuizmo/
    Engine/ThirdParty/glfw/include/
    Engine/ThirdParty/assimp/include/
    Engine/ThirdParty/bullet3/src
    Engine/ThirdParty/entt/
    Engine/ThirdParty/soloud/include/
    Engine/ThirdParty/freetype/include/
    Engine/ThirdParty/recastnavigation/Recast/Include/
    Engine/ThirdParty/recastnavigation/Detour/Include/
    Engine/ThirdParty/recastnavigation/DetourCrowd/Include/
    Engine/ThirdParty/recastnavigation/Detour/Include/
    Engine/ThirdParty/taskflow/
    #Engine/ThirdParty/SPIRV-Cross/
    #Engine/ThirdParty/EASTL/include/
    Engine/Source/
)
target_link_libraries(Engine PUBLIC 
    LUA
    sol2
    glfw 
    assimp
    soloud
    freetype
    DebugUtils Detour DetourCrowd DetourTileCache Recast
    Bullet3Common Bullet3Collision Bullet3Dynamics BulletDynamics BulletSoftBody BulletCollision LinearMath
    glslang glslang-default-resource-limits
    #spirv-cross-core spirv-cross-c 
    #EASTL
)
if(UseWebGPU)
    target_link_libraries(Engine PUBLIC webgpu glfw3webgpu)
    target_copy_webgpu_binaries(Engine)
endif()
if(EMSCRIPTEN)
    target_compile_definitions(Engine PUBLIC -DEMSCRIPTEN=1)
    set_target_properties(Engine PROPERTIES LINK_FLAGS "-s USE_GLFW=3 -s USE_WEBGL2=1 -s FULL_ES3=1")
endif()

#----------------Editor------------------
if(BUILD_EDITOR)
project(Editor)

file(GLOB_RECURSE EDITOR_SOURCE_FILES Editor/Source/*.cpp)
add_executable(Editor ${EDITOR_SOURCE_FILES})
target_include_directories(Editor PUBLIC Engine/Source/)
target_include_directories(Editor PUBLIC Editor/Source/)
target_link_libraries(Editor Engine)
target_compile_definitions(Editor PUBLIC RESOURCES_PATH="${CMAKE_CURRENT_SOURCE_DIR}/Editor/")
target_compile_definitions(Editor PUBLIC ENGINE_PATH="${CMAKE_CURRENT_SOURCE_DIR}/")
add_custom_command(
    TARGET Editor POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_directory_if_different ${CMAKE_CURRENT_SOURCE_DIR}/Engine/Content/ ${CMAKE_CURRENT_SOURCE_DIR}/Editor/Content
)
add_custom_command(
    TARGET Editor POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_directory_if_different ${CMAKE_CURRENT_SOURCE_DIR}/Engine/Content ${CMAKE_CURRENT_SOURCE_DIR}/Editor/Content/ProjectTemplate/Content
)
endif()

#----------------Sandbox------------------


if(BUILD_SANDBOX)

project(Sandbox)
file(GLOB_RECURSE SANDBOX_SOURCE_FILES Sandbox/Source/*.cpp)
add_executable(Sandbox ${SANDBOX_SOURCE_FILES})
target_include_directories(Sandbox PUBLIC Engine/Source/)
target_include_directories(Sandbox PUBLIC Sandbox/Source/)
target_link_libraries(Sandbox Engine)
target_compile_definitions(Sandbox PUBLIC RESOURCES_PATH="${DEFAULT_PROJECT_PATH}")
add_custom_command(
    TARGET Sandbox POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_directory_if_different ${CMAKE_CURRENT_SOURCE_DIR}/Engine/Content/ ${CMAKE_CURRENT_SOURCE_DIR}/Sandbox/Content
)
if(EMSCRIPTEN)
    set_target_properties(Sandbox PROPERTIES SUFFIX ".html")
    target_compile_definitions(Sandbox PUBLIC -DEMSCRIPTEN=1)
    set_target_properties(Sandbox PROPERTIES LINK_FLAGS "-s USE_GLFW=3 -s USE_WEBGL2=1 -s FULL_ES3=1 -s ASYNCIFY -s -s EXPORTED_FUNCTIONS=_malloc,_free,setValue,_main -s EXPORTED_RUNTIME_METHODS=cwrap,ccall -s TOTAL_STACK=64MB -s INITIAL_MEMORY=128MB -s ALLOW_MEMORY_GROWTH -s ASSERTIONS -s NO_DISABLE_EXCEPTION_CATCHING --preload-file ${DEFAULT_PROJECT_PATH}Content@./")
    #set_target_properties(Sandbox PROPERTIES LINK_DEPENDS ${DEFAULT_PROJECT_PATH}Content/Engine/Shaders/Font.glsl)
    set_target_properties(Sandbox PROPERTIES LINK_DEPENDS ${DEFAULT_PROJECT_PATH}Content/Engine/Fonts/fa-solid-900.ttf)
    #set_target_properties(Sandbox PROPERTIES LINK_FLAGS "--preload-file ${DEFAULT_PROJECT_PATH}Content/Engine/Shaders/Font.glsl@./Font.glsl")
    #set_target_properties(Sandbox PROPERTIES LINK_FLAGS "--preload-file ${DEFAULT_PROJECT_PATH}Content/Engine/Fonts/fa-solid-900.ttf@./fa-solid-900.ttf")
endif()

endif()

#----------------Dynamic module------------------

if(BUILD_DYNAMIC_MODULE)
project(DynamicModule)
file(GLOB_RECURSE DYNAMIC_MODULE_SOURCE_FILES DynamicModule/Source/*.cpp)
add_library(DynamicModule SHARED ${DYNAMIC_MODULE_SOURCE_FILES})
#target_include_directories(dynamic_module PUBLIC engine/vendor/)
#target_include_directories(dynamic_module PUBLIC engine/vendor/cereal/include/)
#target_include_directories(dynamic_module PUBLIC engine/vendor/magic_enum/include/)
#target_include_directories(DynamicModule PUBLIC Engine/vendor/taskflow/)
target_include_directories(DynamicModule PUBLIC Engine/Source/)
#target_include_directories(dynamic_module PUBLIC game/src/)
target_link_libraries(DynamicModule Engine)
#target_compile_definitions(DynamicModule PUBLIC RESOURCES_PATH="${CMAKE_CURRENT_SOURCE_DIR}")
endif()
