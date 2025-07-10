#include "Greyboxing.h"
#include "OD/RenderPipeline/MeshRendererComponent.h"
#include "OD/Core/ImGui.h"

namespace Standard{

Ref<Material> defaultMaterial = nullptr;

Ref<Mesh> CreatePlaneMesh(Vector2 size, IVector2 resolution, MeshPivot pivot){
    Ref<Mesh> mesh = CreateRef<Mesh>();

    int xSegments = std::max(1, resolution.x);
    int ySegments = std::max(1, resolution.y);
    float xStep = size.x / xSegments;
    float yStep = size.y / ySegments;

    Vector2 originOffset = (pivot == MeshPivot::Center) ? Vector2(-size.x * 0.5f, -size.y * 0.5f) : Vector2(0, 0);

    for (int y = 0; y <= ySegments; ++y) {
        for (int x = 0; x <= xSegments; ++x) {
            float px = x * xStep + originOffset.x;
            float py = y * yStep + originOffset.y;

            mesh->vertices.emplace_back(px, 0.0f, py);
            //mesh->uv.emplace_back(static_cast<float>(x) / xSegments, static_cast<float>(y) / ySegments, 0.0f);

            float uvScale = 1.0f; // 1 texel = 1m
            mesh->uv.push_back(Vector3(px * uvScale, py * uvScale, 0.0f));

            //mesh->uv.emplace_back(px, py, 0.0f);

            mesh->normals.emplace_back(0.0f, 1.0f, 0.0f);
        }
    }

    for (int y = 0; y < ySegments; ++y) {
        for (int x = 0; x < xSegments; ++x) {
            int i = y * (xSegments + 1) + x;
            mesh->indices.push_back(i);
            mesh->indices.push_back(i + xSegments + 1);
            mesh->indices.push_back(i + 1);

            mesh->indices.push_back(i + 1);
            mesh->indices.push_back(i + xSegments + 1);
            mesh->indices.push_back(i + xSegments + 2);
        }
    }

    mesh->CalculateTangent(); // Optional
    mesh->Submit();

    return mesh;
}

Ref<Mesh> CreateCubeMesh(Vector3 size, IVector3 resolution, MeshPivot pivot) {
    Ref<Mesh> mesh = CreateRef<Mesh>();

    Vector3 offset = (pivot == MeshPivot::Center) ? -size * 0.5f : Vector3(0);

    auto addFace = [&](Vector3 faceOrigin, Vector3 rightDir, Vector3 upDir, float faceWidth, float faceHeight, int resX, int resY) {
        int baseIndex = static_cast<int>(mesh->vertices.size());

        for (int y = 0; y <= resY; ++y) {
            for (int x = 0; x <= resX; ++x) {
                float fx = static_cast<float>(x) / resX;
                float fy = static_cast<float>(y) / resY;

                Vector3 localPos = faceOrigin + rightDir * (fx * faceWidth) + upDir * (fy * faceHeight);
                Vector3 worldPos = localPos + offset;

                mesh->vertices.push_back(worldPos);
                mesh->uv.push_back(Vector3(fx * faceWidth, fy * faceHeight, 0.0f));

                Vector3 normal = glm::normalize(glm::cross(rightDir, upDir));
                mesh->normals.push_back(normal);
            }
        }

        for (int y = 0; y < resY; ++y) {
            for (int x = 0; x < resX; ++x) {
                int i = baseIndex + y * (resX + 1) + x;
                mesh->indices.push_back(i);
                mesh->indices.push_back(i + 1);
                mesh->indices.push_back(i + resX + 1);

                mesh->indices.push_back(i + 1);
                mesh->indices.push_back(i + resX + 2);
                mesh->indices.push_back(i + resX + 1);
            }
        }
    };

    int xRes = std::max(1, resolution.x);
    int yRes = std::max(1, resolution.y);
    int zRes = std::max(1, resolution.z);

    // +Y (top) - corrigido
    addFace(
        Vector3(size.x, size.y, 0),
        Vector3(-1, 0, 0),
        Vector3(0, 0, 1),
        size.x, size.z,
        xRes, zRes
    );

    // -Y (bottom) - corrigido
    addFace(
        Vector3(0, 0, 0),
        Vector3(1, 0, 0),
        Vector3(0, 0, 1),
        size.x, size.z,
        xRes, zRes
    );

    // +Z (front)
    addFace(
        Vector3(0, 0, size.z),
        Vector3(1, 0, 0),
        Vector3(0, 1, 0),
        size.x, size.y,
        xRes, yRes
    );

    // -Z (back)
    addFace(
        Vector3(size.x, 0, 0),
        Vector3(-1, 0, 0),
        Vector3(0, 1, 0),
        size.x, size.y,
        xRes, yRes
    );

    // +X (right)
    addFace(
        Vector3(size.x, 0, size.z),
        Vector3(0, 0, -1),
        Vector3(0, 1, 0),
        size.z, size.y,
        zRes, yRes
    );

    // -X (left)
    addFace(
        Vector3(0, 0, 0),
        Vector3(0, 0, 1),
        Vector3(0, 1, 0),
        size.z, size.y,
        zRes, yRes
    );

    mesh->CalculateTangent();
    mesh->Submit();

    return mesh;
}

Ref<Mesh> CreateCylinderMesh(float radius, float height, IVector2 resolution, MeshPivot pivot) {
    Ref<Mesh> mesh = CreateRef<Mesh>();

    int radialSegments = std::max(3, resolution.x);
    int heightSegments = std::max(1, resolution.y);

    float angleStep = glm::two_pi<float>() / radialSegments;
    float heightStep = height / heightSegments;

    float yOffset = (pivot == MeshPivot::Center) ? -height * 0.5f : 0.0f;

    // -----------------------
    // Body (side) generation
    // -----------------------
    for (int y = 0; y <= heightSegments; ++y) {
        float fy = static_cast<float>(y) / heightSegments;
        float yPos = fy * height + yOffset;

        for (int i = 0; i <= radialSegments; ++i) {
            float angle = i * angleStep;
            float x = std::cos(angle) * radius;
            float z = std::sin(angle) * radius;

            mesh->vertices.emplace_back(x, yPos, z);
            mesh->normals.emplace_back(glm::normalize(Vector3(x, 0, z)));
            mesh->uv.emplace_back(Vector3(angle * radius, fy * height, 0.0f)); // greybox UV

            if (y < heightSegments && i < radialSegments) {
                int current = y * (radialSegments + 1) + i;
                int next = current + radialSegments + 1;

                mesh->indices.push_back(current);
                mesh->indices.push_back(next);
                mesh->indices.push_back(current + 1);

                mesh->indices.push_back(current + 1);
                mesh->indices.push_back(next);
                mesh->indices.push_back(next + 1);
            }
        }
    }

    // -----------------------
    // Top / Bottom Caps
    // -----------------------
    auto addCap = [&](bool top) {
        int baseIndex = static_cast<int>(mesh->vertices.size());
        float y = top ? (height + yOffset) : yOffset;
        Vector3 normal = top ? Vector3(0, 1, 0) : Vector3(0, -1, 0);

        mesh->vertices.push_back(Vector3(0, y, 0)); // center
        mesh->normals.push_back(normal);
        mesh->uv.push_back(Vector3(0.5f * radius * 2.0f, 0.5f * radius * 2.0f, 0.0f)); // centro da UV (metade do espaço)

        for (int i = 0; i <= radialSegments; ++i) {
            float angle = i * angleStep;
            float x = std::cos(angle) * radius;
            float z = std::sin(angle) * radius;

            mesh->vertices.emplace_back(x, y, z);
            mesh->normals.push_back(normal);

            // UV centrado entre [0, radius*2] depois você normaliza no shader ou aqui
            float u = (x + radius); // [0, 2r]
            float v = (z + radius); // [0, 2r]
            mesh->uv.push_back(Vector3(u, v, 0.0f));
        }

        for (int i = 0; i < radialSegments; ++i) {
            int centerIndex = baseIndex;
            int curr = baseIndex + 1 + i;
            int next = baseIndex + 1 + ((i + 1) % (radialSegments + 1));

            if (top) {
                // ⚠️ TOP: inverter para anti-horário olhando de cima
                mesh->indices.push_back(centerIndex);
                mesh->indices.push_back(next);
                mesh->indices.push_back(curr);
            } else {
                // ⚠️ BOTTOM: inverter para anti-horário olhando de baixo
                mesh->indices.push_back(centerIndex);
                mesh->indices.push_back(curr);
                mesh->indices.push_back(next);
            }
        }
    };

    // Top cap
    addCap(true);

    // Bottom cap
    addCap(false);

    mesh->CalculateTangent(); // opcional
    mesh->Submit();

    return mesh;
}

Ref<Mesh> CreateConeMesh(float radius, float height, int radialSegments, int heightSegments, bool addBaseCap, MeshPivot pivot) {
    Ref<Mesh> mesh = CreateRef<Mesh>();

    radialSegments = std::max(3, radialSegments);
    heightSegments = std::max(1, heightSegments);

    float angleStep = glm::two_pi<float>() / radialSegments;
    float yOffset = (pivot == MeshPivot::Center) ? -height * 0.5f : 0.0f;

    // Corpo do cone
    for (int y = 0; y <= heightSegments; ++y) {
        float fy = static_cast<float>(y) / heightSegments;
        float currRadius = radius * (1.0f - fy);
        float yPos = fy * height + yOffset;

        for (int i = 0; i <= radialSegments; ++i) {
            float angle = i * angleStep;
            float x = std::cos(angle);
            float z = std::sin(angle);

            Vector3 pos = Vector3(x * currRadius, yPos, z * currRadius);
            Vector3 normal = glm::normalize(Vector3(x, radius / height, z)); // aproximação

            mesh->vertices.push_back(pos);
            mesh->normals.push_back(normal);
            mesh->uv.push_back(Vector3(angle * radius, fy * height, 0.0f)); // greybox UV

            if (y < heightSegments && i < radialSegments) {
                int current = y * (radialSegments + 1) + i;
                int next = current + radialSegments + 1;

                mesh->indices.push_back(current);
                mesh->indices.push_back(next);
                mesh->indices.push_back(current + 1);

                mesh->indices.push_back(current + 1);
                mesh->indices.push_back(next);
                mesh->indices.push_back(next + 1);
            }
        }
    }

    // Base (cap)
    if (addBaseCap) {
        int baseIndex = static_cast<int>(mesh->vertices.size());
        float y = yOffset;
        mesh->vertices.push_back(Vector3(0, y, 0));
        mesh->normals.push_back(Vector3(0, -1, 0));
        mesh->uv.push_back(Vector3(radius, radius, 0)); // centro

        for (int i = 0; i <= radialSegments; ++i) {
            float angle = i * angleStep;
            float x = std::cos(angle) * radius;
            float z = std::sin(angle) * radius;

            mesh->vertices.emplace_back(x, y, z);
            mesh->normals.push_back(Vector3(0, -1, 0));
            mesh->uv.push_back(Vector3(x + radius, z + radius, 0));

            if (i < radialSegments) {
                int centerIndex = baseIndex;
                int curr = baseIndex + 1 + i;
                int next = baseIndex + 1 + ((i + 1) % (radialSegments + 1));

                mesh->indices.push_back(centerIndex);
                mesh->indices.push_back(curr);
                mesh->indices.push_back(next);
            }
        }
    }

    mesh->CalculateTangent();
    mesh->Submit();
    return mesh;
}

Ref<Mesh> CreateSphereMesh(float radius, IVector2 resolution, MeshPivot pivot) {
    Ref<Mesh> mesh = CreateRef<Mesh>();

    int latSegments = std::max(2, resolution.y);
    int lonSegments = std::max(3, resolution.x);

    float yOffset = (pivot == MeshPivot::Corner) ? radius : 0.0f;

    for (int lat = 0; lat <= latSegments; ++lat) {
        float v = static_cast<float>(lat) / latSegments;
        float phi = glm::pi<float>() * v;

        for (int lon = 0; lon <= lonSegments; ++lon) {
            float u = static_cast<float>(lon) / lonSegments;
            float theta = glm::two_pi<float>() * u;

            float x = std::sin(phi) * std::cos(theta);
            float y = std::cos(phi);
            float z = std::sin(phi) * std::sin(theta);

            Vector3 normal = Vector3(x, y, z);
            Vector3 position = normal * radius;
            position.y += yOffset;

            mesh->vertices.push_back(position);
            mesh->normals.push_back(normal);
            mesh->uv.push_back(Vector3(u * radius * glm::two_pi<float>(), v * radius * glm::pi<float>(), 0.0f));
        }
    }

    for (int lat = 0; lat < latSegments; ++lat) {
        for (int lon = 0; lon < lonSegments; ++lon) {
            int i0 = lat * (lonSegments + 1) + lon;
            int i1 = i0 + 1;
            int i2 = i0 + lonSegments + 1;
            int i3 = i2 + 1;

            // ⬅️ Triângulo 1
            mesh->indices.push_back(i0);
            mesh->indices.push_back(i1);
            mesh->indices.push_back(i2);

            // ⬅️ Triângulo 2
            mesh->indices.push_back(i1);
            mesh->indices.push_back(i3);
            mesh->indices.push_back(i2);
        }
    }

    mesh->CalculateTangent();
    mesh->Submit();
    return mesh;
}

void Greyboxing::OnGui(Entity& e, Scene& scene){
    Greyboxing& greyboxing = scene.GetComponent<Greyboxing>(e);

    IMGUI_BeginGlobalTable("Greyboxing");

    IMGUI_GlobalTableRow("Type", {
        if(ImGui::DrawEnumCombo<Greyboxing::Shape>("##Type", &greyboxing.shape)){
            greyboxing.isDirty = true;
        }}
    );

    if(greyboxing.shape == Greyboxing::Shape::Plane){
        IMGUI_GlobalTableRow("Pivot", {
            if(ImGui::DrawEnumCombo<MeshPivot>("##Pivot", &greyboxing.pivot)){
                greyboxing.isDirty = true;
            }}
        );
        IMGUI_GlobalTableRow("Size", {
            if(ImGui::DragFloat2("##Size", &greyboxing.planeSize.x)){
                greyboxing.isDirty = true;
            }}
        );
        IMGUI_GlobalTableRow("resolution", {
            if(ImGui::DragInt2("##resolution", &greyboxing.resolution.x)){
                greyboxing.isDirty = true;
            }}
        );
    }

    if(greyboxing.shape == Greyboxing::Shape::Cube){
        IMGUI_GlobalTableRow("Pivot", {
            if(ImGui::DrawEnumCombo<MeshPivot>("##Pivot", &greyboxing.pivot)){
                greyboxing.isDirty = true;
            }}
        );
        IMGUI_GlobalTableRow("Size", {
            if(ImGui::DragFloat3("##Size", &greyboxing.cubeSize.x)){
                greyboxing.isDirty = true;
            }} 
        );
        IMGUI_GlobalTableRow("resolution", {
            if(ImGui::DragInt3("##resolution", &greyboxing.resolution.x)){
                greyboxing.isDirty = true;
            }}
        );
    }

    if(greyboxing.shape == Greyboxing::Shape::Cylinder){
        IMGUI_GlobalTableRow("Pivot", {
            if(ImGui::DrawEnumCombo<MeshPivot>("##Pivot", &greyboxing.pivot)){
                greyboxing.isDirty = true;
            }}
        );
        IMGUI_GlobalTableRow("radius", {
            if(ImGui::DragFloat("##radius", &greyboxing.radius)){
                greyboxing.isDirty = true;
            }} 
        );
        IMGUI_GlobalTableRow("height", {
            if(ImGui::DragFloat("##height", &greyboxing.height)){
                greyboxing.isDirty = true;
            }} 
        );
        IMGUI_GlobalTableRow("resolution", {
            if(ImGui::DragInt2("##resolution", &greyboxing.resolution.x)){
                greyboxing.isDirty = true;
            }}
        );
    }

    if(greyboxing.shape == Greyboxing::Shape::Cone){
        IMGUI_GlobalTableRow("Pivot", {
            if(ImGui::DrawEnumCombo<MeshPivot>("##Pivot", &greyboxing.pivot)){
                greyboxing.isDirty = true;
            }}
        );
        IMGUI_GlobalTableRow("radius", {
            if(ImGui::DragFloat("##radius", &greyboxing.radius)){
                greyboxing.isDirty = true;
            }} 
        );
        IMGUI_GlobalTableRow("height", {
            if(ImGui::DragFloat("##height", &greyboxing.height)){
                greyboxing.isDirty = true;
            }} 
        );
        IMGUI_GlobalTableRow("resolution", {
            if(ImGui::DragInt2("##resolution", &greyboxing.resolution.x)){
                greyboxing.isDirty = true;
            }}
        );
    }

    if(greyboxing.shape == Greyboxing::Shape::Sphere){
        IMGUI_GlobalTableRow("Pivot", {
            if(ImGui::DrawEnumCombo<MeshPivot>("##Pivot", &greyboxing.pivot)){
                greyboxing.isDirty = true;
            }}
        );
        IMGUI_GlobalTableRow("radius", {
            if(ImGui::DragFloat("##radius", &greyboxing.radius)){
                greyboxing.isDirty = true;
            }} 
        );
        IMGUI_GlobalTableRow("resolution", {
            if(ImGui::DragInt2("##resolution", &greyboxing.resolution.x)){
                greyboxing.isDirty = true;
            }}
        );
    }

    IMGUI_EndGlobalTable();
}

void Greyboxing::UpdateMesh(Scene& scene, Entity e){
    if(defaultMaterial == nullptr){
        defaultMaterial = CreateRef<Material>(AssetManager::Get().LoadAsset<Shader>("Engine/Shaders/Lit.glsl"));
        defaultMaterial->SetTexture("mainTex", AssetManager::Get().LoadAsset<Texture2D>("StandardAsset/Textures/GreyboxTextures/greybox_grey_grid.png"));
    }
    MeshRendererComponent& meshRenderer = scene.AddOrGetComponent<MeshRendererComponent>(e);
    meshRenderer.material = material == nullptr ? defaultMaterial : material; 

    if(resolution.x <= 0) resolution.x = 1;
    if(resolution.y <= 0) resolution.y = 1;

    if(shape == Greyboxing::Shape::Plane){
        meshRenderer.mesh = CreatePlaneMesh(planeSize, resolution, pivot);
        meshRenderer.UpdateAABB();
    }
    if(shape == Greyboxing::Shape::Cube){
        meshRenderer.mesh = CreateCubeMesh(cubeSize, resolution, pivot);
        meshRenderer.UpdateAABB();
    }
    if(shape == Greyboxing::Shape::Cylinder){
        meshRenderer.mesh = CreateCylinderMesh(radius, height, resolution, pivot);
        meshRenderer.UpdateAABB();
    }
    if(shape == Greyboxing::Shape::Cone){
        meshRenderer.mesh = CreateConeMesh(radius, height, resolution.x, resolution.y, true, pivot);
        meshRenderer.UpdateAABB();
    }
    if(shape == Greyboxing::Shape::Sphere){
        meshRenderer.mesh = CreateSphereMesh(radius, resolution, pivot);
        meshRenderer.UpdateAABB();
    }
}

}