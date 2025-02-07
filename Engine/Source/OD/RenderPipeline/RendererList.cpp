#include "RendererList.h"
#include "OD/Graphics/Material.h"
#include "OD/Graphics/Mesh.h"
#include "OD/Graphics/Framebuffer.h"
#include "OD/Graphics/Graphics.h"
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

bool MaterialBind2::operator<(const MaterialBind2& a) const{
    return materialId < a.materialId;
}

void RendererList::SetOverrideMaterial(Ref<Material> material){
    overrideMaterial = material;
}

void RendererList::AddDrawCommand(DrawCommand comand, float distance){
    Assert(comand.material != nullptr);
    Assert(comand.meshs != nullptr);

    /*drawCommands.Add(
        {distance, comand.material->MaterialId()}, 
        comand
    );*/

    //m.lock();
    drawCommands.Add(comand);
    //m.unlock();
    //drawCommandsMaterials.insert(comand.material);
    
    /*if(std::find(drawCommandsMaterials.begin(), drawCommandsMaterials.end(), comand.material) == drawCommandsMaterials.end()){
        drawCommandsMaterials.push_back(comand.material);
    }*/
}   

void RendererList::AddDrawInstancingCommand(DrawCommand comand){
    Assert(comand.material != nullptr);
    Assert(comand.meshs != nullptr);

    DrawInstancingCommand& c = drawIntancingCommands.Get(comand.material, comand.meshs);
    c.material = comand.material;
    c.meshs = comand.meshs;
    //m.lock();
    c.trans.push_back(comand.trans);
    //m.unlock();

    //drawIntancingCommandsMaterials.insert(comand.material);
} 

void RendererList::AddSkinnedDrawCommand(SkinnedDrawCommand comand, float distance){
    Assert(comand.material != nullptr);
    Assert(comand.meshs != nullptr);

    //m.lock();
    skinnedDrawCommands.Add(
        {distance, comand.material->MaterialId()}, 
        comand
    );
    //m.unlock();
    //skinnedDrawCommandsMaterials.insert(comand.material);
}

void RendererList::Clean(){
    overrideMaterial = nullptr;

    drawCommands.Clear();
    //drawIntancingCommands.Clear();
    skinnedDrawCommands.Clear();

    for(auto& i: drawIntancingCommands.commands){
        for(auto& j: i.second){
            j.second.trans.clear();
        }
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
    drawIntancingCommands.Sort();
    skinnedDrawCommands.Sort();
}

void RendererList::Submit(){
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
    drawCommands.Each([&](auto& cm){
        auto _mat = cm.material;
        if(overrideMaterial != nullptr) _mat = overrideMaterial.get();

        if(_mat != lastMat){
            if(onUpdateMaterial != nullptr) onUpdateMaterial(*_mat);
            _mat->DisableKeyword("INSTANCING");
            _mat->DisableKeyword("SKINNED");
            Material::SubmitGraphicDatas(*_mat);
            if(postUpdateMaterial != nullptr) postUpdateMaterial(*_mat);
        }

        lastMat = _mat;
        //Shader::Bind(*_mat->GetShader());
        //_mat->GetShader()->SetMatrix4("model", cm.trans);
        //Graphics::DrawMeshRaw(*cm.meshs);
        Graphics::DrawMesh(*cm.meshs, *_mat, cm.trans);
    });
    lastMat = nullptr;

    // ---------------Submiting DrawIntancingCommands-----------------
    drawIntancingCommands.Each([&](auto& cm){
        if(cm.trans.size() == 0) return;
        auto _mat = cm.material;
        if(overrideMaterial != nullptr) _mat = overrideMaterial.get();

        if(_mat != lastMat){
            if(onUpdateMaterial != nullptr) onUpdateMaterial(*_mat);
            //_mat->DisableKeyword("SKINNED");
            _mat->EnableKeyword("INSTANCING");
            Material::SubmitGraphicDatas(*_mat);
            if(postUpdateMaterial != nullptr) postUpdateMaterial(*_mat);
        }
        
        /*cm.meshs->instancingModelMatrixs.clear();
        for(auto j: cm.trans){
            cm.meshs->instancingModelMatrixs.push_back(j);
        }
        cm.meshs->SubmitInstancingModelMatrixs();*/
        lastMat = _mat;
        //cm.meshs->SubmitInstancingCustomModelMatrixs(&cm.trans[0], cm.trans.size());
        //Graphics::DrawMeshInstancingRaw(*cm.meshs, cm.trans.size());
        Graphics::DrawMeshInstancing(*cm.meshs, *_mat, &cm.trans[0], cm.trans.size());
    });
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
    skinnedDrawCommands.Each([&](auto& cm){
        auto _mat = cm.material;
        if(overrideMaterial != nullptr) _mat = overrideMaterial.get();

        if(_mat != lastMat){
            if(onUpdateMaterial != nullptr) onUpdateMaterial(*_mat);
            //_mat->DisableKeyword("INSTANCING");
            _mat->EnableKeyword("SKINNED");
            Material::SubmitGraphicDatas(*_mat);
            if(postUpdateMaterial != nullptr) postUpdateMaterial(*_mat);
        }

        lastMat = _mat;
        //Shader::Bind(*_mat->GetShader());
        //_mat->GetShader()->SetMatrix4("animated", *cm.posePalette);
        //_mat->GetShader()->SetMatrix4("model", cm.trans);
        //Graphics::DrawMeshRaw(*cm.meshs);

        Graphics::DrawMeshSkinned(*cm.meshs, *_mat, cm.trans, &(*cm.posePalette)[0], cm.posePalette->size());
        
        //_mat->SetMatrix4("animated", &(*cm.posePalette)[0], cm.posePalette->size());
        //Graphics::DrawMesh(*cm.meshs, *_mat, cm.trans);
    });
    lastMat = nullptr;
}

}