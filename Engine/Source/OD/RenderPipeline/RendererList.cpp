#include "OD/pch.h"
#include "RendererList.h"
#include "OD/Graphics/Material.h"
#include "OD/Graphics/Mesh.h"
#include "OD/Graphics/UniformBuffer.h"
#include "OD/Graphics/Graphics.h"
#include "OD/Core/Instrumentor.h"
#include "OD/Core/ResourceManager.h"
#include "MeshRendererComponent.h"

namespace OD{

bool DrawMultTypeCommand::operator<(const DrawMultTypeCommand& a) const {
    return material < a.material;
}

bool DrawCommand::operator<(const DrawCommand& a) const {
    return material < a.material;
}

bool SkinnedDrawCommand::operator<(const SkinnedDrawCommand& a) const {
    return material < a.material;
}

bool DrawInstancingCommand::operator<(const DrawCommand& a) const{
    return material < a.material;
}

bool DrawInstancingCommand2::operator<(const DrawCommand& a) const{
    return material < a.material;
}

bool DrawInstancingCommand3::operator<(const DrawCommand& a) const{
    return material < a.material;
}

bool DrawInstancingCommand4::operator<(const DrawInstancingCommand4& a) const {
    return material < a.material;
}

bool MaterialBind2::operator<(const MaterialBind2& a) const{
    return materialId < a.materialId;
}

void RendererList::SetOverrideMaterial(uint32_t materialId){
    overrideMaterial = materialId;
}

void RendererList::AddDrawCommand(DrawCommand&& comand, float distance){
    Assert(comand.material != INVALID_RESOURCE_ID);
    Assert(comand.meshs != INVALID_RESOURCE_ID);

    if(sortType == SortType::None){
        drawCommandsNorSort.Add(comand.material, std::move(comand));
    } else {
        //drawCommands.Add(std::move(comand));

        DrawMultTypeCommand cmd = {comand.subShader, comand.material, comand.meshs, comand.distance};
        //cmd.perDrawData = comand.perDrawData;
        cmd.standTrans = comand.trans;
        cmd.type = DrawMultTypeCommand::Type::Stand;
        sortDrawMultTypeCommands.Add(cmd);
    }
}   

void RendererList::AddDrawInstancingCommand(DrawInstancingCommand4&& comand){
    Assert(comand.material != INVALID_RESOURCE_ID);
    Assert(comand.meshs != INVALID_RESOURCE_ID);

    #ifdef UseExperimentalCommandBucket5
    DrawInstancingCommand& c = drawIntancingCommands.Get(comand.material, comand.meshs);
    #else
    DrawInstancingCommand& c = drawIntancingCommands.Get(comand.material, comand.meshs);
    #endif

    c.material = comand.material;
    c.meshs = comand.meshs;
    #ifdef USE_INSTANCING_MATRIX43
    c.trans.push_back({
        math::row(comand.trans, 0),
        math::row(comand.trans, 1),
        math::row(comand.trans, 2)
    });
    #else
    c.trans.push_back(std::move(comand.trans));
    #endif
} 

void RendererList::AddDrawInstancingCommand(DrawInstancingCommand3&& comand){
    Assert(comand.material != INVALID_RESOURCE_ID);
    Assert(comand.meshs != INVALID_RESOURCE_ID);

    //if(sortType == SortType::None){
        #ifdef UseExperimentalCommandBucket5
        DrawInstancingCommand& c = drawIntancingCommands.Get(comand.material, comand.meshs);
        #else
        DrawInstancingCommand& c = drawIntancingCommands.Get(comand.material, comand.meshs);
        #endif

        c.material = comand.material;
        c.meshs = comand.meshs;
        c.buffers.push_back(comand.buffer);
    /*} else {
        //INFO: This gen more material submit, becose no bacth/group by material
        DrawMultTypeCommand cmd = {comand.subShader, comand.material, comand.meshs, comand.distance};
        cmd.instancingBuffer = comand.buffer;
        cmd.type = DrawMultTypeCommand::Type::Instancing;
        sortDrawMultTypeCommands.Add(cmd);
    }*/
}

void RendererList::AddSkinnedDrawCommand(SkinnedDrawCommand&& comand, float distance){
    Assert(comand.material != INVALID_RESOURCE_ID);
    Assert(comand.meshs != INVALID_RESOURCE_ID);

    if(sortType == SortType::None){
        skinnedDrawCommandsNorSort.Add(comand.material, std::move(comand));
    } else {
        /*skinnedDrawCommands.Add(
            {distance, comand.material},
            std::move(comand)
        );*/
        
        DrawMultTypeCommand cmd = {comand.subShader, comand.material, comand.meshs, comand.distance};
        //cmd.perDrawData = comand.perDrawData;
        cmd.skinnedTrans = comand.trans;
        cmd.skinnedPosePalette = comand.posePalette;
        cmd.type = DrawMultTypeCommand::Type::Skinned;
        sortDrawMultTypeCommands.Add(cmd);
    }
}

void RendererList::Clean(){
    overrideMaterial = INVALID_RESOURCE_ID;

    drawCommands.Clear();
    drawCommandsNorSort.Clear();
#ifdef UseExperimentalCommandBucket5
    for(auto& i: drawIntancingCommands.commands){
        for(auto& command: i){
            command.trans.clear();
        }
    }
#else
    drawIntancingCommands.Clear();
#endif
    skinnedDrawCommands.Clear();
    skinnedDrawCommandsNorSort.Clear();
    sortDrawMultTypeCommands.Clear();

    /*drawCommandsMaterials.clear();
    drawIntancingCommandsMaterials.clear();
    skinnedDrawCommandsMaterials.clear();*/
}

void RendererList::Sort(){
    //return;

    if(sortType == SortType::None){
        drawCommands.sortFunction = nullptr;
        return;
    }

    if(sortType == SortType::CommonOpaque){
        /*drawCommands.sortFunction = [](auto& a, auto& b){
            if(a.first.materialId != b.first.materialId) return a.first.materialId < b.first.materialId;
            return a.first.distance < b.first.distance;
        };*/

        drawCommands.sortFunction = [](auto& a, auto& b){
            if(a.subShader != b.subShader) return a.subShader < b.subShader;  
            if(a.material != b.material) return a.material < b.material; 
            return a.meshs < b.meshs;   
            
            //if(a.material != b.material) return a.material < b.material;
            //return a.distance < b.distance;
        };

        skinnedDrawCommands.sortFunction = [](auto& a, auto& b){
            if(a.second.subShader != b.second.subShader) return a.second.subShader < b.second.subShader;  
            if(a.second.material != b.second.material) return a.second.material < b.second.material; 
            return a.second.meshs < b.second.meshs;   

            //if(a.first.materialId != b.first.materialId) return a.first.materialId < b.first.materialId;
            //return a.first.distance < b.first.distance;
        };

        sortDrawMultTypeCommands.sortFunction = [](auto& a, auto& b){
            //LogInfo("Test");
            if(a.subShader != b.subShader) return a.subShader < b.subShader;  
            if(a.material != b.material) return a.material < b.material; 
            return a.meshs < b.meshs;   
            //return a.distance > b.distance;
        };
    }

    if(sortType == SortType::CommonTransparent){
        /*drawCommands.sortFunction = [](auto& a, auto& b){
            if(a.first.materialId != b.first.materialId) return a.first.materialId < b.first.materialId;
            return a.first.distance > b.first.distance;
        };*/

        drawCommands.sortFunction = [](auto& a, auto& b){
            if(a.material != b.material) return a.material < b.material;
            return a.distance > b.distance;
        };

        skinnedDrawCommands.sortFunction = [](auto& a, auto& b){
            if(a.first.materialId != b.first.materialId) return a.first.materialId < b.first.materialId;
            return a.first.distance > b.first.distance;
        };

        sortDrawMultTypeCommands.sortFunction = [](auto& a, auto& b){
            //Comparing material IDs here can change blending order.
            //LogInfo("A: %f, B: %f", a.distance, b.distance);
            return a.distance > b.distance;
        };
    }

    drawCommands.Sort();
    //drawIntancingCommands.Sort();
    skinnedDrawCommands.Sort();
    sortDrawMultTypeCommands.Sort();
}

void RendererList::Submit(bool skipEntityId){
    OD_PROFILE_SCOPE("RendererList::Submit");
    auto* materialView = ResourceManager::Get().GetAllocatorView<Material>();
    auto* meshView = ResourceManager::Get().GetAllocatorView<Mesh>();
    auto* instancingBufferView = ResourceManager::Get().GetAllocatorView<InstancingBuffer>();
    auto* uniformBufferView = ResourceManager::Get().GetAllocatorView<UniformBuffer>();
    Material* lastMat = nullptr;

    {
    OD_PROFILE_SCOPE("RendererList::Submit::drawCommands");
    drawCommands.Each([&](auto& cm){
        OD_PROFILE_SCOPE("RendererList::Submit::drawCommands::0");
        Material* _mat = overrideMaterial != INVALID_RESOURCE_ID ? materialView->Get(overrideMaterial) : materialView->Get(cm.material);
        Mesh* mesh = meshView->Get(cm.meshs);
        if(_mat == nullptr || mesh == nullptr) return;

        if(_mat != lastMat){
            if(onUpdateMaterial != nullptr) onUpdateMaterial(*_mat);
            /*_mat->DisableKeyword("INSTANCING");
            _mat->DisableKeyword("INSTANCINGMATRIX43");
            _mat->DisableKeyword("SKINNED");*/
        }

        lastMat = _mat;
        //if(skipEntityId) cm.perDrawData.Int_0_SetMask(0, false);
        Graphics::DrawMesh(*mesh, *_mat, cm.trans);//, &cm.perDrawData);
    });
    }
    lastMat = nullptr;

    {
    OD_PROFILE_SCOPE("RendererList::Submit::drawCommandsNorSort");
    drawCommandsNorSort.Each([&](auto& cm){
        OD_PROFILE_SCOPE("RendererList::Submit::drawCommands::0");
        Material* _mat = overrideMaterial != INVALID_RESOURCE_ID ? materialView->Get(overrideMaterial) : materialView->Get(cm.material);
        Mesh* mesh = meshView->Get(cm.meshs);
        if(_mat == nullptr || mesh == nullptr) return;

        if(_mat != lastMat){
            if(onUpdateMaterial != nullptr) onUpdateMaterial(*_mat);
            /*_mat->DisableKeyword("INSTANCING");
            _mat->DisableKeyword("INSTANCINGMATRIX43");
            _mat->DisableKeyword("SKINNED");*/
        }

        lastMat = _mat;
        //if(skipEntityId) cm.perDrawData.Int_0_SetMask(0, false);
        Graphics::DrawMesh(*mesh, *_mat, cm.trans);//, &cm.perDrawData);
    });
    }
    lastMat = nullptr;

    // ---------------Submiting DrawIntancingCommands-----------------
    {
    OD_PROFILE_SCOPE("RendererList::Submit::drawIntancingCommands");
    drawIntancingCommands.Each([&](auto& cm){
        if(cm.trans.size() == 0 && cm.buffers.size() == 0) return;
        Material* _mat = overrideMaterial != INVALID_RESOURCE_ID ? materialView->Get(overrideMaterial) : materialView->Get(cm.material);
        Mesh* mesh = meshView->Get(cm.meshs);
        if(_mat == nullptr || mesh == nullptr) return;

        if(_mat != lastMat){
            if(onUpdateMaterial != nullptr) onUpdateMaterial(*_mat);
            //_mat->DisableKeyword("SKINNED");
            /*#ifdef USE_INSTANCING_MATRIX43
            _mat->EnableKeyword("INSTANCINGMATRIX43");
            #else
            _mat->EnableKeyword("INSTANCING");
            #endif*/
        }
        
        lastMat = _mat;
        if(cm.trans.size() > 0){
            Graphics::DrawMeshInstancing(*mesh, *_mat, &cm.trans[0], cm.trans.size());
        }
        for(uint32_t bufferId: cm.buffers){
            InstancingBuffer* buffer = instancingBufferView->Get(bufferId);
            if(buffer != nullptr) Graphics::DrawMeshInstancing(*mesh, *_mat, *buffer, buffer->Count());
        }
    });
    }
    lastMat = nullptr;

    // ---------------Submiting SkinnedDrawCommands-----------------
    {
    OD_PROFILE_SCOPE("RendererList::Submit::skinnedDrawCommands");
    skinnedDrawCommands.Each([&](auto& cm){
        Material* _mat = overrideMaterial != INVALID_RESOURCE_ID ? materialView->Get(overrideMaterial) : materialView->Get(cm.material);
        Mesh* mesh = meshView->Get(cm.meshs);
        if(_mat == nullptr || mesh == nullptr || cm.posePalette == nullptr) return;

        if(_mat != lastMat){
            if(onUpdateMaterial != nullptr) onUpdateMaterial(*_mat);
            //_mat->DisableKeyword("INSTANCING");
            /*_mat->EnableKeyword("SKINNED");*/
        }

        lastMat = _mat;
        //if(skipEntityId) cm.perDrawData.Int_0_SetMask(0, false);
        Graphics::DrawMeshSkinned(*mesh, *_mat, cm.trans, cm.posePalette->data(), cm.posePalette->size());//, &cm.perDrawData);
    });
    }
    lastMat = nullptr;
    {
    OD_PROFILE_SCOPE("RendererList::Submit::skinnedDrawCommandsNorSort");
    skinnedDrawCommandsNorSort.Each([&](auto& cm){
        Material* _mat = overrideMaterial != INVALID_RESOURCE_ID ? materialView->Get(overrideMaterial) : materialView->Get(cm.material);
        Mesh* mesh = meshView->Get(cm.meshs);
        if(_mat == nullptr || mesh == nullptr || cm.posePalette == nullptr) return;

        if(_mat != lastMat){
            if(onUpdateMaterial != nullptr) onUpdateMaterial(*_mat);
            //_mat->DisableKeyword("INSTANCING");
            /*_mat->EnableKeyword("SKINNED");*/
        }

        lastMat = _mat;

        //if(skipEntityId) cm.perDrawData.Int_0_SetMask(0, false);

        UniformBuffer* skinnedData = uniformBufferView->Get(cm.skinnedData);
        if(skinnedData != nullptr){
            Graphics::DrawMeshSkinned(*mesh, *_mat, cm.trans, skinnedData, cm.posePalette->size());//, &cm.perDrawData);
        } else {
            Graphics::DrawMeshSkinned(*mesh, *_mat, cm.trans, cm.posePalette->data(), cm.posePalette->size());//, &cm.perDrawData);
        }
    });
    }
    lastMat = nullptr;

    {
    OD_PROFILE_SCOPE("RendererList::Submit::skinnedDrawCommandsNorSort"); //TODO: Review this and maybe rename
    sortDrawMultTypeCommands.Each([&](auto& cm){//TODO: Review this and maybe rename
        Material* _mat = overrideMaterial != INVALID_RESOURCE_ID ? materialView->Get(overrideMaterial) : materialView->Get(cm.material);
        Mesh* mesh = meshView->Get(cm.meshs);
        if(_mat == nullptr || mesh == nullptr) return;

        if(_mat != lastMat){
            if(onUpdateMaterial != nullptr) onUpdateMaterial(*_mat);
        }

        lastMat = _mat;

        if(cm.type == DrawMultTypeCommand::Type::Stand){
            Graphics::DrawMesh(*mesh, *_mat, cm.standTrans);//, &cm.perDrawData);
        }
        if(cm.type == DrawMultTypeCommand::Type::Skinned){
            if(cm.skinnedPosePalette != nullptr) Graphics::DrawMeshSkinned(*mesh, *_mat, cm.skinnedTrans, cm.skinnedPosePalette->data(), cm.skinnedPosePalette->size());//, &cm.perDrawData);
        }
        if(cm.type == DrawMultTypeCommand::Type::Instancing){
            InstancingBuffer* buffer = instancingBufferView->Get(cm.instancingBuffer);
            if(buffer != nullptr) Graphics::DrawMeshInstancing(*mesh, *_mat, *buffer, buffer->Count());
        }

    });
    }
    lastMat = nullptr;
}

}
