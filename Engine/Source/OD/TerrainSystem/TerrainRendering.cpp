#include "OD/pch.h"
#include "TerrainRendering.h"
#include "TerrainComponents.h"
#include "OD/RenderPipeline/RenderContext.h"
#include "OD/Graphics/Mesh.h"
#include "OD/Graphics/Material.h"
#include "OD/Graphics/Geometry.h"
#include <chrono>
#include <cstring>

namespace OD{
namespace{
using Clock = std::chrono::steady_clock;

struct TerrainGpu{
    Ref<Material> color, shadow;
    Ref<Texture2D> heights, paint;
    WeakRef<Heightmap> heightSource;
    WeakRef<TerrainPaintMap> paintSource;
    uint64_t heightRevision = 0, paintRevision = 0;
    bool fullPrecision = true;
    Clock::time_point lastHeight{}, lastPaint{};
};

template <class T>
bool UploadRegion(Ref<Texture2D>& texture,
                  const std::vector<T>& pixels,
                  int width,
                  int channels,
                  TerrainDirtyRegion r,
                  TextureDataType type){
    int w = r.x1 - r.x0, h = r.z1 - r.z0;
    if(!texture || w <= 0 || h <= 0)
        return false;
    std::vector<T> packed(size_t(w) * h * channels);
    for(int y = 0; y < h; ++y)
        std::copy_n(pixels.data() + (size_t(y + r.z0) * width + r.x0) * channels,
                    size_t(w) * channels,
                    packed.data() + size_t(y) * w * channels);
    return texture->UpdateRegion(r.x0, r.z0, w, h, packed.data(), type);
}

void SetTerrainUniforms(Material& mat, const TerrainDataComponent& data, const Matrix4& root, const TerrainGpu& gpu){
    mat.SetTexture("heightMap", gpu.heights);
    mat.SetTexture("paintMap", gpu.paint);
    mat.SetVector3("terrainSize", data.size);
    mat.SetVector2("heightSize", Vector2(data.heightmap->width, data.heightmap->height));
    mat.SetMatrix4("terrainModel", root);
    mat.SetMatrix4("terrainNormal", math::transpose(math::inverse(root)));
}
}

struct TerrainQuadtreeRenderer::State{
    Scene* scene = nullptr;
    std::unordered_map<Entity, TerrainGpu> terrains;
    std::unordered_map<int, std::array<Ref<Mesh>, 16>> grids;
    Ref<Texture2D> white, normal, defaultPaint;

