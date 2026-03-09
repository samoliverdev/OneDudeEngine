#include "CommandBuffer.h"
#include "OD/Graphics/Graphics.h"

namespace OD{

void CommandBuffer::DrawMesh(Mesh& mesh, Material& mat, Matrix4 modelMatrix){
    static_assert(sizeof(DrawMeshCmd) <= sizeof(CommandData));

    Command cmd;
    cmd.Execute = [](const void* data){
        auto& c = *(const DrawMeshCmd*)data;
        Graphics::DrawMesh(*c.mesh, *c.mat, c.trans);
    };
    new (cmd.data) DrawMeshCmd{modelMatrix, &mesh, &mat};
    commands.push_back(cmd);
} 

void CommandBuffer::Execute(){
    for(auto& cmd: commands){
        cmd.Execute(cmd.data);
    }
}

void CommandBuffer::Clear(){
    commands.clear();
}

}