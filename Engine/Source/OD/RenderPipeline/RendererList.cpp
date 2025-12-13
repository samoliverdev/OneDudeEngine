#include "OD/pch.h"
#include "RendererList.h"
#include "OD/Graphics/Material.h"
#include "OD/Graphics/Mesh.h"
#include "OD/Graphics/Graphics.h"
#include "OD/Core/Instrumentor.h"
#include "MeshRendererComponent.h"

namespace OD{

bool DrawMultTypeCommand::operator<(const DrawMultTypeCommand& a) const {
    return material->MaterialId() < a.material->MaterialId();
}

bool DrawCommand::operator<(const DrawCommand& a) const {
    return material->MaterialId() < a.material->MaterialId();
}

bool SkinnedDrawCommand::operator<(const SkinnedDrawCommand& a) const {
    return material->MaterialId() < a.material->MaterialId();
}

bool DrawInstancingCommand::operator<(const DrawCommand& a) const{
    return material->MaterialId() < a.material->MaterialId();
}

bool DrawInstancingCommand2::operator<(const DrawCommand& a) const{
    return material->MaterialId() < a.material->MaterialId();
}

bool DrawInstancingCommand3::operator<(const DrawCommand& a) const{
    return material->MaterialId() < a.material->MaterialId();
}

bool DrawInstancingCommand4::operator<(const DrawInstancingCommand4& a) const {
    return material->MaterialId() < a.material->MaterialId();
}

bool MaterialBind2::operator<(const MaterialBind2& a) const{
    return materialId < a.materialId;
}

void RendererList::SetOverrideMaterial(Ref<Material> material){
    overrideMaterial = material;
}

void RendererList::AddDrawCommand(DrawCommand&& comand, float distance){
    Assert(comand.material != nullptr);
    Assert(comand.meshs != nullptr);

    if(sortType == SortType::None){
        drawCommandsNorSort.Add(comand.material, std::move(comand));
    } else {
        //drawCommands.Add(std::move(comand));

        DrawMultTypeCommand cmd = {comand.subShader, comand.material, comand.meshs, comand.distance};
        cmd.standTrans = comand.trans;
        cmd.type = DrawMultTypeCommand::Type::Stand;
        sortDrawMultTypeCommands.Add(cmd);
    }
}   

void RendererList::AddDrawInstancingCommand(DrawInstancingCommand4&& comand){
    Assert(comand.material != nullptr);
    Assert(comand.meshs != nullptr);

    #ifdef UseExperimentalCommandBucket5
    DrawInstancingCommand& c = drawIntancingCommands.Get(comand.material->MaterialId(), comand.meshs->Id());
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
    Assert(comand.material != nullptr);
    Assert(comand.meshs != nullptr);

    if(sortType == SortType::None){
        #ifdef UseExperimentalCommandBucket5
        DrawInstancingCommand& c = drawIntancingCommands.Get(comand.material->MaterialId(), comand.meshs->Id());
        #else
        DrawInstancingCommand& c = drawIntancingCommands.Get(comand.material, comand.meshs);
        #endif

        c.material = comand.material;
        c.meshs = comand.meshs;
        c.buffers.push_back(comand.buffer);
    } else {
        DrawMultTypeCommand cmd = {comand.subShader, comand.material, comand.meshs, comand.distance};
        cmd.instancingBuffer = comand.buffer;
        cmd.type = DrawMultTypeCommand::Type::Instancing;
        sortDrawMultTypeCommands.Add(cmd);
    }
}

void RendererList::AddSkinnedDrawCommand(SkinnedDrawCommand&& comand, float distance){
    Assert(comand.material != nullptr);
    Assert(comand.meshs != nullptr);

    if(sortType == SortType::None){
        skinnedDrawCommandsNorSort.Add(comand.material, std::move(comand));
    } else {
        /*skinnedDrawCommands.Add(
            {distance, comand.material->MaterialId()}, 
            std::move(comand)
        );*/

        DrawMultTypeCommand cmd = {comand.subShader, comand.material, comand.meshs, comand.distance};
        cmd.skinnedTrans = comand.trans;
        cmd.skinnedPosePalette = comand.posePalette;
        cmd.type = DrawMultTypeCommand::Type::Skinned;
        sortDrawMultTypeCommands.Add(cmd);
    }
}

void RendererList::Clean(){
    overrideMaterial = nullptr;

    drawCommands.Clear();
    drawCommandsNorSort.Clear();
    //drawIntancingCommands.Clear();
    skinnedDrawCommands.Clear();
    skinnedDrawCommandsNorSort.Clear();
    sortDrawMultTypeCommands.Clear();

    for(auto& i: drawIntancingCommands.commands){
        #ifdef UseExperimentalCommandBucket5
        for(auto& j: i){
            j.trans.clear();
        }
        #else
        for(auto& j: i.second){
            j.second.trans.clear();
            j.second.buffers.clear();
        }
        #endif
    }

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
            
            //if(a.material->MaterialId() != b.material->MaterialId()) return a.material->MaterialId() < b.material->MaterialId();
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
            if(a.material->MaterialId() != b.material->MaterialId()) return a.material->MaterialId() < b.material->MaterialId();
            return a.distance > b.distance;
        };

