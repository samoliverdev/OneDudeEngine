#include "CommandBuffer.h"
#include "OD/Graphics/Material.h"
#include "OD/Graphics/Mesh.h"
#include "OD/Graphics/Framebuffer.h"
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

void CommandBuffer::SetCamera(Camera inCamera){
    setCamera = true;
    camera = inCamera;
}

void CommandBuffer::CleanRenderTarget(Vector3 inClearColor){
    clearRenderTarget = true;
    clearColor = inClearColor;
}

void CommandBuffer::SetRenderTarget(Framebuffer* framebuffer){
    setRenderTarget = true;
    renderTarget = framebuffer;
}

void CommandBuffer::SetViewport(IVector4 inViewport){
    setViewport = true;
    viewport = inViewport;
}

void CommandBuffer::SetOverrideMaterial(Ref<Material> material){
    overrideMaterial = material;
}

void CommandBuffer::AddDrawCommand(DrawCommand comand, float distance){
    Assert(comand.material != nullptr);
    Assert(comand.meshs != nullptr);

    /*drawCommands.Add(
        {distance, comand.material->MaterialId()}, 
        comand
    );*/

    drawCommands.Add(comand);
    //drawCommandsMaterials.insert(comand.material);
    
    /*if(std::find(drawCommandsMaterials.begin(), drawCommandsMaterials.end(), comand.material) == drawCommandsMaterials.end()){
        drawCommandsMaterials.push_back(comand.material);
    }*/
}   

void CommandBuffer::AddDrawInstancingCommand(DrawCommand comand){
    Assert(comand.material != nullptr);
    Assert(comand.meshs != nullptr);

    DrawInstancingCommand& c = drawIntancingCommands.Get(comand.material, comand.meshs);
    c.material = comand.material;
    c.meshs = comand.meshs;
    c.trans.push_back(comand.trans);

    //drawIntancingCommandsMaterials.insert(comand.material);
} 

void CommandBuffer::AddSkinnedDrawCommand(SkinnedDrawCommand comand, float distance){
    Assert(comand.material != nullptr);
    Assert(comand.meshs != nullptr);

    skinnedDrawCommands.Add(
        {distance, comand.material->MaterialId()}, 
        comand
    );
    //skinnedDrawCommandsMaterials.insert(comand.material);
}

void CommandBuffer::Clean(){
    overrideMaterial = nullptr;
    setRenderTarget = false;
    renderTarget = nullptr;
    clearRenderTarget = false;
    setViewport = false;
    setCamera = false;
    camera = Camera();

    drawCommands.Clear();
    drawIntancingCommands.Clear();
    skinnedDrawCommands.Clear();

    /*drawCommandsMaterials.clear();
    drawIntancingCommandsMaterials.clear();
    skinnedDrawCommandsMaterials.clear();*/
}

void CommandBuffer::Sort(){
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

void CommandBuffer::Submit(){
    if(setRenderTarget && renderTarget != nullptr){
        Framebuffer::Bind(*renderTarget);
    } else if(setRenderTarget && renderTarget == nullptr){
        Framebuffer::Unbind();
    }

    if(setCamera) Graphics::SetCamera(camera);
    if(setViewport) Graphics::SetViewport(viewport.x, viewport.y, viewport.z, viewport.w);
    if(clearRenderTarget) Graphics::Clean(clearColor.x, clearColor.y, clearColor.z, 1);

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
            _mat->DisableKeyword("INSTANCING");
            _mat->DisableKeyword("SKINNED");
            Material::SubmitGraphicDatas(*_mat);
            if(onUpdateMaterial != nullptr) onUpdateMaterial(*_mat);
        }

        lastMat = _mat;
        //Shader::Bind(*_mat->GetShader());
        _mat->GetShader()->SetMatrix4("model", cm.trans);
        Graphics::DrawMeshRaw(*cm.meshs);
    });
    lastMat = nullptr;

    // ---------------Submiting DrawIntancingCommands-----------------
    drawIntancingCommands.Each([&](auto& cm){
        auto _mat = cm.material;
        if(overrideMaterial != nullptr) _mat = overrideMaterial.get();

        _mat->DisableKeyword("SKINNED");
        _mat->EnableKeyword("INSTANCING");
        Material::SubmitGraphicDatas(*_mat);
        if(onUpdateMaterial != nullptr) onUpdateMaterial(*_mat);
        
        cm.meshs->instancingModelMatrixs.clear();
        for(auto j: cm.trans){
            cm.meshs->instancingModelMatrixs.push_back(j);
        }
        cm.meshs->SubmitInstancingModelMatrixs();
        
        Graphics::DrawMeshInstancingRaw(*cm.meshs, cm.trans.size());
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
            _mat->DisableKeyword("INSTANCING");
            _mat->EnableKeyword("SKINNED");
            Material::SubmitGraphicDatas(*_mat);
            if(onUpdateMaterial != nullptr) onUpdateMaterial(*_mat);
        }

        lastMat = _mat;
        //Shader::Bind(*_mat->GetShader());
        _mat->GetShader()->SetMatrix4("animated", *cm.posePalette);
        _mat->GetShader()->SetMatrix4("model", cm.trans);
        Graphics::DrawMeshRaw(*cm.meshs);
    });
    lastMat = nullptr;
}

}