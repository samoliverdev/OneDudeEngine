cmake_minimum_required(VERSION 3.20)

if(NOT DEFINED ENGINE_PATH)
    message(FATAL_ERROR "ENGINE_PATH not defined. Set it to the location of engine.")
elseif(NOT EXISTS "${ENGINE_PATH}")
    message(FATAL_ERROR "Invalid ENGINE_PATH location'${ENGINE_PATH}' does not exist")
endif()

message(STATUS "Engine location: ${ENGINE_PATH}")

set(CMAKE_CXX_STANDARD 17)
set(DEFAULT_PROJECT_PATH "${CMAKE_CURRENT_SOURCE_DIR}")

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

#add_subdirectory(../OneDudeEngine OneDudeEngineBuild)
#sset(ENGINE_PATH ../OneDudeEngine/)

add_link_options(/NOIMPLIB)
add_link_options(/NOEXP)

file(GLOB_RECURSE ENGINE_DLL ${ENGINE_PATH}/*Engine*.dll)
file(GLOB_RECURSE ENGINE_LIB ${ENGINE_PATH}/*Engine*.lib)
#file(GLOB_RECURSE EDITOR_EXE ${ENGINE_PATH}/*Editor*.exe)
#set(ENGINE_DLL ${ENGINE_PATH}/Bin/Engine.dll)
#set(ENGINE_LIB ${ENGINE_PATH}/Bin/Engine.lib)
MESSAGE("ENGINE_DLL = " ${ENGINE_DLL})
MESSAGE("ENGINE_LIB = " ${ENGINE_LIB})
#MESSAGE("EDITOR_EXE = " ${EDITOR_EXE})

#add_subdirectory(${ENGINE_PATH} engine)

#project(Engine)
#add_library(Engine INTERFACE ${ENGINE_DLL})
#add_library(Engine SHARED IMPORTED)
#set_property(TARGET Engine PROPERTY IMPORTED_LOCATION ${ENGINE_DLL})
#set_property(TARGET Engine PROPERTY IMPORTED_IMPLIB ${ENGINE_LIB})

#project(Editor)
#add_executable(Editor IMPORTED GLOBAL)
#set_property(TARGET Editor PROPERTY IMPORTED_LOCATION ${EDITOR_EXE})

project(Game)
file(GLOB_RECURSE GAME_SOURCE_FILES Source/*.cpp)

add_library(Game SHARED ${GAME_SOURCE_FILES})
#target_link_libraries(Game Engine)
target_link_libraries(Game ${ENGINE_LIB})
target_include_directories(Game PUBLIC 
    ${ENGINE_PATH}/Engine/Source/ThirdParty
    ${ENGINE_PATH}/Engine/Source/ThirdParty/lua-5.1.5/src
    ${ENGINE_PATH}/Engine/Source/ThirdParty/sol2/include
    ${ENGINE_PATH}/Engine/Source/ThirdParty/cgltf
    ${ENGINE_PATH}/Engine/Source/ThirdParty/cereal/include
    ${ENGINE_PATH}/Engine/Source/ThirdParty/magic_enum/include
    ${ENGINE_PATH}/Engine/Source/ThirdParty/imgui
    ${ENGINE_PATH}/Engine/Source/ThirdParty/ImGuizmo
    ${ENGINE_PATH}/Engine/Source/ThirdParty/glfw/include
    ${ENGINE_PATH}/Engine/Source/ThirdParty/assimp/include
    ${ENGINE_PATH}/Engine/Source/ThirdParty/bullet3/src
    ${ENGINE_PATH}/Engine/Source/ThirdParty/entt
    ${ENGINE_PATH}/Engine/Source/ThirdParty/soloud/include
    ${ENGINE_PATH}/Engine/Source/ThirdParty/freetype/include
    ${ENGINE_PATH}/Engine/Source/ThirdParty/recastnavigation/Recast/Include
    ${ENGINE_PATH}/Engine/Source/ThirdParty/recastnavigation/Detour/Include
    ${ENGINE_PATH}/Engine/Source/ThirdParty/recastnavigation/DetourCrowd/Include
    ${ENGINE_PATH}/Engine/Source/ThirdParty/recastnavigation/Detour/Include
    ${ENGINE_PATH}/Engine/Source/ThirdParty/taskflow
    ${ENGINE_PATH}/Engine/Source
)
add_custom_command(
    TARGET Game POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_directory_if_different ${ENGINE_PATH}/Engine/Content ${CMAKE_SOURCE_DIR}/Content
)
#set_target_properties(Game PROPERTIES 
#    VS_DEBUGGER_COMMAND "${EDITOR_EXE}"
#    VS_DEBUGGER_COMMAND_ARGUMENTS " ${CMAKE_CURRENT_SOURCE_DIR}"
#)