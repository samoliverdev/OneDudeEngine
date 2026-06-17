#include "LineRenderer.h"
#include <OD/RenderPipeline/MeshRendererComponent.h>

namespace Standard{

void LineRenderer::UpdateMesh(Scene& scene, Entity e){
    MeshRendererComponent& meshRenderer = scene.AddOrGetComponent<MeshRendererComponent>(e);
    meshRenderer.material = material;
    
    if(meshRenderer.mesh == nullptr){
        meshRenderer.mesh = CreateRef<Mesh>();
        /*meshRenderer.mesh->vertices.push_back({-0.1f, -0.1f, 0});
        meshRenderer.mesh->vertices.push_back({0.1f, -0.1f, 0});
        meshRenderer.mesh->vertices.push_back({-0.1f, 0.1f, 0});
        meshRenderer.mesh->vertices.push_back({0.1f, 0.1f, 0});

        meshRenderer.mesh->indices.push_back(0);
        meshRenderer.mesh->indices.push_back(1);
        meshRenderer.mesh->indices.push_back(3);

        meshRenderer.mesh->indices.push_back(0);
        meshRenderer.mesh->indices.push_back(2);
        meshRenderer.mesh->indices.push_back(3);

        meshRenderer.mesh->CalculateNormals();
        meshRenderer.mesh->Submit();*/
    }

    if(points.size() < 2){
        // Clear mesh if not enough points to form a line
        meshRenderer.mesh->vertices.clear();
        meshRenderer.mesh->uv.clear();
        meshRenderer.mesh->indices.clear();
        meshRenderer.mesh->Submit();
        return;
    }
    
    if(points.size() < 2){
        //_mesh.Clear();
        return;
    }
    
    Entity mainCam = scene.GetMainCamera();
    if(mainCam == EntityNull) return;

    TransformComponent trans = scene.GetComponent<TransformComponent>(e);
    TransformComponent mainCamtrans = scene.GetComponent<TransformComponent>(mainCam);

    meshRenderer.mesh->vertices.clear();
    meshRenderer.mesh->uv.clear();
    meshRenderer.mesh->indices.clear();
    //meshRenderer.mesh->colors.clear();

    for(int i = 0; i < points.size(); i++){
        Vector3 currentPoint = points[i];
        
        if(useWorldSpace){
            currentPoint = trans.InverseTransformPoint(currentPoint);
        }

        Vector3 direction = Vector3Zero;

        // 1. Determine line direction at this segment
        if(i == 0){
            Vector3 nextPoint = useWorldSpace ? trans.InverseTransformPoint(points[i + 1]) : points[i + 1];
            direction = math::normalize(nextPoint - currentPoint);
        } else if (i == points.size() - 1){
            Vector3 prevPoint = useWorldSpace ? trans.InverseTransformPoint(points[i - 1]) : points[i - 1];
            direction = math::normalize(currentPoint - prevPoint);
        } else {
            Vector3 prevPoint = useWorldSpace ? trans.InverseTransformPoint(points[i - 1]) : points[i - 1];
            Vector3 nextPoint = useWorldSpace ? trans.InverseTransformPoint(points[i + 1]) : points[i + 1];
            
            Vector3 dirToPrev = math::normalize(currentPoint - prevPoint);
            Vector3 dirToNext = (nextPoint - currentPoint);
            direction = (dirToPrev + dirToNext);
        }

        // 2. Calculate the normal (extrusion direction)
        Vector3 normal = Vector3Zero;

        if(cameraFacing){
            // Get the vector from the point to the camera (in local space)
            Vector3 camPosLocal = trans.InverseTransformPoint(mainCamtrans.Position());
            Vector3 toCamera = math::normalize(camPosLocal - currentPoint);

            // The cross product of the line direction and the vector to the camera 
            // gives us a vector that is perfectly perpendicular to both.
            normal = math::normalize(math::cross(direction, toCamera));
        } else {
            // Default flat 2D fallback (XY plane) if camera facing is turned off
            //normal = new Vector3(-direction.y, direction.x, 0f).normalized;

            // FIX: Use a 3D cross product with an 'Up' direction instead of a 2D math assumption.
            // This creates a reliable "ribbon" effect in 3D space.
            Vector3 localUp = useWorldSpace ? trans.InverseTransformDirection(worldUpVector) : worldUpVector;
            
            // If the line is pointing directly up, change the cross target to avoid a zero vector
            if(math::abs(math::dot(direction, localUp)) > 0.99f){
                localUp = useWorldSpace ? trans.InverseTransformDirection(Vector3Forward) : Vector3Forward;
            }

            normal = math::normalize(math::cross(direction, localUp));
        }

        // 3. Extrude vertices outwards to create width
        Vector3 vertexLeft = currentPoint + normal * (thickness * 0.5f);
        Vector3 vertexRight = currentPoint - normal * (thickness * 0.5f);

        meshRenderer.mesh->vertices.push_back(vertexLeft);
        meshRenderer.mesh->vertices.push_back(vertexRight);

        // UVs
        float progress = (float)i / (points.size() - 1);
        /*_uvs.Add(new Vector2(0f, progress));
        _uvs.Add(new Vector2(1f, progress));*/
        meshRenderer.mesh->uv.push_back(Vector3(progress, 0.0f, 0));
        meshRenderer.mesh->uv.push_back(Vector3(progress, 1.0f, 0));

        // Evaluate gradient color at this specific point's progress percentage
        /*Color segmentColor = colorGradient.Evaluate(progress);
        _colors.Add(segmentColor); // Left vertex color
        _colors.Add(segmentColor); // Right vertex color*/

        // 4. Construct triangles
        if(i > 0){
            int currentLeft = i * 2;
            int currentRight = i * 2 + 1;
            int prevLeft = (i - 1) * 2;
            int prevRight = (i - 1) * 2 + 1;

            meshRenderer.mesh->indices.push_back(prevLeft);
            meshRenderer.mesh->indices.push_back(currentLeft);
            meshRenderer.mesh->indices.push_back(prevRight);

            meshRenderer.mesh->indices.push_back(prevRight);
            meshRenderer.mesh->indices.push_back(currentLeft);
            meshRenderer.mesh->indices.push_back(currentRight);
        }
    }

    meshRenderer.mesh->CalculateNormals();
    meshRenderer.mesh->Submit();
    meshRenderer.UpdateAABB();
}

}