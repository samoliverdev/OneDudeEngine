#pragma once
#include "OD/Defines.h"
#include "OD/Scene/Scene.h"
#include "OD/Serialization/Serialization.h"
#include "RenderingPath.h"
#include "PassRenderSettings.h"

namespace sol{ class state; }

namespace OD{

class Framebuffer;

struct OD_API CameraComponent{
    enum class Type{
        Perspective, Orthographic 
    };

    Type type = Type::Perspective;

    bool isMain = true;

    float orthographicSize = 5;
    float fieldOfView = 45;
    float nearClipPlane = 0.1f;
    float farClipPlane = 100;
    Vector4 viewportRect = {0, 0, 1, 1};
    Vector4 cleanColor = {0, 0, 0, 1};

    RenderingPath renderingPath = RenderingPath::Forward;

    PassRenderSettings passRenderSettings = {true, true, true, true};
    PassCollectSettings collectSettings = {};

    Ref<Framebuffer> customRenderOut = nullptr;

    //TODO: Implement Later
    /*
    std::vector<Ref<RenderPass>> passes;
    std::vector<CommandBufferEntry> commandBuffers;

    void AddPass(Ref<RenderPass> pass){ passes.push_back(pass); }
    void AddCommandBuffer(int event, Ref<CommandBuffer> cmd){ commandBuffers.push_back({event, cmd}); }
    */

    Camera GetCamera();
    
    void UpdateCameraData(TransformComponent& transform, int width, int height);

    static void OnGui(Entity& e, Scene& scene);
    static void CreateLuaBind(sol::state& lua);

    template <class Archive>
    void serialize(Archive & ar){
        ArchiveDump(ar, CEREAL_NVP(type));
        ArchiveDump(ar, CEREAL_NVP(isMain));
        ArchiveDump(ar, CEREAL_NVP(orthographicSize));
        ArchiveDump(ar, CEREAL_NVP(fieldOfView));
        ArchiveDump(ar, CEREAL_NVP(nearClipPlane));
        ArchiveDump(ar, CEREAL_NVP(farClipPlane));
        ArchiveDump(ar, CEREAL_NVP(cleanColor));
        ArchiveDump(ar, CEREAL_NVP(renderingPath));
        ArchiveDump(ar, CEREAL_NVP(passRenderSettings));
        ArchiveDump(ar, CEREAL_NVP(collectSettings));
    }

private:
    Camera camera;
};

}

/*namespace cereal {                                                                                                                               
    template<class Archive> inline std::string save_minimal(Archive&, const OD::CameraComponent::Type& h){ return std::string(magic_enum::enum_name(h)); }    
    template<class Archive> inline void load_minimal(const Archive&, OD::CameraComponent::Type& enumType, const std::string& str){                     
        enumType = magic_enum::enum_cast<OD::CameraComponent::Type>(str).value();                                                                
    }                                                                                                                           
}*/