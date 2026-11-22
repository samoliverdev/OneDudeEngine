#include <gtest/gtest.h>
#include "OD/TerrainSystem/TerrainRendering.h"
#include "OD/TerrainSystem/TerrainComponents.h"
#include "OD/TerrainSystem/TerrainNavigation.h"
#include "OD/RenderPipeline/RenderContext.h"
#include "OD/Graphics/Material.h"
#include "OD/Graphics/Graphics.h"
#include "OD/Graphics/Framebuffer.h"
#include "OD/Graphics/Mesh.h"
#include "OD/Navmesh/Navmesh.h"
#include "OD/TerrainSystem/TerrainPhysics.h"
#include "OD/Physics/PhysicsSystem.h"
#include <filesystem>
#include <fstream>
#if defined(_WIN32) && defined(OPENGL_SUPPORT)
#include <Windows.h>
#endif

namespace OD{
static void RegisterTerrainTestPasses(){
    if(FramebufferRenderPass::GetRenderPassIndex("EntityId") < 0){
        RenderPassInfo id;
        id.colorAttachments = {{FramebufferTextureFormat::RED_INTEGER}};
        id.depthAttachment = {FramebufferTextureFormat::DEPTH_COMPONENT32};
        FramebufferRenderPass::RegisterRenderPass("EntityId", id);
    }
}

TEST(TerrainSystemRenderTest, ShaderCompileFailureDoesNotProduceAValidProgram){
    auto file = std::filesystem::temp_directory_path() / "OneDudeInvalidTerrainShader.glsl";
{
        std::ofstream out(file);
        out << "#pragma BeginPassDef\nName MainPass\nRenderPass Forward\n#pragma EndPassDef\n"
               "#if defined(VERTEX)\nvoid main(){ vec4 patch=vec4(0); gl_Position=patch; }\n#endif\n"
               "#if defined(FRAGMENT)\nlayout(location=0) out vec4 color; void main(){color=vec4(1);}\n#endif\n";
    }
    Shader shader;
    EXPECT_FALSE(shader.LoadFromFile(file.string()));
    EXPECT_FALSE(shader.IsComplete());
    std::filesystem::remove(file);
    // A subsequent valid shader must still compile; no invalid program/block queries are allowed.
    auto valid = ResourceManager::Get().LoadByPath<Shader>("Engine/Shaders/TerrainSystem/TerrainShadow.glsl");
    ASSERT_TRUE(valid);
    EXPECT_TRUE(valid->IsComplete());
    EXPECT_GT(valid->PassesCount(), 0);
}

TEST(TerrainSystemRenderTest, CompilesTerrainLightingAndInstancedShadowShaders){
    auto lit = ResourceManager::Get().LoadByPath<Shader>("Engine/Shaders/TerrainSystem/TerrainLit.glsl");
    auto shadow = ResourceManager::Get().LoadByPath<Shader>("Engine/Shaders/TerrainSystem/TerrainShadow.glsl");
    ASSERT_TRUE(lit);
    ASSERT_TRUE(lit->IsComplete());
    ASSERT_GT(lit->PassesCount(), 0);
    ASSERT_TRUE(shadow);
    ASSERT_TRUE(shadow->IsComplete());
    ASSERT_GT(shadow->PassesCount(), 0);
    auto material = ResourceManager::Get().Create<Material>(lit, true);
    auto depth = ResourceManager::Get().Create<Material>(shadow, true);
    EXPECT_TRUE(material->SupportInstancing());
    EXPECT_TRUE(depth->SupportInstancing());
    EXPECT_GE(material->MainPass(), 0);
    EXPECT_GE(depth->DepthPass(), 0);
}

TEST(TerrainSystemRenderTest, CollectsIndependentCameraLodsAndPreservesBounds){
    // The test module boots graphics without the standard rendering module.
    RegisterTerrainTestPasses();
    Scene scene(true);
    Entity e = scene.AddEntity("Terrain render acceptance");
    auto& data = scene.AddComponent<TerrainDataComponent>(e);
    data.size = Vector3(32000, 1000, 16000);
    ASSERT_TRUE(data.Create(33, 17, 16, 16, 0.25f));
    auto& settings = scene.AddComponent<TerrainRendererComponent>(e);
    settings.pixelError = 2;
    settings.maxPatches = 512;
    RenderContext context;
    TerrainQuadtreeRenderer backend;
    Camera nearCamera;
    nearCamera.SetPerspective(60, 0.1f, 1000000, 800, 600);
    nearCamera.viewPos = Vector3(500, 400, 500);
    nearCamera.LookAt(nearCamera.viewPos, Vector3(2000, 0, 2000), Vector3(0, 1, 0));
    std::vector<RenderData> close;
    backend.OnCollectCameraRenderData(scene, context, nearCamera, close);
    ASSERT_FALSE(close.empty());
    EXPECT_LE(close.size(), 512u);
    Camera farCamera = nearCamera;
    farCamera.viewPos = Vector3(16000, 90000, 8000);
    farCamera.LookAt(farCamera.viewPos, Vector3(16000, 0, 8000), Vector3(0, 0, 1));
    std::vector<RenderData> distant;
    backend.OnCollectCameraRenderData(scene, context, farCamera, distant);
    ASSERT_FALSE(distant.empty());
    EXPECT_LT(distant.size(), close.size());
    for(auto& rd : close){
        EXPECT_GE(rd.aabb.GetMin().y, -0.01f);
        EXPECT_LE(rd.aabb.GetMax().y, 1000.01f);
        EXPECT_TRUE(rd.HasFlag(RenderData::RenderShadow));
        EXPECT_NE(rd.customShadowPass, INVALID_RESOURCE_ID);
        EXPECT_GT(rd.targetMatrix[2][3], 0);
    }
    settings.castShadows = false;
    settings.useInstancing = false;
    std::vector<RenderData> noninstanced;
    backend.OnCollectCameraRenderData(scene, context, nearCamera, noninstanced);
    EXPECT_EQ(noninstanced.size(), close.size());
    for(auto& rd : noninstanced){
        EXPECT_FALSE(rd.HasFlag(RenderData::RenderShadow));
        auto* material = ResourceManager::Get().GetAllocatorView<Material>()->Get(rd.targetMaterial);
        ASSERT_NE(material, nullptr);
        EXPECT_FALSE(material->EnableInstancing());
    }
}

TEST(TerrainSystemRenderTest, PartialHeightUpdatesPreserveOtherTexels){
#if defined(_WIN32) && defined(OPENGL_SUPPORT)
    std::vector<float> heights(15, 0.25f);
    auto texture =
        Texture2D::CreateFromRaw(heights.data(),
                                 5,
                                 3,
                                 TextureDataType::Float,
                                 {TextureFilter::Linear, TextureWrapping::ClampToEdge, false, TextureFormat::RED32F},
                                 "Terrain update test");
    ASSERT_TRUE(texture);
    float patch[] = {0.5f, 0.75f};
    ASSERT_TRUE(texture->UpdateRegion(2, 1, 2, 1, patch, TextureDataType::Float));
    // Engine's GL loader is private to its DLL; read back with the system OpenGL 1.1 exports.
    HMODULE module = GetModuleHandleW(L"opengl32.dll");
    ASSERT_NE(module, nullptr);
    auto getInt = reinterpret_cast<PFNGLGETINTEGERVPROC>(GetProcAddress(module, "glGetIntegerv"));
    auto bind = reinterpret_cast<PFNGLBINDTEXTUREPROC>(GetProcAddress(module, "glBindTexture"));
    auto getImage = reinterpret_cast<PFNGLGETTEXIMAGEPROC>(GetProcAddress(module, "glGetTexImage"));
    ASSERT_NE(getInt, nullptr);
    ASSERT_NE(bind, nullptr);
    ASSERT_NE(getImage, nullptr);
    GLint previous = 0;
    getInt(GL_TEXTURE_BINDING_2D, &previous);
    bind(GL_TEXTURE_2D, GLuint(reinterpret_cast<uintptr_t>(texture->RenderId())));
    std::vector<float> read(15);
    getImage(GL_TEXTURE_2D, 0, GL_RED, GL_FLOAT, read.data());
    bind(GL_TEXTURE_2D, GLuint(previous));
    for(int i = 0; i < 15; ++i)
        EXPECT_FLOAT_EQ(read[i], i == 7 ? 0.5f : i == 8 ? 0.75f : 0.25f);
#endif
}

TEST(TerrainSystemRenderTest, NavigationGeometryIsBoundedAndIndependentOfRenderer){
    Scene scene(true);
    Entity e = scene.AddEntity("Terrain navigation acceptance");
    auto& data = scene.AddComponent<TerrainDataComponent>(e);
    data.size = Vector3(32000, 1000, 16000);
    ASSERT_TRUE(data.Create(33, 17, 16, 16, 0.25f));
    auto& settings = scene.AddComponent<TerrainNavmeshDataComponent>(e);
    settings.sampleSpacing = 4;
    settings.maxCellsPerAxis = 64;
    scene.GetComponent<TransformComponent>(e).Position(Vector3(100, 0, 200));
    AABB bounds(Vector3(2100, 200, 2200), 128, 300, 128);
    int calls = 0;
    RasterizeTerrainNavigation(scene, bounds, {}, [&](const Matrix4& root, Ref<Mesh>& mesh){
        ++calls;
        ASSERT_FALSE(mesh->vertices.empty());
        EXPECT_LE(mesh->vertices.size(), 65u * 65);
        for(auto& vertex : mesh->vertices)
            EXPECT_NEAR(vertex.y, 250, 0.0001);
        EXPECT_NEAR(root[3].x, 100, 0.0001);
        EXPECT_NEAR(root[3].z, 200, 0.0001);
    });
    EXPECT_EQ(calls, 1);
    settings.enabled = false;
    RasterizeTerrainNavigation(scene, bounds, {}, [&](const Matrix4&, Ref<Mesh>&){
        ++calls;
    });
    EXPECT_EQ(calls, 1);
    settings.enabled = true;
    scene.AddTagComponent<NavmeshSkipTag>(e);
    RasterizeTerrainNavigation(scene, bounds, {}, [&](const Matrix4&, Ref<Mesh>&){
        ++calls;
    });
    EXPECT_EQ(calls, 1);
}

TEST(TerrainSystemRenderTest, RendersResident8kHeightmapWithMatchingColorAndShadowDepth){
#if defined(_WIN32) && defined(OPENGL_SUPPORT)
    RegisterTerrainTestPasses();
    Scene scene(true);
    Entity e = scene.AddEntity("32 km terrain GPU acceptance");
    auto& data = scene.AddComponent<TerrainDataComponent>(e);
    data.size = Vector3(32000, 1000, 32000);
    ASSERT_TRUE(data.Create(8193, 8193, 16, 16, 0.25f));
    auto& settings = scene.AddComponent<TerrainRendererComponent>(e);
    settings.maxPatches = 1;
    settings.layers[0].tint = Vector4(0.25f, 0.5f, 0.125f, 1);
    RenderContext context;
    TerrainQuadtreeRenderer backend;
    Camera camera;
    camera.SetOrtho(16000, 0.1f, 30000, 32, 32);
    camera.viewPos = Vector3(16000, 5000, 16000);
    camera.LookAt(camera.viewPos, Vector3(16000, 0, 16000), Vector3(0, 0, 1));
    std::vector<RenderData> draws;
    backend.OnCollectCameraRenderData(scene, context, camera, draws);
    ASSERT_EQ(draws.size(), 1u);
    auto& rd = draws[0];
    auto* mesh = ResourceManager::Get().GetAllocatorView<Mesh>()->Get(rd.targetMesh);
    auto* color = ResourceManager::Get().GetAllocatorView<Material>()->Get(rd.targetMaterial);
    auto* shadow = ResourceManager::Get().GetAllocatorView<Material>()->Get(rd.customShadowPass);
    ASSERT_NE(mesh, nullptr);
    ASSERT_NE(color, nullptr);
    ASSERT_NE(shadow, nullptr);
    FrameBufferSpecification spec{32, 32};
    spec.colorAttachments = {{FramebufferTextureFormat::RGB16F},
                             {FramebufferTextureFormat::RGBA16F},
                             {FramebufferTextureFormat::RGBA16F},
                             {FramebufferTextureFormat::RGB11B10F}};
    spec.depthAttachment = {FramebufferTextureFormat::DEPTH_COMPONENT32F};
    auto gbuffer = CreateRef<Framebuffer>(spec);
    spec.colorAttachments.clear();
    auto depth = CreateRef<Framebuffer>(spec);
    HMODULE module = GetModuleHandleW(L"opengl32.dll");
    auto bind = reinterpret_cast<PFNGLBINDTEXTUREPROC>(GetProcAddress(module, "glBindTexture"));
    auto getImage = reinterpret_cast<PFNGLGETTEXIMAGEPROC>(GetProcAddress(module, "glGetTexImage"));
    ASSERT_NE(bind, nullptr);
    ASSERT_NE(getImage, nullptr);
    color->DisableKeyword("Forward");
    color->EnableKeyword("Deferred");
    Graphics::SetCamera(camera);
    for(bool instanced : {false, true}){
        Graphics::BeginFramebuffer(*gbuffer);
        color->SetPass(color->MainPass());
        if(instanced)
            Graphics::DrawMeshInstancing(*mesh, *color, &rd.targetMatrix, 1);
        else
            Graphics::DrawMesh(*mesh, *color, rd.targetMatrix);
        Graphics::EndFramebuffer();
        std::vector<float> albedo(32 * 32 * 4), surfaceDepth(32 * 32), shadowDepth(32 * 32);
        bind(GL_TEXTURE_2D, GLuint(reinterpret_cast<uintptr_t>(gbuffer->ColorAttachmentId(1))));
        getImage(GL_TEXTURE_2D, 0, GL_RGBA, GL_FLOAT, albedo.data());
        bind(GL_TEXTURE_2D, GLuint(reinterpret_cast<uintptr_t>(gbuffer->DepthAttachmentId())));
        getImage(GL_TEXTURE_2D, 0, GL_DEPTH_COMPONENT, GL_FLOAT, surfaceDepth.data());
        Graphics::BeginFramebuffer(*depth);
        shadow->SetPass(shadow->DepthPass());
        if(instanced)
            Graphics::DrawMeshInstancing(*mesh, *shadow, &rd.targetMatrix, 1);
        else
            Graphics::DrawMesh(*mesh, *shadow, rd.targetMatrix);
        Graphics::EndFramebuffer();
        bind(GL_TEXTURE_2D, GLuint(reinterpret_cast<uintptr_t>(depth->DepthAttachmentId())));
        getImage(GL_TEXTURE_2D, 0, GL_DEPTH_COMPONENT, GL_FLOAT, shadowDepth.data());
        bind(GL_TEXTURE_2D, 0);
        int center = 16 * 32 + 16;
        EXPECT_NEAR(albedo[center * 4], 0.25f, 0.01f);
        EXPECT_NEAR(albedo[center * 4 + 1], 0.5f, 0.01f);
        EXPECT_NEAR(albedo[center * 4 + 2], 0.125f, 0.01f);
        EXPECT_LT(surfaceDepth[center], 0.99f);
        for(int pixel = 0; pixel < 32 * 32; ++pixel)
            EXPECT_NEAR(surfaceDepth[pixel], shadowDepth[pixel], 0.00001f);
    }
#endif
}

TEST(TerrainSystemRenderTest, NormalMappingRemainsVisibleAtGrazingAngles){
#if defined(_WIN32) && defined(OPENGL_SUPPORT)
    RegisterTerrainTestPasses();
    Scene scene(true);
    Entity e = scene.AddEntity("Terrain grazing normal regression");
    auto& data = scene.AddComponent<TerrainDataComponent>(e);
    data.size = Vector3(32, 1, 32);
    ASSERT_TRUE(data.Create(9, 9, 4, 4, 0));
    auto& settings = scene.AddComponent<TerrainRendererComponent>(e);
    settings.maxPatches = 1;
    unsigned char normalPixel[] = {191, 128, 238, 255};
    settings.layers[0].normal = Texture2D::CreateFromRaw(
        normalPixel, 1, 1, TextureDataType::UnsignedByte,
        {TextureFilter::Linear, TextureWrapping::Repeat, false, TextureFormat::RGBA8});
    ASSERT_TRUE(settings.layers[0].normal);

    Camera camera;
    camera.SetOrtho(1, 0.1f, 100, 32, 32);
    camera.viewPos = Vector3(-10, 2, 16);
    camera.LookAt(camera.viewPos, Vector3(16, 0, 16), Vector3(0, 1, 0));
    RenderContext context;
    TerrainQuadtreeRenderer backend;
    std::vector<RenderData> draws;
    backend.OnCollectCameraRenderData(scene, context, camera, draws);
    ASSERT_EQ(draws.size(), 1u);
    auto& rd = draws[0];
    auto* mesh = ResourceManager::Get().GetAllocatorView<Mesh>()->Get(rd.targetMesh);
    auto* material = ResourceManager::Get().GetAllocatorView<Material>()->Get(rd.targetMaterial);
    ASSERT_NE(mesh, nullptr);
    ASSERT_NE(material, nullptr);
    material->DisableKeyword("Forward");
    material->EnableKeyword("Deferred");

    FrameBufferSpecification spec{32, 32};
    spec.colorAttachments = {{FramebufferTextureFormat::RGB16F},
                             {FramebufferTextureFormat::RGBA16F},
                             {FramebufferTextureFormat::RGBA16F},
                             {FramebufferTextureFormat::RGB11B10F}};
    spec.depthAttachment = {FramebufferTextureFormat::DEPTH_COMPONENT32F};
    auto gbuffer = CreateRef<Framebuffer>(spec);
    Graphics::SetCamera(camera);
    unsigned int viewportX, viewportY, viewportWidth, viewportHeight;
    Graphics::GetViewport(&viewportX, &viewportY, &viewportWidth, &viewportHeight);
    Graphics::SetViewport(0, 0, 32, 32);
    Graphics::BeginFramebuffer(*gbuffer);
    material->SetPass(material->MainPass());
    Graphics::DrawMesh(*mesh, *material, rd.targetMatrix);
    Graphics::EndFramebuffer();
    Graphics::SetViewport(viewportX, viewportY, viewportWidth, viewportHeight);

    HMODULE module = GetModuleHandleW(L"opengl32.dll");
    auto bind = reinterpret_cast<PFNGLBINDTEXTUREPROC>(GetProcAddress(module, "glBindTexture"));
    auto getImage = reinterpret_cast<PFNGLGETTEXIMAGEPROC>(GetProcAddress(module, "glGetTexImage"));
    ASSERT_NE(bind, nullptr);
    ASSERT_NE(getImage, nullptr);
    std::vector<float> normals(32 * 32 * 3), depths(32 * 32);
    bind(GL_TEXTURE_2D, GLuint(reinterpret_cast<uintptr_t>(gbuffer->ColorAttachmentId(0))));
    getImage(GL_TEXTURE_2D, 0, GL_RGB, GL_FLOAT, normals.data());
    bind(GL_TEXTURE_2D, GLuint(reinterpret_cast<uintptr_t>(gbuffer->DepthAttachmentId())));
    getImage(GL_TEXTURE_2D, 0, GL_DEPTH_COMPONENT, GL_FLOAT, depths.data());
    bind(GL_TEXTURE_2D, 0);

    int center = 16 * 32 + 16;
    ASSERT_LT(depths[center], 1.f);
    Vector2 packed(normals[center * 3], normals[center * 3 + 1]);
    Vector3 normal(packed.x, packed.y, 1 - std::abs(packed.x) - std::abs(packed.y));
    if(normal.z < 0){
        normal.x = (1 - std::abs(packed.y)) * (packed.x >= 0 ? 1 : -1);
        normal.y = (1 - std::abs(packed.x)) * (packed.y >= 0 ? 1 : -1);
    }
    normal = glm::normalize(normal);
    Vector4 position = glm::inverse(camera.projection * camera.view) *
                       Vector4(2 * 16.5f / 32 - 1, 2 * 16.5f / 32 - 1, depths[center] * 2 - 1, 1);
    Vector3 viewDirection = glm::normalize(camera.viewPos - Vector3(position) / position.w);
    EXPECT_GT(glm::dot(normal, viewDirection), 0.01f);
#endif
}

TEST(TerrainSystemRenderTest, PhysicsHeightfieldMatchesTerrainAndRebuildsScaledShapes){
    PhysicsModuleInit();

    struct ShutdownPhysics{
        ~ShutdownPhysics(){
            PhysicsModuleShutdown();
        }
    } shutdown;

    Scene scene(true);
    scene.AddSystem<PhysicsSystem>();
    scene.AddSystem<TerrainCollisionSystem>();
    Entity e = scene.AddEntity("Terrain physics acceptance");
    auto& data = scene.AddComponent<TerrainDataComponent>(e);
    data.size = Vector3(16, 100, 16);
    ASSERT_TRUE(data.Create(9, 9, 9, 9, 0.5f));
    auto& config = scene.AddComponent<TerrainCollisionComponent>(e);
    config.tilesX = config.tilesZ = 1;
    config.samplesPerTile = 8;
    auto& root = scene.GetComponent<TransformComponent>(e);
    root.LocalPosition(Vector3(10, 20, 30));
    root.LocalScale(Vector3(2, 1, 3));
    scene.Start();
    auto* physics = scene.GetSystem<PhysicsSystem>();
    physics->PrePhysicsUpdate(scene);
    RayResult hit{};
    ASSERT_TRUE(physics->Raycast(Vector3(26, 200, 54), Vector3(0, -200, 0), hit));
    EXPECT_NEAR(hit.hitPoint.y, 70, 0.1f);
    root.LocalScale(Vector3(2, 2, 3));
    scene.GetSystem<TerrainCollisionSystem>()->PrePhysicsUpdate(scene);
    physics->PrePhysicsUpdate(scene);
    ASSERT_TRUE(physics->Raycast(Vector3(26, 200, 54), Vector3(0, -200, 0), hit));
    EXPECT_NEAR(hit.hitPoint.y, 120, 0.1f);
    // A non-planar cell reveals opposite-diagonal errors that flat-ground tests cannot.
    std::fill(data.heightmap->data.begin(), data.heightmap->data.end(), 0.f);
    data.heightmap->data[4 * 9 + 4] = 1.f;
    data.Touch();
    scene.GetSystem<TerrainCollisionSystem>()->PrePhysicsUpdate(scene);
    physics->PrePhysicsUpdate(scene);
    ASSERT_TRUE(physics->Raycast(Vector3(24, 250, 51), Vector3(0, -250, 0), hit));
    EXPECT_NEAR(hit.hitPoint.y, 120, 0.2f);
    auto indices = TerrainCpu::GridIndices(8, 0);
    int sharedDiagonal = 0;
    for(size_t i = 0; i < indices.size(); i += 3){
        bool first = false, last = false;
        for(size_t j = i; j < i + 3; ++j){
            first |= indices[j] == 30;
            last |= indices[j] == 40;
        }
        if(first && last)
            ++sharedDiagonal;
    }
    EXPECT_EQ(sharedDiagonal, 2);
    scene.Stop();
}
}
