# Install script for directory: E:/c++/OneDudeEngine_0.2/engine/vendor/assimp/code

# Set the install prefix
if(NOT DEFINED CMAKE_INSTALL_PREFIX)
  set(CMAKE_INSTALL_PREFIX "C:/Program Files (x86)/GLFW")
endif()
string(REGEX REPLACE "/$" "" CMAKE_INSTALL_PREFIX "${CMAKE_INSTALL_PREFIX}")

# Set the install configuration name.
if(NOT DEFINED CMAKE_INSTALL_CONFIG_NAME)
  if(BUILD_TYPE)
    string(REGEX REPLACE "^[^A-Za-z0-9_]+" ""
           CMAKE_INSTALL_CONFIG_NAME "${BUILD_TYPE}")
  else()
    set(CMAKE_INSTALL_CONFIG_NAME "Release")
  endif()
  message(STATUS "Install configuration: \"${CMAKE_INSTALL_CONFIG_NAME}\"")
endif()

# Set the component getting installed.
if(NOT CMAKE_INSTALL_COMPONENT)
  if(COMPONENT)
    message(STATUS "Install component: \"${COMPONENT}\"")
    set(CMAKE_INSTALL_COMPONENT "${COMPONENT}")
  else()
    set(CMAKE_INSTALL_COMPONENT)
  endif()
endif()

# Is this installation the result of a crosscompile?
if(NOT DEFINED CMAKE_CROSSCOMPILING)
  set(CMAKE_CROSSCOMPILING "FALSE")
endif()

if(CMAKE_INSTALL_COMPONENT STREQUAL "libassimp5.2.5-dev" OR NOT CMAKE_INSTALL_COMPONENT)
  if(CMAKE_INSTALL_CONFIG_NAME MATCHES "^([Dd][Ee][Bb][Uu][Gg])$")
    file(INSTALL DESTINATION "${CMAKE_INSTALL_PREFIX}/lib" TYPE STATIC_LIBRARY FILES "E:/c++/OneDudeEngine_0.2/engine/vendor/assimp/lib/Debug/assimp-vc143-mtd.lib")
  elseif(CMAKE_INSTALL_CONFIG_NAME MATCHES "^([Rr][Ee][Ll][Ee][Aa][Ss][Ee])$")
    file(INSTALL DESTINATION "${CMAKE_INSTALL_PREFIX}/lib" TYPE STATIC_LIBRARY FILES "E:/c++/OneDudeEngine_0.2/engine/vendor/assimp/lib/Release/assimp-vc143-mt.lib")
  elseif(CMAKE_INSTALL_CONFIG_NAME MATCHES "^([Mm][Ii][Nn][Ss][Ii][Zz][Ee][Rr][Ee][Ll])$")
    file(INSTALL DESTINATION "${CMAKE_INSTALL_PREFIX}/lib" TYPE STATIC_LIBRARY FILES "E:/c++/OneDudeEngine_0.2/engine/vendor/assimp/lib/MinSizeRel/assimp-vc143-mt.lib")
  elseif(CMAKE_INSTALL_CONFIG_NAME MATCHES "^([Rr][Ee][Ll][Ww][Ii][Tt][Hh][Dd][Ee][Bb][Ii][Nn][Ff][Oo])$")
    file(INSTALL DESTINATION "${CMAKE_INSTALL_PREFIX}/lib" TYPE STATIC_LIBRARY FILES "E:/c++/OneDudeEngine_0.2/engine/vendor/assimp/lib/RelWithDebInfo/assimp-vc143-mt.lib")
  endif()
endif()

