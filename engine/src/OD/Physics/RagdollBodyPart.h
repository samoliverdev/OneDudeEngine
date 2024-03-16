#pragma once
#include "OD/Defines.h"
#include "OD/Core/Transform.h"
#include "OD/Core/ImGui.h"
#include "OD/Serialization/Serialization.h"
#include "OD/Serialization/ImGuiArchive.h"

namespace OD{

struct RagdollBodyPart{
    enum class CollisionType{Box, Sphere, Capsule};

    CollisionType collisionType;
    Vector3 size = {1,1,1};
    float radius = 1;
    float height;
    int parent = -1;
    int animatorBone = -1;
    
    Vector3 frameA;
    Vector3 frameB;

    Quaternion frameARot;
    Quaternion frameBRot;

    Vector3 center;
    Quaternion rot;

    Transform trans;
    Vector3 pivot;

    template<class Archive>
    void serialize(Archive& ar){
        ArchiveDump(ar, CEREAL_NVP(collisionType));
        ArchiveDump(ar, CEREAL_NVP(size));
        ArchiveDump(ar, CEREAL_NVP(radius));
        ArchiveDump(ar, CEREAL_NVP(height));
        ArchiveDump(ar, CEREAL_NVP(parent));
        ArchiveDump(ar, CEREAL_NVP(animatorBone));
    }
};

namespace GUI{
    inline void DrawUI(const char* name, RagdollBodyPart& value, ImGuiArchive2& ar, Options opt){
        ar.elementCount += 1;
        ImGui::PushID(ar.elementCount);
        
        if(ImGui::TreeNodeEx(name, ImGuiTreeNodeFlags_DefaultOpen)){
            GUI::DrawUI("collisionType", value.collisionType, ar, opt);
        
            if(value.collisionType == RagdollBodyPart::CollisionType::Box){
                GUI::DrawUI("size", value.size, ar, opt);
            } else if(value.collisionType == RagdollBodyPart::CollisionType::Sphere){
                GUI::DrawUI("radius", value.radius, ar, opt);
            } else if(value.collisionType == RagdollBodyPart::CollisionType::Capsule){
                GUI::DrawUI("radius", value.radius, ar, opt);
                GUI::DrawUI("height", value.height, ar, opt);
            }
            
            GUI::DrawUI("parent", value.parent, ar, opt);
            GUI::DrawUI("animatorBone", value.animatorBone, ar, opt);

            ImGui::TreePop();
        }

        ImGui::PopID();
    }
}

}