#pragma once
#include "OD/Defines.h"
#include "OD/Core/Math.h"
#include <vector>

namespace OD{

class RenderContext;
class Mesh;
class Material;

struct DrawMeshCmd{
    Matrix4 trans;
    Mesh* mesh;
    Material* mat;
};

union CommandData{
    DrawMeshCmd drawMesh;
};

struct Command{
    void (*Execute)(const void*);
    alignas(CommandData) uint8_t data[sizeof(CommandData)];
};

class OD_API CommandBuffer{
public:
    void DrawMesh(Mesh& mesh, Material& mat, Matrix4 modelMatrix); 
    void Execute();
    void Clear();
private:
    std::vector<Command> commands;
};

}