if(CMAKE_INSTALL_COMPONENT STREQUAL "assimp-dev" OR NOT CMAKE_INSTALL_COMPONENT)
  file(INSTALL DESTINATION "${CMAKE_INSTALL_PREFIX}/include/assimp" TYPE FILE FILES
    "E:/c++/OneDudeEngine_0.2/engine/vendor/assimp/code/../include/assimp/anim.h"
    "E:/c++/OneDudeEngine_0.2/engine/vendor/assimp/code/../include/assimp/aabb.h"
    "E:/c++/OneDudeEngine_0.2/engine/vendor/assimp/code/../include/assimp/ai_assert.h"
    "E:/c++/OneDudeEngine_0.2/engine/vendor/assimp/code/../include/assimp/camera.h"
    "E:/c++/OneDudeEngine_0.2/engine/vendor/assimp/code/../include/assimp/color4.h"
    "E:/c++/OneDudeEngine_0.2/engine/vendor/assimp/code/../include/assimp/color4.inl"
    "E:/c++/OneDudeEngine_0.2/engine/vendor/assimp/code/../include/assimp/config.h"
    "E:/c++/OneDudeEngine_0.2/engine/vendor/assimp/code/../include/assimp/ColladaMetaData.h"
    "E:/c++/OneDudeEngine_0.2/engine/vendor/assimp/code/../include/assimp/commonMetaData.h"
    "E:/c++/OneDudeEngine_0.2/engine/vendor/assimp/code/../include/assimp/defs.h"
    "E:/c++/OneDudeEngine_0.2/engine/vendor/assimp/code/../include/assimp/cfileio.h"
    "E:/c++/OneDudeEngine_0.2/engine/vendor/assimp/code/../include/assimp/light.h"
    "E:/c++/OneDudeEngine_0.2/engine/vendor/assimp/code/../include/assimp/material.h"
    "E:/c++/OneDudeEngine_0.2/engine/vendor/assimp/code/../include/assimp/material.inl"
    "E:/c++/OneDudeEngine_0.2/engine/vendor/assimp/code/../include/assimp/matrix3x3.h"
    "E:/c++/OneDudeEngine_0.2/engine/vendor/assimp/code/../include/assimp/matrix3x3.inl"
    "E:/c++/OneDudeEngine_0.2/engine/vendor/assimp/code/../include/assimp/matrix4x4.h"
    "E:/c++/OneDudeEngine_0.2/engine/vendor/assimp/code/../include/assimp/matrix4x4.inl"
    "E:/c++/OneDudeEngine_0.2/engine/vendor/assimp/code/../include/assimp/mesh.h"
    "E:/c++/OneDudeEngine_0.2/engine/vendor/assimp/code/../include/assimp/ObjMaterial.h"
    "E:/c++/OneDudeEngine_0.2/engine/vendor/assimp/code/../include/assimp/pbrmaterial.h"
    "E:/c++/OneDudeEngine_0.2/engine/vendor/assimp/code/../include/assimp/GltfMaterial.h"
    "E:/c++/OneDudeEngine_0.2/engine/vendor/assimp/code/../include/assimp/postprocess.h"
    "E:/c++/OneDudeEngine_0.2/engine/vendor/assimp/code/../include/assimp/quaternion.h"
    "E:/c++/OneDudeEngine_0.2/engine/vendor/assimp/code/../include/assimp/quaternion.inl"
    "E:/c++/OneDudeEngine_0.2/engine/vendor/assimp/code/../include/assimp/scene.h"
    "E:/c++/OneDudeEngine_0.2/engine/vendor/assimp/code/../include/assimp/metadata.h"
    "E:/c++/OneDudeEngine_0.2/engine/vendor/assimp/code/../include/assimp/texture.h"
    "E:/c++/OneDudeEngine_0.2/engine/vendor/assimp/code/../include/assimp/types.h"
    "E:/c++/OneDudeEngine_0.2/engine/vendor/assimp/code/../include/assimp/vector2.h"
    "E:/c++/OneDudeEngine_0.2/engine/vendor/assimp/code/../include/assimp/vector2.inl"
    "E:/c++/OneDudeEngine_0.2/engine/vendor/assimp/code/../include/assimp/vector3.h"
    "E:/c++/OneDudeEngine_0.2/engine/vendor/assimp/code/../include/assimp/vector3.inl"
    "E:/c++/OneDudeEngine_0.2/engine/vendor/assimp/code/../include/assimp/version.h"
    "E:/c++/OneDudeEngine_0.2/engine/vendor/assimp/code/../include/assimp/cimport.h"
    "E:/c++/OneDudeEngine_0.2/engine/vendor/assimp/code/../include/assimp/AssertHandler.h"
    "E:/c++/OneDudeEngine_0.2/engine/vendor/assimp/code/../include/assimp/importerdesc.h"
    "E:/c++/OneDudeEngine_0.2/engine/vendor/assimp/code/../include/assimp/Importer.hpp"
    "E:/c++/OneDudeEngine_0.2/engine/vendor/assimp/code/../include/assimp/DefaultLogger.hpp"
    "E:/c++/OneDudeEngine_0.2/engine/vendor/assimp/code/../include/assimp/ProgressHandler.hpp"
    "E:/c++/OneDudeEngine_0.2/engine/vendor/assimp/code/../include/assimp/IOStream.hpp"
    "E:/c++/OneDudeEngine_0.2/engine/vendor/assimp/code/../include/assimp/IOSystem.hpp"
    "E:/c++/OneDudeEngine_0.2/engine/vendor/assimp/code/../include/assimp/Logger.hpp"
    "E:/c++/OneDudeEngine_0.2/engine/vendor/assimp/code/../include/assimp/LogStream.hpp"
    "E:/c++/OneDudeEngine_0.2/engine/vendor/assimp/code/../include/assimp/NullLogger.hpp"
    "E:/c++/OneDudeEngine_0.2/engine/vendor/assimp/code/../include/assimp/cexport.h"
    "E:/c++/OneDudeEngine_0.2/engine/vendor/assimp/code/../include/assimp/Exporter.hpp"
    "E:/c++/OneDudeEngine_0.2/engine/vendor/assimp/code/../include/assimp/DefaultIOStream.h"
    "E:/c++/OneDudeEngine_0.2/engine/vendor/assimp/code/../include/assimp/DefaultIOSystem.h"
    "E:/c++/OneDudeEngine_0.2/engine/vendor/assimp/code/../include/assimp/ZipArchiveIOSystem.h"
    "E:/c++/OneDudeEngine_0.2/engine/vendor/assimp/code/../include/assimp/SceneCombiner.h"
    "E:/c++/OneDudeEngine_0.2/engine/vendor/assimp/code/../include/assimp/fast_atof.h"
    "E:/c++/OneDudeEngine_0.2/engine/vendor/assimp/code/../include/assimp/qnan.h"
    "E:/c++/OneDudeEngine_0.2/engine/vendor/assimp/code/../include/assimp/BaseImporter.h"
    "E:/c++/OneDudeEngine_0.2/engine/vendor/assimp/code/../include/assimp/Hash.h"
    "E:/c++/OneDudeEngine_0.2/engine/vendor/assimp/code/../include/assimp/MemoryIOWrapper.h"
    "E:/c++/OneDudeEngine_0.2/engine/vendor/assimp/code/../include/assimp/ParsingUtils.h"
    "E:/c++/OneDudeEngine_0.2/engine/vendor/assimp/code/../include/assimp/StreamReader.h"
    "E:/c++/OneDudeEngine_0.2/engine/vendor/assimp/code/../include/assimp/StreamWriter.h"
    "E:/c++/OneDudeEngine_0.2/engine/vendor/assimp/code/../include/assimp/StringComparison.h"
    "E:/c++/OneDudeEngine_0.2/engine/vendor/assimp/code/../include/assimp/StringUtils.h"
    "E:/c++/OneDudeEngine_0.2/engine/vendor/assimp/code/../include/assimp/SGSpatialSort.h"
    "E:/c++/OneDudeEngine_0.2/engine/vendor/assimp/code/../include/assimp/GenericProperty.h"
    "E:/c++/OneDudeEngine_0.2/engine/vendor/assimp/code/../include/assimp/SpatialSort.h"
    "E:/c++/OneDudeEngine_0.2/engine/vendor/assimp/code/../include/assimp/SkeletonMeshBuilder.h"
    "E:/c++/OneDudeEngine_0.2/engine/vendor/assimp/code/../include/assimp/SmallVector.h"
    "E:/c++/OneDudeEngine_0.2/engine/vendor/assimp/code/../include/assimp/SmoothingGroups.h"
    "E:/c++/OneDudeEngine_0.2/engine/vendor/assimp/code/../include/assimp/SmoothingGroups.inl"
    "E:/c++/OneDudeEngine_0.2/engine/vendor/assimp/code/../include/assimp/StandardShapes.h"
    "E:/c++/OneDudeEngine_0.2/engine/vendor/assimp/code/../include/assimp/RemoveComments.h"
    "E:/c++/OneDudeEngine_0.2/engine/vendor/assimp/code/../include/assimp/Subdivision.h"
    "E:/c++/OneDudeEngine_0.2/engine/vendor/assimp/code/../include/assimp/Vertex.h"
    "E:/c++/OneDudeEngine_0.2/engine/vendor/assimp/code/../include/assimp/LineSplitter.h"
    "E:/c++/OneDudeEngine_0.2/engine/vendor/assimp/code/../include/assimp/TinyFormatter.h"
    "E:/c++/OneDudeEngine_0.2/engine/vendor/assimp/code/../include/assimp/Profiler.h"
    "E:/c++/OneDudeEngine_0.2/engine/vendor/assimp/code/../include/assimp/LogAux.h"
    "E:/c++/OneDudeEngine_0.2/engine/vendor/assimp/code/../include/assimp/Bitmap.h"
    "E:/c++/OneDudeEngine_0.2/engine/vendor/assimp/code/../include/assimp/XMLTools.h"
    "E:/c++/OneDudeEngine_0.2/engine/vendor/assimp/code/../include/assimp/IOStreamBuffer.h"
    "E:/c++/OneDudeEngine_0.2/engine/vendor/assimp/code/../include/assimp/CreateAnimMesh.h"
    "E:/c++/OneDudeEngine_0.2/engine/vendor/assimp/code/../include/assimp/XmlParser.h"
    "E:/c++/OneDudeEngine_0.2/engine/vendor/assimp/code/../include/assimp/BlobIOSystem.h"
    "E:/c++/OneDudeEngine_0.2/engine/vendor/assimp/code/../include/assimp/MathFunctions.h"
    "E:/c++/OneDudeEngine_0.2/engine/vendor/assimp/code/../include/assimp/Exceptional.h"
    "E:/c++/OneDudeEngine_0.2/engine/vendor/assimp/code/../include/assimp/ByteSwapper.h"
    "E:/c++/OneDudeEngine_0.2/engine/vendor/assimp/code/../include/assimp/Base64.hpp"
    )
