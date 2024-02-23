#pragma once
#include "OD/Scene/Scene.h"
#include "OD/Graphics/Mesh.h"
#include "OD/Graphics/Material.h"

namespace OD{

struct MeshRendererComponent{
    Ref<Mesh> mesh;
    Ref<Material> material;
    AABB boundingVolume;

    inline static void OnGui(Entity& e){}

    template<class Archive>
    void serialize(Archive& ar){}

    void UpdateAABB(Vector3 scale = Vector3One){
        if(mesh == nullptr){
            boundingVolume = AABB(Vector3Zero, 1, 1, 1);
            return;
        }

        Vector3 minAABB = Vector3(std::numeric_limits<float>::max());
        Vector3 maxAABB = Vector3(std::numeric_limits<float>::min());
        for(auto& vertex : mesh->vertices){
            vertex *= scale;
        
            minAABB.x = std::min(minAABB.x, vertex.x);
            minAABB.y = std::min(minAABB.y, vertex.y);
            minAABB.z = std::min(minAABB.z, vertex.z);

            maxAABB.x = std::max(maxAABB.x, vertex.x);
            maxAABB.y = std::max(maxAABB.y, vertex.y);
            maxAABB.z = std::max(maxAABB.z, vertex.z);
        }
        boundingVolume = AABB(minAABB, maxAABB);
    }

    AABB GetGlobalAABB(TransformComponent& transform){
        //Get global scale thanks to our transform
        const Vector3 globalCenter{ transform.GlobalModelMatrix() * Vector4(boundingVolume.center, 1) };

        // Scaled orientation
        const Vector3 right = transform.Right() * boundingVolume.extents.x;
        const Vector3 up = transform.Up() * boundingVolume.extents.y;
        const Vector3 forward = transform.Forward() * boundingVolume.extents.z;

        const float newIi = math::abs(math::dot(Vector3{ 1.f, 0.f, 0.f }, right)) +
            math::abs(math::dot(Vector3{ 1.f, 0.f, 0.f }, up)) +
            math::abs(math::dot(Vector3{ 1.f, 0.f, 0.f }, forward));

        const float newIj = math::abs(math::dot(Vector3{ 0.f, 1.f, 0.f }, right)) +
            math::abs(math::dot(Vector3{ 0.f, 1.f, 0.f }, up)) +
            math::abs(math::dot(Vector3{ 0.f, 1.f, 0.f }, forward));

        const float newIk = math::abs(math::dot(Vector3{ 0.f, 0.f, 1.f }, right)) +
            math::abs(math::dot(Vector3{ 0.f, 0.f, 1.f }, up)) +
            math::abs(math::dot(Vector3{ 0.f, 0.f, 1.f }, forward));

        AABB result = AABB(globalCenter, newIi, newIj, newIk);
        result.Expand(transform.Scale());
        return result;
    }
};

}