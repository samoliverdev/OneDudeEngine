#pragma once
#include "OD/Defines.h"
#include "OD/Base.h"
#include "OD/Graphics/Graphics.h"
#include "OD/Graphics/Camera.h"
#include "OD/Core/Math.h"
#include "RenderPipelineUtils.h"
//#include <EASTL/vector.h>

namespace OD{

class Material;
class Mesh;

struct OD_API DrawCommand{
    Matrix4 trans;
    Material* material; //Ref<Material> material;
    Mesh* meshs;// Ref<Mesh> meshs;
    float distance;

    /*#if EnableExperimentalPerDrawCustomData
    bool useCustomData = false;
    Vector4 customData;
    #endif*/

    PerDrawData perDrawData;

    bool operator<(const DrawCommand& a) const;
};

struct OD_API SkinnedDrawCommand{
    Matrix4 trans;
    Material* material;// Ref<Material> material;
    Mesh* meshs;// Ref<Mesh> meshs;
    std::vector<Matrix4>* posePalette;
    PerDrawData perDrawData;
    float distance;

    bool operator<(const SkinnedDrawCommand& a) const;
};

struct OD_API DrawInstancingCommand{
    Material* material;// Ref<Material> material;
    Mesh* meshs;// Ref<Mesh> meshs;
    std::vector<Matrix4> trans;
    
    bool operator<(const DrawCommand& a) const;
};

struct OD_API MaterialBind2{
    float distance;
    uint32_t materialId;

    bool operator<(const MaterialBind2& a) const;
};

struct OD_API RendererList{
    friend class RenderContext;

    enum class SortType{None, CommonOpaque, CommonTransparent};

    std::string name = "CommandBuffer";
    std::function<void(Material& material)> onUpdateMaterial = nullptr;
    std::function<void(Material& material)> postUpdateMaterial = nullptr;
    SortType sortType;

    void SetOverrideMaterial(Ref<Material> shader);

    void AddDrawCommand(DrawCommand comand, float distance = 0);  
    void AddDrawInstancingCommand(DrawCommand comand);
    void AddSkinnedDrawCommand(SkinnedDrawCommand comand, float distance = 0); 
    
    void Clean();
    void Sort();
    void Submit(bool skipEntityId = true);

private:
    CommandBucket0<DrawCommand> drawCommands;
    CommandBucket3<Material*, DrawCommand> drawCommandsNorSort;

    //CommandBucket1<MaterialBind2, DrawCommand> drawCommands;

    CommandBucket4<Material*, Mesh*, DrawInstancingCommand> drawIntancingCommands;

    CommandBucket1<MaterialBind2, SkinnedDrawCommand> skinnedDrawCommands;
    CommandBucket3<Material*, SkinnedDrawCommand> skinnedDrawCommandsNorSort;

    //NOTE: This not working why Materials can shared the same shader
    /*std::unordered_set<Ref<Material>> drawCommandsMaterials;
    //std::vector<Ref<Material>> drawCommandsMaterials;
    std::set<Ref<Material>> drawIntancingCommandsMaterials;
    std::set<Ref<Material>> skinnedDrawCommandsMaterials;*/

    Ref<Material> overrideMaterial = nullptr;
};

}