endif()

if(CMAKE_INSTALL_COMPONENT STREQUAL "assimp-dev" OR NOT CMAKE_INSTALL_COMPONENT)
  file(INSTALL DESTINATION "${CMAKE_INSTALL_PREFIX}/include/assimp/Compiler" TYPE FILE FILES
    "E:/c++/OneDudeEngine_0.2/engine/vendor/assimp/code/../include/assimp/Compiler/pushpack1.h"
    "E:/c++/OneDudeEngine_0.2/engine/vendor/assimp/code/../include/assimp/Compiler/poppack1.h"
    "E:/c++/OneDudeEngine_0.2/engine/vendor/assimp/code/../include/assimp/Compiler/pstdint.h"
    )
endif()

if(CMAKE_INSTALL_COMPONENT STREQUAL "Unspecified" OR NOT CMAKE_INSTALL_COMPONENT)
  if(CMAKE_INSTALL_CONFIG_NAME MATCHES "^([Dd][Ee][Bb][Uu][Gg])$")
    file(INSTALL DESTINATION "${CMAKE_INSTALL_PREFIX}/lib" TYPE FILE FILES "E:/c++/OneDudeEngine_0.2/engine/vendor/assimp/code/Debug/assimp-vc143-mtd.pdb")
  endif()
endif()

if(CMAKE_INSTALL_COMPONENT STREQUAL "Unspecified" OR NOT CMAKE_INSTALL_COMPONENT)
  if(CMAKE_INSTALL_CONFIG_NAME MATCHES "^([Rr][Ee][Ll][Ww][Ii][Tt][Hh][Dd][Ee][Bb][Ii][Nn][Ff][Oo])$")
    file(INSTALL DESTINATION "${CMAKE_INSTALL_PREFIX}/lib" TYPE FILE FILES "E:/c++/OneDudeEngine_0.2/engine/vendor/assimp/code/RelWithDebInfo/assimp-vc143-mt.pdb")
  endif()
endif()

