#pragma once
#include <OD/OD.h>

using namespace OD;

enum class FaceDirections{
    Front = 0, 
    Back,
    Left,
    Right, 
    Up, 
    Down
};

inline static const IVector3 ChunkVoxelNeighbors[6] = {
    {0, 0, -1},
    {0, 0, 1},
    {-1, 0, 0},
    {1, 0, 0},
    {0, 1, 0},
    {0, -1, 0}
};

inline static const Vector3 ChunkVoxelNormals[6] = {
    {0, 0, 1},
    {0, 0, -1},
    {-1, 0, 0},
    {1, 0, 0},
    {0, 1, 0},
    {0, -1, 0}
};

/*inline static const Vector3 CubeVertexs[8] = {
    Vector3(-0.5, 0.5, 0.5),        // 0
    Vector3(0.5, 0.5, 0.5),         // 1
    Vector3(-0.5, -0.5, 0.5),       // 2
    Vector3(0.5, -0.5, 0.5),        // 3

    Vector3(-0.5, 0.5, -0.5),       // 4
    Vector3(0.5, 0.5, -0.5),        // 5    
    Vector3(-0.5, -0.5, -0.5),      // 6
    Vector3(0.5, -0.5, -0.5)        // 7
};*/

inline static const Vector3 CubeVertexs[8] = {
    Vector3(0, 1, 0),        // 0
    Vector3(1, 1, 0),         // 1
    Vector3(0, 0, 0),       // 2
    Vector3(1, 0, 0),        // 3

    Vector3(0, 1, -1),       // 4
    Vector3(1, 1, -1),        // 5    
    Vector3(0, 0, -1),      // 6
    Vector3(1, 0, -1)        // 7
};

inline static const Vector3 FaceUvs[6] = {
    Vector3(0, 0, 0),
    Vector3(1, 1, 0),
    Vector3(0, 1, 0), 
    
    Vector3(0, 0, 0),
    Vector3(1, 0, 0),
    Vector3(1, 1, 0),
};

struct FaceTriangles{
    int tri[6];
};

inline static const FaceTriangles FacesTriangles[6] = {
    {2, 1, 0, 2, 3, 1},
    {7, 4, 5, 7, 6, 4},
    {6, 0, 4, 6, 2, 0},
    {3, 5, 1, 3, 7, 5},
    {0, 5, 4, 0, 1, 5},
    {6, 3, 2, 6, 7, 3}
};
