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

void CommandBuffer::AddGlobalShader(Ref<Shader> shader){
    globalShaders.insert(shader);
}

void CommandBuffer::SetGlobalFloat(const char* name, float value){
    globalMaterial.SetFloat(name, value);
}

void CommandBuffer::SetGlobalVector2(const char* name, Vector2 value){
    globalMaterial.SetVector2(name, value);
}

void CommandBuffer::SetGlobalVector3(const char* name, Vector3 value){
    globalMaterial.SetVector3(name, value);
}

void CommandBuffer::SetGlobalVector4(const char* name, Vector4 value){
    globalMaterial.SetVector4(name, value);
}

void CommandBuffer::SetGlobalMatrix4(const char* name, Matrix4 value){
    globalMaterial.SetMatrix4(name, value);
}

void CommandBuffer::SetGlobalTexture(const char* name, Ref<Texture2D> tex){
    globalMaterial.SetTexture(name, tex);
}

void CommandBuffer::SetGlobalTexture(const char* name, Framebuffer* tex){
    globalMaterial.SetTexture(name, tex);
}

void CommandBuffer::SetGlobalCubemap(const char* name, Ref<Cubemap> tex){
    globalMaterial.SetCubemap(name, tex);
}

void CommandBuffer::SetOverrideMaterial(Ref<Material> shader){
    overrideMaterial = shader;
}

void CommandBuffer::AddDrawCommand(DrawCommand comand, float distance){
    Assert(comand.material != nullptr);
    Assert(comand.meshs != nullptr);

    /*drawCommands.Add(
        {distance, comand.material->MaterialId()}, 
        comand
    );*/

    drawCommands.Add(comand);
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
    globalMaterial.CleanData();
    globalShaders.clear();
    drawCommands.Clear();
    drawIntancingCommands.Clear();
    skinnedDrawCommands.Clear();
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
            if(a.material->MaterialId() != b.material->MaterialId()) return a.material->MaterialId() < b.material->MaterialId();
            return a.distance < b.distance;
        };

        skinnedDrawCommands.sortFunction = [](auto& a, auto& b){
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

    if(setCamera) Renderer::SetCamera(camera);

    if(setViewport) Renderer::SetViewport(viewport.x, viewport.y, viewport.z, viewport.w);
    if(clearRenderTarget) Renderer::Clean(clearColor.x, clearColor.y, clearColor.z, 1);

    for(auto i: globalShaders){
        globalMaterial.ApplyUniformTo(*i);
    }

    Ref<Material> lastMat = nullptr;

    drawCommands.Each([&](auto& cm){
        Ref<Material> _mat = cm.material;
        if(overrideMaterial != nullptr) _mat = overrideMaterial;

        if(_mat != lastMat){
            _mat->UpdateDatas();
            _mat->GetShader()->SetFloat("useInstancing", 0.0f); 
            _mat->GetShader()->SetFloat("isSkinned", 0.0f);  
            if(onUpdateMaterial != nullptr) onUpdateMaterial(*_mat);
        }
    
        lastMat = _mat;
        _mat->GetShader()->SetMatrix4("model", cm.trans);
        Renderer::DrawMesh(*cm.meshs);
    });

    drawIntancingCommands.Each([&](auto& cm){
        Ref<Material> _mat = cm.material;
        if(overrideMaterial != nullptr) _mat = overrideMaterial;

        _mat->UpdateDatas();
        _mat->GetShader()->SetFloat("useInstancing", 1); 
        _mat->GetShader()->SetFloat("isSkinned", 0.0f);  
        if(onUpdateMaterial != nullptr) onUpdateMaterial(*_mat);

        cm.meshs->instancingModelMatrixs.clear();

        for(auto j: cm.trans){
            cm.meshs->instancingModelMatrixs.push_back(j);
        }
        cm.meshs->UpdateMeshInstancingModelMatrixs();
        Renderer::DrawMeshInstancing(*cm.meshs, cm.trans.size());
    });

    skinnedDrawCommands.Each([&](auto& cm){
        Ref<Material> _mat = cm.material;
        if(overrideMaterial != nullptr) _mat = overrideMaterial;

        if(_mat != lastMat){
            _mat->UpdateDatas();
            _mat->GetShader()->SetFloat("useInstancing", 0.0f);
            _mat->GetShader()->SetFloat("isSkinned", 1.0f);  
            if(onUpdateMaterial != nullptr) onUpdateMaterial(*_mat);
        }

        lastMat = _mat;
        _mat->GetShader()->SetMatrix4("animated", *cm.posePalette);
        _mat->GetShader()->SetMatrix4("model", cm.trans);
        Renderer::DrawMesh(*cm.meshs);
    });
}

}