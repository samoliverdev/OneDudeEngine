cmake_minimum_required(VERSION 3.16)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF) # disables MSVC/GNU extensions
#set(CMAKE_COMPILE_WARNING_AS_ERROR OFF)

option(OD_ENGINE_SHARED_LIBRARY "Engine Shared Library" ON)
option(OD_USE_ASSIMP "Use Assimp support" ON)
option(OD_BUILD_SANDBOX "BUILD_SANDBOX" ON)
option(OD_BUILD_EDITOR "BUILD_EDITOR" ON)
option(OD_BUILD_DYNAMIC_MODULE "BUILD_DYNAMIC_MODULE" ON)
option(OD_SUPPORT_OPENGL "SUPPORT_OPENGL" ON)
option(OD_SUPPORT_WEBGPU "SUPPORT_WEBGPU" OFF)

option(OD_USE_MSVC_RUNTIME_LIBRARY_DLL "Use MSVC Runtime Library Dll" ON)
option(OD_USE_SANITIZE_ADDRESS "Use Sanitize Address" OFF)

option(OD_ADD_RUNTIME_TO_BIN "ADD_RUNTIME_TO_BIN" ON)

#----------------s------------------

if(MSVC)
    message(STATUS "----------Using MSVC compiler----------")
endif()

message(STATUS "#-------------USE_ASSIMP is: ${OD_USE_ASSIMP}")