    Ref<Mesh> Grid(int cells, int mask){
        auto& mesh = grids[cells][mask];
        if(mesh)
            return mesh;
        mesh = ResourceManager::Get().Create<Mesh>();
        for(int z = 0; z <= cells; ++z)
            for(int x = 0; x <= cells; ++x){
                mesh->vertices.emplace_back(float(x) / cells, 0, float(z) / cells);
                mesh->uv.emplace_back(float(x) / cells, float(z) / cells, 0);
                mesh->normals.emplace_back(0, 1, 0);
                mesh->tangents.emplace_back(1, 0, 0);
            }
        mesh->indices = TerrainCpu::GridIndices(cells, mask);
        mesh->Submit();
        return mesh;
    }
};

TerrainQuadtreeRenderer::TerrainQuadtreeRenderer() : state(std::make_unique<State>()){
}

TerrainQuadtreeRenderer::~TerrainQuadtreeRenderer() = default;

void TerrainQuadtreeRenderer::OnCollectCameraRenderData(Scene& scene,
                                                        RenderContext& context,
                                                        const Camera& camera,
                                                        std::vector<RenderData>& out){
    auto& s = *state;
    if(s.scene != &scene){
        s.terrains.clear();
        s.scene = &scene;
    }
    for(auto it = s.terrains.begin(); it != s.terrains.end();){
        if(!scene.IsValid(it->first) || !scene.HasComponent<TerrainDataComponent>(it->first) ||
           !scene.HasComponent<TerrainRendererComponent>(it->first))
            it = s.terrains.erase(it);
        else
            ++it;
    }
    auto view =
        scene.GetRegistry().view<TerrainDataComponent, TerrainRendererComponent, TransformComponent, InfoComponent>(
            entt::exclude<SelfDisable, SkipDraw, HideInEditor>);
    for(auto e : view){
        auto& data = view.get<TerrainDataComponent>(e);
        auto& renderer = view.get<TerrainRendererComponent>(e);
        auto& info = view.get<InfoComponent>(e);
        auto& transform = view.get<TransformComponent>(e);
        renderer.selectedPatches = renderer.submittedPatches = 0;
        if(!info.enable || !renderer.enabled || !data.View().Valid()){
            s.terrains.erase(e);
            continue;
        }
        renderer.Validate();
        Matrix4 root = transform.GlobalModelMatrix();
        if(std::abs(math::determinant(root)) < 1e-10f)
            continue;
        auto& gpu = s.terrains[e];
        auto now = Clock::now();
        bool changedSource = gpu.heightSource.lock() != data.heightmap;
        bool heightChanged =
            changedSource || gpu.heightRevision != data.Revision() || gpu.fullPrecision != renderer.heightTexture32Bit;
        if(heightChanged &&
           (changedSource || !gpu.heights || gpu.fullPrecision != renderer.heightTexture32Bit ||
            std::chrono::duration<float>(now - gpu.lastHeight).count() >= renderer.textureUpdateInterval)){
            auto map = data.heightmap;
            bool recreate = !gpu.heights || changedSource || gpu.fullPrecision != renderer.heightTexture32Bit ||
                            gpu.heights->Width() != unsigned(map->width) ||
                            gpu.heights->Height() != unsigned(map->height);
            bool uploaded = !recreate && UploadRegion(gpu.heights,
                                                      map->data,
                                                      map->width,
                                                      1,
                                                      data.ChangesSince(gpu.heightRevision, false),
                                                      TextureDataType::Float);
            if(!uploaded)
                gpu.heights = Texture2D::CreateFromRaw(
                    map->data.data(),
                    map->width,
                    map->height,
                    TextureDataType::Float,
                    {TextureFilter::Linear,
                     TextureWrapping::ClampToEdge,
                     false,
                     renderer.heightTexture32Bit ? TextureFormat::RED32F : TextureFormat::RED16F},
                    "Terrain heights");
            if(gpu.heights){
                gpu.heightSource = map;
                gpu.heightRevision = data.Revision();
                gpu.fullPrecision = renderer.heightTexture32Bit;
                gpu.lastHeight = now;
            }
        }
        auto paint = data.paintmap;
        bool validPaint = paint && paint->Valid();
        bool newPaint = gpu.paintSource.lock() != paint;
        if(validPaint && (newPaint || gpu.paintRevision != data.PaintRevision()) &&
           (newPaint || !gpu.paint ||
            std::chrono::duration<float>(now - gpu.lastPaint).count() >= renderer.textureUpdateInterval)){
            bool recreate = newPaint || !gpu.paint || gpu.paint->Width() != unsigned(paint->width) ||
                            gpu.paint->Height() != unsigned(paint->height);
            if(recreate || !UploadRegion(gpu.paint,
                                         paint->weights,
                                         paint->width,
                                         4,
                                         data.ChangesSince(gpu.paintRevision, true),
                                         TextureDataType::UnsignedByte))
                gpu.paint = Texture2D::CreateFromRaw(
                    paint->weights.data(),
                    paint->width,
                    paint->height,
                    TextureDataType::UnsignedByte,
                    {TextureFilter::Linear, TextureWrapping::ClampToEdge, false, TextureFormat::RGBA8},
                    "Terrain weights");
            if(gpu.paint){
                gpu.paintSource = paint;
                gpu.paintRevision = data.PaintRevision();
                gpu.lastPaint = now;
            }
        }
        if(!validPaint){
            if(!s.defaultPaint){
                uint8_t bytes[] = {255, 0, 0, 0};
                s.defaultPaint = Texture2D::CreateFromRaw(
                    bytes,
                    1,
                    1,
                    TextureDataType::UnsignedByte,
                    {TextureFilter::Linear, TextureWrapping::ClampToEdge, false, TextureFormat::RGBA8},
                    "Terrain default weights");
            }
            gpu.paint = s.defaultPaint;
            gpu.paintSource.reset();
            gpu.paintRevision = 0;
        }
        if(!gpu.heights || !gpu.paint)
            continue;
        if(!gpu.color){
            auto lit = ResourceManager::Get().LoadByPath<Shader>("Engine/Shaders/TerrainSystem/TerrainLit.glsl");
            auto shadow = ResourceManager::Get().LoadByPath<Shader>("Engine/Shaders/TerrainSystem/TerrainShadow.glsl");
            if(!lit || !shadow || !lit->IsComplete() || !shadow->IsComplete() || lit->PassesCount() == 0 ||
               shadow->PassesCount() == 0)
                continue;
            gpu.color = ResourceManager::Get().Create<Material>(lit);
            gpu.shadow = ResourceManager::Get().Create<Material>(shadow);
        }
        gpu.color->SetEnableInstancing(renderer.useInstancing);
        gpu.shadow->SetEnableInstancing(renderer.useInstancing);
        SetTerrainUniforms(*gpu.color, data, root, gpu);
        SetTerrainUniforms(*gpu.shadow, data, root, gpu);
        if(!s.white)
            s.white = ResourceManager::Get().LoadByPath<Texture2D>("Engine/Textures/White.jpg");
        if(!s.normal)
            s.normal = ResourceManager::Get().LoadByPath<Texture2D>("Engine/Textures/Normal.jpg");
        Vector4 metallic(0), roughness(0), strength(0);
        for(int i = 0; i < 4; ++i){
            auto& l = renderer.layers[i];
            std::string suffix = std::to_string(i);
            gpu.color->SetTexture(("albedo" + suffix).c_str(), l.albedo ? l.albedo : s.white);
            gpu.color->SetTexture(("normal" + suffix).c_str(), l.normal ? l.normal : s.normal);
            gpu.color->SetVector2(("tileSize" + suffix).c_str(), l.tileSize);
            gpu.color->SetVector4(("tint" + suffix).c_str(), l.tint);
            metallic[i] = l.metallic;
            roughness[i] = l.roughness;
            strength[i] = l.normalStrength;
        }
        gpu.color->SetVector4("layerMetallic", metallic);
        gpu.color->SetVector4("layerRoughness", roughness);
        gpu.color->SetVector4("layerNormalStrength", strength);
        Vector3 eye = Vector3(math::inverse(root) * Vector4(camera.viewPos, 1));
        TerrainCpu::QuadSettings settings;
        settings.width = data.size.x;
        settings.length = data.size.z;
        settings.height = data.size.y;
        settings.eyeX = eye.x;
        settings.eyeY = eye.y;
        settings.eyeZ = eye.z;
        settings.cells = renderer.patchCells;
        settings.maxDepth = renderer.maxDepth;
        settings.maxPatches = renderer.maxPatches;
        settings.minPatchSize = renderer.minPatchSize;
        settings.pixelError = renderer.pixelError;
        // A conservative scale factor keeps detail when the terrain entity is scaled.
        float scale = (std::max)({math::length(Vector3(root[0])),
                                  math::length(Vector3(root[1])),
                                  math::length(Vector3(root[2]))});
        settings.projectionScale =
            std::abs(camera.projection[1][1]) * (std::max)(1, camera.height) * 0.5f * (std::max)(1.f, scale);
        settings.orthographic = std::abs(camera.projection[3][3] - 1) < 0.001f;
        auto patches = TerrainCpu::SelectPatches(settings);
        renderer.selectedPatches = int(patches.size());
        for(auto p : patches){
            float span = std::ldexp(1.f, -p.depth), w = data.size.x * span, l = data.size.z * span;
            Matrix4 patch =
                math::translate(Matrix4(1), Vector3(p.x * w, 0, p.z * l)) * math::scale(Matrix4(1), Vector3(w, 1, l));
            RenderData rd{};
            rd.targetMatrix = root * patch;
            rd.aabb = transform_aabb_optimized_abs_center_extents(
                AABB(Vector3(0.5f, data.size.y * 0.5f, 0.5f), 0.5f, data.size.y * 0.5f, 0.5f), rd.targetMatrix);
            Vector3 delta = math::max(math::abs(camera.viewPos - rd.aabb.center) - rd.aabb.extents, Vector3(0));
            if(math::dot(delta, delta) > renderer.maxDrawDistance * renderer.maxDrawDistance)
                continue;
            // Do not camera-frustum reject here: pipeline culling also needs shadow-only patches.
            rd.targetMesh = s.Grid(renderer.patchCells, p.stitch)->GetId();
            rd.targetMaterial = gpu.color->GetId();
            rd.customShadowPass = gpu.shadow->GetId();
            rd.layer = info.layer;
            rd.distance = math::distance2(camera.viewPos, rd.aabb.center);
            rd.SetFlag(RenderData::FromMesh, true);
            rd.SetFlag(RenderData::RenderShadow, renderer.castShadows);
            rd.targetMatrix[0][3] = p.x * span;
            rd.targetMatrix[1][3] = p.z * span;
            rd.targetMatrix[2][3] = span;
            rd.targetMatrix[3][3] = float(info.layer);
            out.push_back(rd);
            ++renderer.submittedPatches;
        }
    }
}
}
