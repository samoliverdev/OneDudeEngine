#pragma once
#include "OD/Defines.h"
#include "OD/Base.h"
#include "OD/Graphics/Graphics.h"
#include "OD/Graphics/Camera.h"
#include "OD/Graphics/InstancingBuffer.h"
#include "OD/Core/Math.h"
#include "OD/Core/AlignedAllocator.h"
#include "RenderPipelineUtils.h"
//#include <EASTL/vector.h>

//#define UseExperimentalCommandBucket5

namespace OD{

//TODO: Review/optimaze all struct sizeof later

class Material;
class Mesh;
class SubShader;

struct DrawMultTypeCommand{
    enum class Type{Stand, Skinned, Instancing};

    SubShader* subShader;
    Material* material; //Ref<Material> material;
    Mesh* meshs;// Ref<Mesh> meshs;
    float distance;

    PerDrawData perDrawData;

    union{
        struct {
            Matrix4 standTrans;
        };

        struct {
            Matrix4 skinnedTrans;
            AlignedVector<Matrix4>* skinnedPosePalette;
        };

        struct {
            InstancingBuffer* instancingBuffer;
        };
    };
    
    Type type;

    bool operator<(const DrawMultTypeCommand& a) const;
};

struct OD_API alignas(16) DrawCommand{
    Matrix4 trans;
    PerDrawData perDrawData;
    SubShader* subShader;
    Material* material; //Ref<Material> material;
    Mesh* meshs;// Ref<Mesh> meshs;
    float distance;

    /*#if EnableExperimentalPerDrawCustomData
    bool useCustomData = false;
    Vector4 customData;
    #endif*/

    bool operator<(const DrawCommand& a) const;
};

struct OD_API alignas(16) SkinnedDrawCommand{
    Matrix4 trans;
    PerDrawData perDrawData;
    SubShader* subShader;
    Material* material;// Ref<Material> material;
    Mesh* meshs;// Ref<Mesh> meshs;
    AlignedVector<Matrix4>* posePalette;
    UniformBuffer* skinnedData = nullptr;
    float distance;

    bool operator<(const SkinnedDrawCommand& a) const;
};

//In my quick test i dont have any performace gain
//#define USE_INSTANCING_MATRIX43 //TODO Fix this, is not work with the StaticRendererClusterComponent genInstancingCommands

struct OD_API DrawInstancingCommand{
    //AlignedVector<Matrix4> trans;
    //ReusableVector<Matrix4> trans;

    #ifdef USE_INSTANCING_MATRIX43
    ReusableVector<Matrix4x3> trans;
    #else
    ReusableVector<Matrix4> trans;
    #endif

    ReusableVector<InstancingBuffer*> buffers;

    SubShader* subShader;
    Material* material;
    Mesh* meshs;
    
    bool operator<(const DrawCommand& a) const;
};

struct OD_API DrawInstancingCommand2{
    Ref<InstancingBuffer> buffer;

    #ifdef USE_INSTANCING_MATRIX43
    ReusableVector<Matrix4x3> trans;
    #else
    ReusableVector<Matrix4> trans;
    #endif
    
    SubShader* subShader;
    Material* material;
    Mesh* meshs;
    
    bool operator<(const DrawCommand& a) const;
};

struct OD_API DrawInstancingCommand3{
    InstancingBuffer* buffer;
    SubShader* subShader;
    Material* material;
    Mesh* meshs;
    float distance;
    
    bool operator<(const DrawCommand& a) const;
};

struct OD_API alignas(16) DrawInstancingCommand4{
    Matrix4 trans;
    SubShader* subShader;
    Material* material; //Ref<Material> material;
    Mesh* meshs;// Ref<Mesh> meshs;
    float distance;

    bool operator<(const DrawInstancingCommand4& a) const;
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
    SortType sortType = SortType::None;

    void SetOverrideMaterial(Ref<Material> shader);

    void AddDrawCommand(DrawCommand&& comand, float distance = 0);  
    void AddDrawInstancingCommand(DrawInstancingCommand4&& comand);
    void AddDrawInstancingCommand(DrawInstancingCommand3&& comand);
    void AddSkinnedDrawCommand(SkinnedDrawCommand&& comand, float distance = 0); 
    
    void Clean();
    void Sort();
    void Submit(bool skipEntityId = true);

private:
    CommandBucket0<DrawCommand> drawCommands;
    CommandBucket3<Material*, DrawCommand> drawCommandsNorSort;

    //CommandBucket1<MaterialBind2, DrawCommand> drawCommands;

    #ifdef UseExperimentalCommandBucket5
    CommandBucket5<DrawInstancingCommand> drawIntancingCommands;
    #else
    CommandBucket4<Material*, Mesh*, DrawInstancingCommand> drawIntancingCommands;
    #endif

    //CommandBucket0<DrawInstancingCommand3> drawIntancingCommands2;

    CommandBucket1<MaterialBind2, SkinnedDrawCommand> skinnedDrawCommands;
    CommandBucket3<Material*, SkinnedDrawCommand> skinnedDrawCommandsNorSort;

    //NOTE: This not working why Materials can shared the same shader
    /*std::unordered_set<Ref<Material>> drawCommandsMaterials;
    //std::vector<Ref<Material>> drawCommandsMaterials;
    std::set<Ref<Material>> drawIntancingCommandsMaterials;
    std::set<Ref<Material>> skinnedDrawCommandsMaterials;*/

    CommandBucket0<DrawMultTypeCommand> sortDrawMultTypeCommands;

    Ref<Material> overrideMaterial = nullptr;
};

}