#if(MSVC)
    cmake_policy(SET CMP0091 NEW)
    if(OD_USE_MSVC_RUNTIME_LIBRARY_DLL)
        set(CMAKE_MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>DLL")# CACHE STRING "" FORCE)
    else()
        set(CMAKE_MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>")# CACHE STRING "" FORCE)
    endif()

    #add_compile_options(/O2)
    add_compile_options(/arch:SSE2)
    #add_compile_options(/arch:AVX2)
    #add_compile_options(/Qvec-report:2)

    add_compile_options(/permissive- /Zc:preprocessor /Zc:__cplusplus)

    #add_compile_options(/Bt+)# profiling
    #add_compile_options(/d2cgsummary) # profiling 2

    if(OD_USE_SANITIZE_ADDRESS)
        add_compile_options(/fsanitize=address /Zi)
        add_link_options(/fsanitize=address)
    endif()
    
#endif()

#option(EMSCRIPTEN "EMSCRIPTEN" OFF)

#----------------Engine------------------


if(EMSCRIPTEN)
    message("Using emscripten!")
    #option(ENGINE_SHARED_LIBRARY "" OFF)
    set(OD_ENGINE_SHARED_LIBRARY OFF CACHE BOOL "" FORCE)
    set(OD_BUILD_EDITOR OFF CACHE BOOL "" FORCE)
    set(OD_BUILD_DYNAMIC_MODULE OFF CACHE BOOL "" FORCE)
    set(OD_SUPPORT_WEBGPU OFF CACHE BOOL "" FORCE)
    add_compile_options($<$<COMPILE_LANGUAGE:CXX>:-Wno-nontrivial-memaccess>)
elseif(WIN32)
    message("Using WIN32!")
    #if(MSVC)
    message("Using MSVC!")
    add_compile_options(/WX-)
    add_compile_options(/bigobj)
    add_compile_options(/MP)
    #endif()
elseif(UNIX)
    set(CMAKE_POSITION_INDEPENDENT_CODE ON)
endif()

set(GLFW_BUILD_DOCS OFF CACHE BOOL "" FORCE)
set(GLFW_BUILD_TESTS OFF CACHE BOOL "" FORCE)
set(GLFW_BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)
set(GLFW_INSTALL OFF CACHE BOOL "" FORCE)

set(BUILD_SHARED_LIBS OFF CACHE BOOL "" FORCE)
set(ASSIMP_BUILD_TESTS OFF CACHE BOOL "" FORCE)
set(ASSIMP_INSTALL OFF CACHE BOOL "" FORCE)
set(ASSIMP_BUILD_ALL_EXPORTERS_BY_DEFAULT  OFF CACHE BOOL "" FORCE)
set(ASSIMP_NO_EXPORT  ON CACHE BOOL "" FORCE)
set(ASSIMP_BUILD_DRACO_STATIC  ON CACHE BOOL "" FORCE)
if(OD_USE_MSVC_RUNTIME_LIBRARY_DLL)
    set(USE_STATIC_CRT Off CACHE BOOL "" FORCE)
else()
    set(USE_STATIC_CRT ON CACHE BOOL "" FORCE)
endif()

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

set(SPIRV_REFLECT_EXECUTABLE OFF CACHE BOOL "" FORCE)
set(SPIRV_REFLECT_STATIC_LIB ON CACHE BOOL "" FORCE)

set(ENABLE_INSTALL OFF CACHE BOOL "" FORCE)
if(OD_USE_MSVC_RUNTIME_LIBRARY_DLL)
    set(USE_STATIC_MSVC_RUNTIME_LIBRARY OFF CACHE BOOL "" FORCE)
else()
    set(USE_STATIC_MSVC_RUNTIME_LIBRARY ON CACHE BOOL "" FORCE)
endif()

#add_subdirectory(Engine/ThirdParty/glm-1.0.1) 

if(EMSCRIPTEN)
    message("Using emscripten!2")
    message(${OD_ENGINE_SHARED_LIBRARY})
else()  
    add_subdirectory(Engine/ThirdParty/glfw)
endif()

add_subdirectory(Engine/ThirdParty/lua-5.1.5)
add_subdirectory(Engine/ThirdParty/sol2) 

if(OD_USE_ASSIMP)
    add_subdirectory(Engine/ThirdParty/assimp)
endif()

#add_subdirectory(Engine/ThirdParty/soloud)
add_subdirectory(Engine/ThirdParty/freetype)
add_subdirectory(Engine/ThirdParty/recastnavigation)
add_subdirectory(Engine/ThirdParty/efsw)

add_compile_definitions(JPH_DEBUG_RENDERER)
add_subdirectory(Engine/ThirdParty/JoltPhysics-5.3.0/Build)

set(MSDF_ATLAS_BUILD_STANDALONE OFF CACHE BOOL "" FORCE)
set(MSDF_ATLAS_USE_VCPKG OFF CACHE BOOL "" FORCE)
set(MSDF_ATLAS_MSDFGEN_EXTERNAL OFF CACHE BOOL "" FORCE)
set(MSDF_ATLAS_USE_SKIA OFF CACHE BOOL "" FORCE)
if(OD_USE_MSVC_RUNTIME_LIBRARY_DLL)
    set(MSDF_ATLAS_DYNAMIC_RUNTIME ON CACHE BOOL "" FORCE)
else()
    set(MSDF_ATLAS_DYNAMIC_RUNTIME OFF CACHE BOOL "" FORCE)
endif()
add_subdirectory(Engine/ThirdParty/msdf-atlas-gen)

if(OD_SUPPORT_WEBGPU)
    add_subdirectory(Engine/ThirdParty/glslang)
    add_subdirectory(Engine/ThirdParty/SPIRV-Reflect)
endif()
#add_subdirectory(Engine/ThirdParty/imgui)
#add_subdirectory(Engine/ThirdParty/SPIRV-Cross)
#add_subdirectory(Engine/ThirdParty/EASTL)
#add_subdirectory(engine/ThirdParty/taskflow)

#add_subdirectory(Engine/ThirdParty/tinygltf)

if(OD_SUPPORT_WEBGPU) 
    #FetchContent_Declare(
    #webgpu
    #GIT_REPOSITORY https://github.com/eliemichel/WebGPU-distribution
    #GIT_TAG        main #<branch_name>
    #)
    #FetchContent_MakeAvailable(webgpu)
    add_subdirectory(Engine/ThirdParty/webgpu)
    add_subdirectory(Engine/ThirdParty/glfw3webgpu)
endif()

add_subdirectory(Engine/ThirdParty/unordered_dense-4.8.1)
add_subdirectory(Engine/ThirdParty/tomlplusplus-3.4.0)
add_subdirectory(Engine/ThirdParty/bitsery)

add_subdirectory(Engine/ThirdParty/spdlog-1.17.0)

set(MINIZ_STANDALONE_PROJECT OFF CACHE BOOL "" FORCE)
add_subdirectory(Engine/ThirdParty/miniz)

set(PHYSFS_BUILD_SHARED FALSE CACHE BOOL "" FORCE)
set(PHYSFS_BUILD_TEST FALSE CACHE BOOL "" FORCE)
set(PHYSFS_BUILD_DOCS FALSE CACHE BOOL "" FORCE)
add_subdirectory(Engine/ThirdParty/physfs-release-3.2.0)

#set(CMAKE_MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>")

#set(CMAKE_CXX_STANDARD 17)
set(DEFAULT_PROJECT_PATH "${CMAKE_CURRENT_SOURCE_DIR}/Sandbox/" CACHE STRING "")
MESSAGE("DEFAULT_PROJECT_PATH? " ${DEFAULT_PROJECT_PATH})

#MESSAGE("#---------USE_MSVC_RUNTIME_LIBRARY_DLL? " ${USE_MSVC_RUNTIME_LIBRARY_DLL})

message(STATUS "OD_USE_MSVC_RUNTIME_LIBRARY_DLL = ${OD_USE_MSVC_RUNTIME_LIBRARY_DLL}")
message(STATUS "USE_STATIC_MSVC_RUNTIME_LIBRARY = ${USE_STATIC_MSVC_RUNTIME_LIBRARY}")
message(STATUS "MSDF_ATLAS_DYNAMIC_RUNTIME = ${MSDF_ATLAS_DYNAMIC_RUNTIME}")
message(STATUS "USE_STATIC_CRT = ${USE_STATIC_CRT}")

if(OD_ADD_RUNTIME_TO_BIN)
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
endif()

project(Engine)

file(GLOB_RECURSE ENGINE_SOURCE_FILES 
    Engine/ThirdParty/tinyfiledialogs/*.c
    #Engine/ThirdParty/glad/*.c 
    Engine/ThirdParty/stb/*.c 
    Engine/ThirdParty/cgltf/*.c
    #engine/ThirdParty/glm/*.cpp
    #Engine/ThirdParty/imgui/*.cpp 
    Engine/ThirdParty/ImGuizmo/*.cpp 
    Engine/ThirdParty/entt-3.13.2/src/*.cpp
    Engine/ThirdParty/miniaudio/*.c
    Engine/Source/*.cpp
)
set(ENGINE_SOURCE_FILES ${ENGINE_SOURCE_FILES} Engine/ThirdParty/tinygltf/tiny_gltf.cc)
set(ENGINE_SOURCE_FILES ${ENGINE_SOURCE_FILES} Engine/ThirdParty/tinyobjloader/tiny_obj_loader.cc)
set(ENGINE_SOURCE_FILES ${ENGINE_SOURCE_FILES}
    Engine/ThirdParty/imgui/imgui.cpp
    Engine/ThirdParty/imgui/imgui_draw.cpp
    Engine/ThirdParty/imgui/imgui_demo.cpp
    Engine/ThirdParty/imgui/imgui_tables.cpp
    Engine/ThirdParty/imgui/imgui_widgets.cpp
    Engine/ThirdParty/imgui/backends/imgui_impl_glfw.cpp
)
if(OD_SUPPORT_OPENGL)
set(ENGINE_SOURCE_FILES ${ENGINE_SOURCE_FILES}
    Engine/ThirdParty/imgui/backends/imgui_impl_opengl3.cpp
)
endif()
if(OD_SUPPORT_WEBGPU)
set(ENGINE_SOURCE_FILES ${ENGINE_SOURCE_FILES}
    Engine/ThirdParty/imgui/backends/imgui_impl_wgpu.cpp
)
endif()

if(OD_ENGINE_SHARED_LIBRARY)
    add_library(Engine SHARED ${ENGINE_SOURCE_FILES})
else()
    add_library(Engine STATIC ${ENGINE_SOURCE_FILES})
endif()
#set_target_properties(Engine PROPERTIES UNITY_BUILD ON)#Thi is to compile fast, but has some erro on compile engine
#set_target_properties(Engine PROPERTIES
#    CXX_STANDARD 17
#    CXX_STANDARD_REQUIRED ON
#    CXX_EXTENSIONS OFF
#    COMPILE_WARNING_AS_ERROR ON
#)
target_precompile_headers(Engine 
    PRIVATE $<$<COMPILE_LANGUAGE:CXX>:${CMAKE_CURRENT_SOURCE_DIR}/Engine/Source/OD/pch.h>
    #PRIVATE $<$<COMPILE_LANGUAGE:CXX>:${CMAKE_CURRENT_SOURCE_DIR}/Engine/Source/OD/GlmPch.h>
) #INTERFACE
target_compile_definitions(Engine PUBLIC OD_BUILD_DLL)
target_compile_definitions(Engine PUBLIC OPENGL_SUPPORT)
if(OD_SUPPORT_WEBGPU)
    target_compile_definitions(Engine PUBLIC WEBGPU_SUPPORT)
endif()

target_include_directories(Engine PUBLIC 
    Engine/ThirdParty/
    Engine/ThirdParty/glm-1.0.1/
    Engine/ThirdParty/glad46core/
    Engine/ThirdParty/tinygltf/
    Engine/ThirdParty/tinyobjloader/
    Engine/ThirdParty/lua-5.1.5/src/
    Engine/ThirdParty/sol2/include/
    Engine/ThirdParty/cgltf/
    Engine/ThirdParty/cereal/include/
    Engine/ThirdParty/magic_enum/include/
    Engine/ThirdParty/imgui/
    Engine/ThirdParty/ImGuizmo/
    Engine/ThirdParty/glfw/include/
    Engine/ThirdParty/assimp/include/
    Engine/ThirdParty/JoltPhysics-5.3.0/
    Engine/ThirdParty/entt-3.13.2/src/
    Engine/ThirdParty/soloud/include/
    Engine/ThirdParty/freetype/include/
    Engine/ThirdParty/recastnavigation/Recast/Include/
    Engine/ThirdParty/recastnavigation/Detour/Include/
    Engine/ThirdParty/recastnavigation/DetourCrowd/Include/
    Engine/ThirdParty/recastnavigation/Detour/Include/
    Engine/ThirdParty/taskflow/
    Engine/ThirdParty/SPIRV-Reflect/
    Engine/ThirdParty/efsw/include/
    Engine/ThirdParty/msdf-atlas-gen/msdf-atlas-gen/
    Engine/ThirdParty/unordered_dense-4.8.1/include/
    Engine/ThirdParty/tomlplusplus-3.4.0/include/
    Engine/ThirdParty/bitsery/include/
    Engine/ThirdParty/miniz/
    Engine/ThirdParty/physfs-release-3.2.0/src/
    Engine/ThirdParty/miniaudio/
    #Engine/ThirdParty/spdlog-1.17.0/include/
    #Engine/ThirdParty/tinygltf/
    #Engine/ThirdParty/SPIRV-Cross/
    #Engine/ThirdParty/EASTL/include/
    Engine/Source/
)
#target_include_directories(Engine PUBLIC 
#    Engine/ThirdParty/glm-1.0.1/
#    Engine/ThirdParty/cereal/include/
#    Engine/Source/
#)

target_link_libraries(Engine PUBLIC 
    LUA
    sol2
    glfw 
    #assimp
    #soloud
    freetype
    DebugUtils Detour DetourCrowd DetourTileCache Recast
    Jolt
    efsw
    msdf-atlas-gen
    unordered_dense
    tomlplusplus_tomlplusplus
    Bitsery::bitsery
    miniz
    physfs-static
    #glslang glslang-default-resource-limits
    #spirv-reflect-static
    #spirv-cross-core spirv-cross-c 
    #EASTL
)
target_link_libraries(Engine PUBLIC spdlog::spdlog $<$<BOOL:${MINGW}>:ws2_32>)
#target_link_libraries(Engine PUBLIC spdlog::spdlog_header_only)
if(WIN32)
    target_link_libraries(Engine PRIVATE comctl32)
endif()
if(OD_SUPPORT_WEBGPU)
    target_link_libraries(Engine PRIVATE glslang glslang-default-resource-limits spirv-reflect-static
)
endif()
if(OD_USE_ASSIMP)
    target_link_libraries(Engine PRIVATE assimp)
    target_compile_definitions(Engine PRIVATE USE_ASSIMP)
endif()
#target_link_libraries(Engine PRIVATE tinygltf) 
if(OD_SUPPORT_WEBGPU)
    target_link_libraries(Engine PRIVATE webgpu glfw3webgpu)
    target_copy_webgpu_binaries(Engine)
endif()
if(EMSCRIPTEN)
    target_compile_definitions(Engine PRIVATE -DEMSCRIPTEN=1)
    set_target_properties(Engine PROPERTIES LINK_FLAGS "-s USE_GLFW=3 -s USE_WEBGL2=1 -s FULL_ES3=1")
endif()
#set_target_properties(Engine PROPERTIES MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>")
#if(Clang)
#    target_compile_options(Engine PRIVATE -fsanitize=address)
#    target_link_options(Engine PRIVATE -fsanitize=address)
#endif()
#set_property(TARGET Engine PROPERTY MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>DLL")

#----------------Editor------------------
if(OD_BUILD_EDITOR)
    project(Editor)

    file(GLOB_RECURSE EDITOR_SOURCE_FILES Editor/Source/*.cpp)
    add_executable(Editor ${EDITOR_SOURCE_FILES})
    target_precompile_headers(Editor PRIVATE $<$<COMPILE_LANGUAGE:CXX>:${CMAKE_CURRENT_SOURCE_DIR}/Engine/Source/OD/pch.h>)
    target_include_directories(Editor PRIVATE Engine/Source/)
    target_include_directories(Editor PRIVATE Editor/Source/)
    target_link_libraries(Editor Engine)
    target_compile_definitions(Editor PRIVATE RESOURCES_PATH="${CMAKE_CURRENT_SOURCE_DIR}/Editor/")
    target_compile_definitions(Editor PRIVATE ENGINE_PATH="${CMAKE_CURRENT_SOURCE_DIR}/")
    add_custom_command(
        TARGET Editor POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_directory_if_different ${CMAKE_CURRENT_SOURCE_DIR}/Engine/Content/ ${CMAKE_CURRENT_SOURCE_DIR}/Editor/Content
    )
    add_custom_command(
        TARGET Editor POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_directory_if_different ${CMAKE_CURRENT_SOURCE_DIR}/Engine/Content ${CMAKE_CURRENT_SOURCE_DIR}/Editor/Content/ProjectTemplate/Content
    )
    #set_target_properties(Editor PROPERTIES MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>DLL")
endif()

#----------------Sandbox------------------
if(OD_BUILD_SANDBOX)
    project(Sandbox)
    file(GLOB_RECURSE SANDBOX_SOURCE_FILES Sandbox/Source/*.cpp)
    add_executable(Sandbox ${SANDBOX_SOURCE_FILES})
    target_precompile_headers(Sandbox PRIVATE $<$<COMPILE_LANGUAGE:CXX>:${CMAKE_CURRENT_SOURCE_DIR}/Engine/Source/OD/pch.h>)
    target_include_directories(Sandbox PRIVATE Engine/Source/)
    target_include_directories(Sandbox PRIVATE Sandbox/Source/)
    target_link_libraries(Sandbox Engine)
    target_compile_definitions(Sandbox PRIVATE RESOURCES_PATH="${DEFAULT_PROJECT_PATH}")
    add_custom_command(
        TARGET Sandbox POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_directory_if_different ${CMAKE_CURRENT_SOURCE_DIR}/Engine/Content/ ${CMAKE_CURRENT_SOURCE_DIR}/Sandbox/Content
    )
    if(EMSCRIPTEN)
        set_target_properties(Sandbox PROPERTIES SUFFIX ".html")
        target_compile_definitions(Sandbox PRIVATE -DEMSCRIPTEN=1)
        set_target_properties(Sandbox PROPERTIES LINK_FLAGS "-s USE_GLFW=3 -s USE_WEBGL2=1 -s FULL_ES3=1 -s ASYNCIFY -s -s EXPORTED_FUNCTIONS=_malloc,_free,setValue,_main -s EXPORTED_RUNTIME_METHODS=cwrap,ccall -s TOTAL_STACK=64MB -s INITIAL_MEMORY=128MB -s ALLOW_MEMORY_GROWTH -s ASSERTIONS -s NO_DISABLE_EXCEPTION_CATCHING --preload-file ${DEFAULT_PROJECT_PATH}Content@./")
        #set_target_properties(Sandbox PROPERTIES LINK_DEPENDS ${DEFAULT_PROJECT_PATH}Content/Engine/Shaders/Font.glsl)
        set_target_properties(Sandbox PROPERTIES LINK_DEPENDS ${DEFAULT_PROJECT_PATH}Content/Engine/Fonts/fa-solid-900.ttf)
        #set_target_properties(Sandbox PROPERTIES LINK_FLAGS "--preload-file ${DEFAULT_PROJECT_PATH}Content/Engine/Shaders/Font.glsl@./Font.glsl")
        #set_target_properties(Sandbox PROPERTIES LINK_FLAGS "--preload-file ${DEFAULT_PROJECT_PATH}Content/Engine/Fonts/fa-solid-900.ttf@./fa-solid-900.ttf")
    endif()
    #set_target_properties(Sandbox PROPERTIES MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>DLL")
endif()

#----------------Dynamic module------------------
if(OD_BUILD_DYNAMIC_MODULE)
    project(DynamicModule)
    file(GLOB_RECURSE DYNAMIC_MODULE_SOURCE_FILES DynamicModule/Source/*.cpp)
    add_library(DynamicModule SHARED ${DYNAMIC_MODULE_SOURCE_FILES})
    target_precompile_headers(DynamicModule PRIVATE $<$<COMPILE_LANGUAGE:CXX>:${CMAKE_CURRENT_SOURCE_DIR}/Engine/Source/OD/pch.h>)
    #target_include_directories(dynamic_module PUBLIC engine/vendor/)
    #target_include_directories(dynamic_module PUBLIC engine/vendor/cereal/include/)
    #target_include_directories(dynamic_module PUBLIC engine/vendor/magic_enum/include/)
    #target_include_directories(DynamicModule PUBLIC Engine/vendor/taskflow/)
    target_include_directories(DynamicModule PRIVATE Engine/Source/)
    #target_include_directories(dynamic_module PUBLIC game/src/)
    target_link_libraries(DynamicModule Engine)
    #target_compile_definitions(DynamicModule PUBLIC RESOURCES_PATH="${CMAKE_CURRENT_SOURCE_DIR}")
    #set_target_properties(DynamicModule PROPERTIES MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>DLL")
endif()
