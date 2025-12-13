#pragma once
#include <OD/Graphics/Mesh.h>
#include <OD/Graphics/Material.h>
#include <OD/Scene/Scene.h>

using namespace OD;

namespace Standard{

enum class MeshPivot {
    Center,
    Corner
};

Ref<Mesh> CreatePlaneMesh(Vector2 size, IVector2 resolution, MeshPivot pivot);
Ref<Mesh> CreateCubeMesh(Vector3 size, IVector3 resolution, MeshPivot pivot);
Ref<Mesh> CreateCylinderMesh(float radius, float height, IVector2 resolution, MeshPivot pivot);
Ref<Mesh> CreateConeMesh(float radius, float height, int radialSegments, int heightSegments, bool addBaseCap, MeshPivot pivot);
Ref<Mesh> CreateSphereMesh(float radius, IVector2 resolution, MeshPivot pivot);
Ref<Mesh> CreateRampMesh(Vector3 size, MeshPivot pivot, bool includeBottomFace = true);

struct Greyboxing{
    enum class Shape{
        Plane,
        Cube,
        Cylinder,
        Cone,
        Sphere,
        Ramp
    };
    
    Shape shape = Shape::Plane;
    MeshPivot pivot = MeshPivot::Center;

    Vector2 planeSize = {1, 1};
    Vector3 cubeSize = {1, 1, 1};
    IVector3 resolution = {8, 8, 8};

    float radius = 0.5f;
    float height = 2;

    Ref<Material> material = nullptr;
    Ref<Mesh> mesh = nullptr;

    bool isDirty = true;

    void UpdateMesh(Scene& scene, Entity e, Ref<Material> defaultMaterial);

    static void OnGui(Entity& e, Scene& scene);

    template <class Archive>
    void serialize(Archive& ar){
        ArchiveDumpNVP(ar, shape);
        ArchiveDumpNVP(ar, pivot);
        ArchiveDumpNVP(ar, planeSize);
        ArchiveDumpNVP(ar, cubeSize);
        ArchiveDumpNVP(ar, resolution);
        ArchiveDumpNVP(ar, radius);
        ArchiveDumpNVP(ar, height);
    }
};

}