        skinnedDrawCommands.sortFunction = [](auto& a, auto& b){
            if(a.first.materialId != b.first.materialId) return a.first.materialId < b.first.materialId;
            return a.first.distance > b.first.distance;
        };

        sortDrawMultTypeCommands.sortFunction = [](auto& a, auto& b){
            //if(a.material->MaterialId() != b.material->MaterialId()) return a.material->MaterialId() < b.material->MaterialId(); //This can bug the blending order
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
    Material* lastMat = nullptr;

    {
    OD_PROFILE_SCOPE("RendererList::Submit::drawCommands");
    drawCommands.Each([&](auto& cm){
        OD_PROFILE_SCOPE("RendererList::Submit::drawCommands::0");
        Material* _mat = cm.material;

        if(overrideMaterial != nullptr) _mat = overrideMaterial.get();

        if(_mat != lastMat){
            if(onUpdateMaterial != nullptr) onUpdateMaterial(*_mat);
            /*_mat->DisableKeyword("INSTANCING");
            _mat->DisableKeyword("INSTANCINGMATRIX43");
            _mat->DisableKeyword("SKINNED");*/
        }

        lastMat = _mat;
        if(skipEntityId) cm.perDrawData.Int_0_SetMask(0, false);
        Graphics::DrawMesh(*cm.meshs, *_mat, cm.trans, &cm.perDrawData);
    });
    }
    lastMat = nullptr;

    {
    OD_PROFILE_SCOPE("RendererList::Submit::drawCommandsNorSort");
    drawCommandsNorSort.Each([&](auto& cm){
        OD_PROFILE_SCOPE("RendererList::Submit::drawCommands::0");
        Material* _mat = cm.material;

        if(overrideMaterial != nullptr) _mat = overrideMaterial.get();

        if(_mat != lastMat){
            if(onUpdateMaterial != nullptr) onUpdateMaterial(*_mat);
            /*_mat->DisableKeyword("INSTANCING");
            _mat->DisableKeyword("INSTANCINGMATRIX43");
            _mat->DisableKeyword("SKINNED");*/
        }

        lastMat = _mat;
        if(skipEntityId) cm.perDrawData.Int_0_SetMask(0, false);
        Graphics::DrawMesh(*cm.meshs, *_mat, cm.trans, &cm.perDrawData);
    });
    }
    lastMat = nullptr;

    // ---------------Submiting DrawIntancingCommands-----------------
    {
    OD_PROFILE_SCOPE("RendererList::Submit::drawIntancingCommands");
    drawIntancingCommands.Each([&](auto& cm){
        if(cm.trans.size() == 0 && cm.buffers.size() == 0) return;
        auto _mat = cm.material;
        if(overrideMaterial != nullptr) _mat = overrideMaterial.get();

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
            Graphics::DrawMeshInstancing(*cm.meshs, *_mat, &cm.trans[0], cm.trans.size());
        }
        for(auto& buffer: cm.buffers){
            Graphics::DrawMeshInstancing(*cm.meshs, *_mat, *buffer, buffer->Count());
        }
    });
    }
    lastMat = nullptr;

