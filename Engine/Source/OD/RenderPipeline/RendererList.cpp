#include "RendererList.h"
#include "OD/Graphics/Material.h"
#include "OD/Graphics/Mesh.h"
#include "OD/Graphics/Graphics.h"
#include "OD/Core/Instrumentor.h"
#include "MeshRendererComponent.h"
#include <algorithm>

namespace OD{

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

bool MaterialBind2::operator<(const MaterialBind2& a) const{
    return materialId < a.materialId;
}

void RendererList::SetOverrideMaterial(Ref<Material> material){
    overrideMaterial = material;
}

void RendererList::AddDrawCommand(DrawCommand&& comand, float distance){
    Assert(comand.material != nullptr);
    Assert(comand.meshs != nullptr);

    /*drawCommands.Add(
        {distance, comand.material->MaterialId()}, 
        comand
    );*/

    //m.lock();
    if(sortType == SortType::None){
        drawCommandsNorSort.Add(comand.material, std::move(comand));
    } else {
        drawCommands.Add(std::move(comand));
    }
    //drawCommands.Add(comand.material, comand);
    //m.unlock();
    //drawCommandsMaterials.insert(comand.material);
    
    /*if(std::find(drawCommandsMaterials.begin(), drawCommandsMaterials.end(), comand.material) == drawCommandsMaterials.end()){
        drawCommandsMaterials.push_back(comand.material);
    }*/
}   

void RendererList::AddDrawInstancingCommand(DrawCommand&& comand){
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

    #ifdef UseExperimentalCommandBucket5
    DrawInstancingCommand& c = drawIntancingCommands.Get(comand.material->MaterialId(), comand.meshs->Id());
    #else
    DrawInstancingCommand& c = drawIntancingCommands.Get(comand.material, comand.meshs);
    #endif

    c.material = comand.material;
    c.meshs = comand.meshs;
    c.buffers.push_back(comand.buffer);
}

void RendererList::AddSkinnedDrawCommand(SkinnedDrawCommand&& comand, float distance){
    Assert(comand.material != nullptr);
    Assert(comand.meshs != nullptr);

    //m.lock();

    if(sortType == SortType::None){
        skinnedDrawCommandsNorSort.Add(comand.material, std::move(comand));
    } else {
        skinnedDrawCommands.Add(
            {distance, comand.material->MaterialId()}, 
            std::move(comand)
        );
    }
    //m.unlock();
    //skinnedDrawCommandsMaterials.insert(comand.material);
}

void RendererList::Clean(){
    overrideMaterial = nullptr;

    drawCommands.Clear();
    drawCommandsNorSort.Clear();
    //drawIntancingCommands.Clear();
    skinnedDrawCommands.Clear();
    skinnedDrawCommandsNorSort.Clear();

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
    if(sortType == SortType::None){
        drawCommands.sortFunction = nullptr;
    }
    if(sortType == SortType::CommonOpaque){
        /*drawCommands.sortFunction = [](auto& a, auto& b){
            if(a.first.materialId != b.first.materialId) return a.first.materialId < b.first.materialId;
            return a.first.distance < b.first.distance;
        };*/

        drawCommands.sortFunction = [](auto& a, auto& b){
            //return a.material->MaterialId() < b.material->MaterialId();
            if(a.material->MaterialId() != b.material->MaterialId()) return a.material->MaterialId() < b.material->MaterialId();
            return a.distance < b.distance;
        };

        skinnedDrawCommands.sortFunction = [](auto& a, auto& b){
            //return a.first.materialId < b.first.materialId;
            if(a.first.materialId != b.first.materialId) return a.first.materialId < b.first.materialId;
            return a.first.distance < b.first.distance;
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
    }

    drawCommands.Sort();
    //drawIntancingCommands.Sort();
    skinnedDrawCommands.Sort();
}

void RendererList::Submit(bool skipEntityId){
    OD_PROFILE_SCOPE("RendererList::Submit");
    Material* lastMat = nullptr;

    //NOTE: This not working why Materials can shared the same shader
    /*
    if(overrideMaterial != nullptr){
        overrideMaterial->DisableKeyword("INSTANCING");
        overrideMaterial->DisableKeyword("SKINNED");
        Material::SubmitGraphicDatas(*overrideMaterial);
        if(onUpdateMaterial != nullptr) onUpdateMaterial(*overrideMaterial);
    } else {
        for(Ref<Material> i: drawCommandsMaterials){
            i->DisableKeyword("INSTANCING");
            i->DisableKeyword("SKINNED");
            Material::SubmitGraphicDatas(*i);
            if(onUpdateMaterial != nullptr) onUpdateMaterial(*i);
        }
    }*/
    {
    OD_PROFILE_SCOPE("RendererList::Submit::drawCommands");
    drawCommands.Each([&](auto& cm){
        OD_PROFILE_SCOPE("RendererList::Submit::drawCommands::0");
        Material* _mat = cm.material;

        if(overrideMaterial != nullptr) _mat = overrideMaterial.get();

        if(_mat != lastMat){
            if(onUpdateMaterial != nullptr) onUpdateMaterial(*_mat);
            _mat->DisableKeyword("INSTANCING");
            _mat->DisableKeyword("INSTANCINGMATRIX43");
            _mat->DisableKeyword("SKINNED");
        }

        lastMat = _mat;
        if(skipEntityId) cm.perDrawData.int_0.clear();
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
            _mat->DisableKeyword("INSTANCING");
            _mat->DisableKeyword("INSTANCINGMATRIX43");
            _mat->DisableKeyword("SKINNED");
        }

        lastMat = _mat;
        if(skipEntityId) cm.perDrawData.int_0.clear();
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
            #ifdef USE_INSTANCING_MATRIX43
            _mat->EnableKeyword("INSTANCINGMATRIX43");
            #else
            _mat->EnableKeyword("INSTANCING");
            #endif
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
            _mat->EnableKeyword("SKINNED");
        }

        lastMat = _mat;
        if(skipEntityId) cm.perDrawData.int_0.clear();
        Graphics::DrawMeshSkinned(*cm.meshs, *_mat, cm.trans, &(*cm.posePalette)[0], cm.posePalette->size(), &cm.perDrawData);
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
            _mat->EnableKeyword("SKINNED");
        }

        lastMat = _mat;

        if(skipEntityId) cm.perDrawData.int_0.clear();
        Graphics::DrawMeshSkinned(*cm.meshs, *_mat, cm.trans, &(*cm.posePalette)[0], cm.posePalette->size(), &cm.perDrawData);
    });
    }
    lastMat = nullptr;
}

}