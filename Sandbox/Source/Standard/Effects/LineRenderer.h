#pragma once
#include <OD/Graphics/Mesh.h>
#include <OD/Graphics/Material.h>
#include <OD/Scene/Scene.h>

using namespace OD;

namespace Standard{

struct LineRenderer{
    bool isDirty = true;

    std::vector<Vector3> points;
    float thickness = 0.1f;
    bool useWorldSpace = true;

    bool cameraFacing = true;
    Vector3 worldUpVector = Vector3Up;

    Ref<Material> material = nullptr;

    void UpdateMesh(Scene& scene, Entity e);
    
    template <class Archive>
    void serialize(Archive& ar){
        ArchiveDumpNVP(ar, isDirty);

        ArchiveDumpNVP(ar, points);
        ArchiveDumpNVP(ar, thickness);
        ArchiveDumpNVP(ar, useWorldSpace);
        ArchiveDumpNVP(ar, cameraFacing);
        ArchiveDumpNVP(ar, worldUpVector);

        ResourceRefSerialize<Material> _material(material);
        ArchiveDumpNamed(ar, "material", _material);
    }
};

}