    // ---------------Submiting SkinnedDrawCommands-----------------
    //NOTE: This not working why Materials can shared the same shader
    /*
    if(overrideMaterial != nullptr){
        overrideMaterial->DisableKeyword("INSTANCING");
        overrideMaterial->EnableKeyword("SKINNED");
        Material::SubmitGraphicDatas(*overrideMaterial);
        if(onUpdateMaterial != nullptr) onUpdateMaterial(*overrideMaterial);
    } else {
        for(auto i: skinnedDrawCommandsMaterials){
            i->DisableKeyword("INSTANCING");
            i->EnableKeyword("SKINNED");
            Material::SubmitGraphicDatas(*i);
            if(onUpdateMaterial != nullptr) onUpdateMaterial(*i);
        }
    }*/
    {
    OD_PROFILE_SCOPE("RendererList::Submit::skinnedDrawCommands");
    skinnedDrawCommands.Each([&](auto& cm){
        auto _mat = cm.material;
        if(overrideMaterial != nullptr) _mat = overrideMaterial.get();

        if(_mat != lastMat){
            if(onUpdateMaterial != nullptr) onUpdateMaterial(*_mat);
            //_mat->DisableKeyword("INSTANCING");
            /*_mat->EnableKeyword("SKINNED");*/
        }

        lastMat = _mat;
        if(skipEntityId) cm.perDrawData.Int_0_SetMask(0, false);
        Graphics::DrawMeshSkinned(*cm.meshs, *_mat, cm.trans, cm.posePalette->data(), cm.posePalette->size(), &cm.perDrawData);
    });
    }
    lastMat = nullptr;
    {
    OD_PROFILE_SCOPE("RendererList::Submit::skinnedDrawCommandsNorSort");
    skinnedDrawCommandsNorSort.Each([&](auto& cm){
        auto _mat = cm.material;
        if(overrideMaterial != nullptr) _mat = overrideMaterial.get();

        if(_mat != lastMat){
            if(onUpdateMaterial != nullptr) onUpdateMaterial(*_mat);
            //_mat->DisableKeyword("INSTANCING");
            /*_mat->EnableKeyword("SKINNED");*/
        }

        lastMat = _mat;

        if(skipEntityId) cm.perDrawData.Int_0_SetMask(0, false);
        Graphics::DrawMeshSkinned(*cm.meshs, *_mat, cm.trans, cm.posePalette->data(), cm.posePalette->size(), &cm.perDrawData);
    });
    }
    lastMat = nullptr;

    {
    OD_PROFILE_SCOPE("RendererList::Submit::skinnedDrawCommandsNorSort");
    sortDrawMultTypeCommands.Each([&](auto& cm){
        auto _mat = cm.material;
        if(overrideMaterial != nullptr) _mat = overrideMaterial.get();

        if(_mat != lastMat){
            if(onUpdateMaterial != nullptr) onUpdateMaterial(*_mat);
        }

        lastMat = _mat;

        if(cm.type == DrawMultTypeCommand::Type::Stand){
            Graphics::DrawMesh(*cm.meshs, *_mat, cm.standTrans);
        }

        if(cm.type == DrawMultTypeCommand::Type::Skinned){
            Graphics::DrawMeshSkinned(*cm.meshs, *_mat, cm.skinnedTrans, cm.skinnedPosePalette->data(), cm.skinnedPosePalette->size());
        }

        if(cm.type == DrawMultTypeCommand::Type::Instancing){
            Graphics::DrawMeshInstancing(*cm.meshs, *_mat, *cm.instancingBuffer, cm.instancingBuffer->Count());
        }

    });
    }
    lastMat = nullptr;
}

}