#include "CommandBuffer.h"
#include "MeshRendererComponent.h"
#include "OD/Core/AssetManager.h"
#include "OD/Core/Instrumentor.h"
#include "OD/Defines.h"

namespace OD{

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
    drawCommandsMaterials.insert(comand.material);
}   

void CommandBuffer::AddDrawInstancingCommand(DrawCommand comand){
    Assert(comand.material != nullptr);
    Assert(comand.meshs != nullptr);

    DrawInstancingCommand& c = drawIntancingCommands.Get(comand.material, comand.meshs);
    c.material = comand.material;
    c.meshs = comand.meshs;
    c.trans.push_back(comand.trans);
} 

void CommandBuffer::AddSkinnedDrawCommand(SkinnedDrawCommand comand, float distance){
    Assert(comand.material != nullptr);
    Assert(comand.meshs != nullptr);

    skinnedDrawCommands.Add(
        {distance, comand.material->MaterialId()}, 
        comand
    );
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

    drawCommandsMaterials.clear();
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

    Ref<Material> lastMat = nullptr;

    // ---------------Submiting DrawCommands-----------------
    /*for(auto i: drawCommandsMaterials){
        i->DisableKeyword("INSTANCING");
        i->DisableKeyword("SKINNED");
        Material::SubmitGraphicDatas(*i);
        if(onUpdateMaterial != nullptr) onUpdateMaterial(*i);
    }
    if(overrideMaterial != nullptr){
        overrideMaterial->DisableKeyword("INSTANCING");
        overrideMaterial->DisableKeyword("SKINNED");
        Material::SubmitGraphicDatas(*overrideMaterial);
        if(onUpdateMaterial != nullptr) onUpdateMaterial(*overrideMaterial);
    }*/
    drawCommands.Each([&](auto& cm){
        Ref<Material> _mat = cm.material;
        if(overrideMaterial != nullptr) _mat = overrideMaterial;

        if(_mat != lastMat){
            _mat->DisableKeyword("INSTANCING");
            _mat->DisableKeyword("SKINNED");
            Material::SubmitGraphicDatas(*_mat);
            if(onUpdateMaterial != nullptr) onUpdateMaterial(*_mat);
        }

        lastMat = _mat;
        //Shader::Bind(*_mat->GetShader());
        _mat->GetShader()->SetMatrix4("model", cm.trans);
        Graphics::DrawMesh(*cm.meshs);
    });

    // ---------------Submiting DrawIntancingCommands-----------------
    drawIntancingCommands.Each([&](auto& cm){
        Ref<Material> _mat = cm.material;
        if(overrideMaterial != nullptr) _mat = overrideMaterial;

        _mat->DisableKeyword("SKINNED");
        _mat->EnableKeyword("INSTANCING");
        Material::SubmitGraphicDatas(*_mat);
        if(onUpdateMaterial != nullptr) onUpdateMaterial(*_mat);
        
        cm.meshs->instancingModelMatrixs.clear();
        for(auto j: cm.trans){
            cm.meshs->instancingModelMatrixs.push_back(j);
        }
        cm.meshs->UpdateMeshInstancingModelMatrixs();
        
        Graphics::DrawMeshInstancing(*cm.meshs, cm.trans.size());
    });

    // ---------------Submiting SkinnedDrawCommands-----------------
    skinnedDrawCommands.Each([&](auto& cm){
        Ref<Material> _mat = cm.material;
        if(overrideMaterial != nullptr) _mat = overrideMaterial;

        if(_mat != lastMat){
            _mat->DisableKeyword("INSTANCING");
            _mat->EnableKeyword("SKINNED");
            Material::SubmitGraphicDatas(*_mat);
            if(onUpdateMaterial != nullptr) onUpdateMaterial(*_mat);
        }

        lastMat = _mat;
        _mat->GetShader()->SetMatrix4("animated", *cm.posePalette);
        _mat->GetShader()->SetMatrix4("model", cm.trans);
        Graphics::DrawMesh(*cm.meshs);
    });
}

}