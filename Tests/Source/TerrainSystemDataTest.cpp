#include <gtest/gtest.h>
#include "OD/TerrainSystem/TerrainComponents.h"
#include "OD/TerrainSystem/TerrainPhysics.h"
#include "OD/Scene/SceneManager.h"
#include "OD/Core/Log.h"
#include <cereal/archives/json.hpp>
#include <filesystem>
#include <fstream>
#include <sstream>

namespace OD{
namespace{
TerrainCpu::Brush RaiseAt(float x = 4, float z = 4){
    TerrainCpu::Brush b;
    b.x = x;
    b.z = z;
    b.radius = 2;
    b.amount = 10;
    return b;
}

TEST(TerrainSystemDataTest, StrokeUndoRedoMergesRepeatedSamples){
    TerrainDataComponent d;
    d.size = Vector3(8, 100, 8);
    ASSERT_TRUE(d.Create(9, 9, 9, 9));
    d.BeginStroke();
    ASSERT_TRUE(d.ApplyBrush(RaiseAt()));
    ASSERT_TRUE(d.ApplyBrush(RaiseAt()));
    d.EndStroke();
    EXPECT_NEAR(d.View().Sample(4, 4), 20, 0.0001);
    ASSERT_TRUE(d.Undo());
    EXPECT_FLOAT_EQ(d.View().Sample(4, 4), 0);
    EXPECT_FALSE(d.CanUndo());
    ASSERT_TRUE(d.Redo());
    EXPECT_NEAR(d.View().Sample(4, 4), 20, 0.0001);
    ASSERT_TRUE(d.Undo());
    ASSERT_TRUE(d.ApplyBrush(RaiseAt()));
    EXPECT_FALSE(d.CanRedo());
}

TEST(TerrainSystemDataTest, CopyOnWriteProtectsBothSidesAfterEarlierEditing){
    TerrainDataComponent a;
    a.size = Vector3(8, 100, 8);
    ASSERT_TRUE(a.Create(9, 9, 9, 9));
    ASSERT_TRUE(a.ApplyBrush(RaiseAt()));
    TerrainDataComponent b = a;
    ASSERT_TRUE(a.ApplyBrush(RaiseAt()));
    EXPECT_NEAR(b.View().Sample(4, 4), 10, 0.0001);
    ASSERT_TRUE(b.ApplyBrush(RaiseAt()));
    EXPECT_NE(a.heightmap.get(), b.heightmap.get());
    ASSERT_TRUE(b.Undo());
    EXPECT_NEAR(b.View().Sample(4, 4), 10, 0.0001);
    EXPECT_NEAR(a.View().Sample(4, 4), 20, 0.0001);
}

TEST(TerrainSystemDataTest, PaintStrokeNormalizesAndUndoRestoresAllChannels){
    TerrainDataComponent d;
    d.size = Vector3(8, 100, 8);
    ASSERT_TRUE(d.Create(9, 9, 9, 9));
    auto b = RaiseAt();
    b.mode = TerrainCpu::BrushMode::Paint;
    b.amount = 0.3f;
    d.BeginStroke();
    for(int i = 0; i < 20; ++i)
        ASSERT_TRUE(d.ApplyBrush(b, (i + 1) % 4));
    d.EndStroke();
    for(size_t i = 0; i < d.paintmap->weights.size(); i += 4){
        int sum = 0;
        for(int c = 0; c < 4; ++c)
            sum += d.paintmap->weights[i + c];
        EXPECT_EQ(sum, 255);
    }
    ASSERT_TRUE(d.Undo());
    for(size_t i = 0; i < d.paintmap->weights.size(); i += 4){
        EXPECT_EQ(d.paintmap->weights[i], 255);
        for(int c = 1; c < 4; ++c)
            EXPECT_EQ(d.paintmap->weights[i + c], 0);
    }
}

TEST(TerrainSystemDataTest, RejectsNonfiniteBrushAndOversizedStamp){
    TerrainDataComponent d;
    d.size = Vector3(8, 100, 8);
    ASSERT_TRUE(d.Create(9, 9, 9, 9));
    auto b = RaiseAt();
    b.x = std::numeric_limits<float>::quiet_NaN();
    EXPECT_FALSE(d.ApplyBrush(b));
    b = RaiseAt();
    b.mode = TerrainCpu::BrushMode::Paint;
    b.amount = std::numeric_limits<float>::infinity();
    EXPECT_FALSE(d.ApplyBrush(b));
    b = RaiseAt();
    b.hardness = std::numeric_limits<float>::quiet_NaN();
    EXPECT_FALSE(d.ApplyBrush(b));
    d.maxBrushSamples = 64;
    b = RaiseAt();
    b.radius = 100;
    EXPECT_FALSE(d.ApplyBrush(b));
    EXPECT_FLOAT_EQ(d.View().Sample(4, 4), 0);
}

TEST(TerrainSystemDataTest, TracksRegionalChangesAndFallsBackWhenHistoryExpires){
    TerrainDataComponent d;
    d.size = Vector3(8, 100, 8);
    ASSERT_TRUE(d.Create(33, 33, 9, 9));
    uint64_t old = d.Revision();
    auto b = RaiseAt(2, 2);
    b.radius = 0.5f;
    ASSERT_TRUE(d.ApplyBrush(b));
    auto r = d.ChangesSince(old, false);
    EXPECT_GT(r.x0, 0);
    EXPECT_LT(r.x1, 33);
    EXPECT_GT(r.z0, 0);
    EXPECT_LT(r.z1, 33);
    for(int i = 0; i < 80; ++i)
        d.TouchRegion(false, 0, 0, 1, 1);
    r = d.ChangesSince(old, false);
    EXPECT_EQ(r.x0, 0);
    EXPECT_EQ(r.x1, 33);
}

TEST(TerrainSystemDataTest, RawAndAssetPersistenceRoundtrip){
    auto dir = std::filesystem::temp_directory_path() / "OneDudeTerrainSystemTests";
    std::filesystem::create_directories(dir);
    auto hf = (dir / "roundtrip.heightmap").string(), pf = (dir / "roundtrip.terrainpaint").string(),
         raw = (dir / "roundtrip.raw").string();
    TerrainDataComponent d;
    d.size = Vector3(8, 100, 8);
    ASSERT_TRUE(d.Create(9, 9, 9, 9, 0.25f));
    ASSERT_TRUE(d.ApplyBrush(RaiseAt()));
    ASSERT_TRUE(d.SaveAssets(hf, pf));
    ASSERT_TRUE(d.ExportRaw16(raw, true));
    Heightmap heights;
    ASSERT_TRUE(heights.LoadFromFile(hf));
    EXPECT_EQ(heights.data, d.heightmap->data);
    TerrainPaintMap paint;
    ASSERT_TRUE(paint.LoadFromFile(pf));
    EXPECT_EQ(paint.weights, d.paintmap->weights);
    TerrainDataComponent imported;
    imported.size = d.size;
    ASSERT_TRUE(imported.ImportRaw16(raw, 9, 9, true));
    EXPECT_NEAR(imported.View().Sample(4, 4), d.View().Sample(4, 4), 0.002);
    EXPECT_FALSE(imported.ImportRaw16(raw, 10, 10, true));
    std::filesystem::remove(hf);
    std::filesystem::remove(pf);
    std::filesystem::remove(raw);
    std::filesystem::remove(dir);
}

TEST(TerrainSystemDataTest, ConfigurableSettingsAreValidatedAndSerialized){
    TerrainRendererComponent renderer;
    renderer.patchCells = 31;
    renderer.maxPatches = -1;
    renderer.pixelError = std::numeric_limits<float>::quiet_NaN();
    renderer.Validate();
    EXPECT_EQ(renderer.patchCells, 32);
    EXPECT_EQ(renderer.maxPatches, 1);
    EXPECT_EQ(renderer.pixelError, 6);
    renderer.maxPatches = 4096;
    renderer.layers[2].tileSize = Vector2(13, 17);
    TerrainCollisionComponent collision;
    collision.tilesX = 0;
    collision.samplesPerTile = 13;
    collision.Validate();
    EXPECT_EQ(collision.tilesX, 1);
    EXPECT_EQ(collision.samplesPerTile, 17);
    std::stringstream stream;
{
        cereal::JSONOutputArchive ar(stream);
        ar(renderer, collision);
    }
    TerrainRendererComponent restored;
    TerrainCollisionComponent restoredCollision;
{
        cereal::JSONInputArchive ar(stream);
        ar(restored, restoredCollision);
    }
    EXPECT_EQ(restored.maxPatches, 4096);
    EXPECT_EQ(restored.layers[2].tileSize, Vector2(13, 17));
    EXPECT_EQ(restoredCollision.samplesPerTile, 17);
}

TEST(TerrainSystemDataTest, CollisionTilesRebuildAfterParentTransformChanges){
    Scene scene(true);
    Entity parent = scene.AddEntity("Parent"), e = scene.AddEntity("Terrain");
    scene.SetParent(parent, e);
    auto& data = scene.AddComponent<TerrainDataComponent>(e);
    data.size = Vector3(16, 100, 16);
    ASSERT_TRUE(data.Create(9, 9, 9, 9, 0.5f));
    auto& collision = scene.AddComponent<TerrainCollisionComponent>(e);
    collision.tilesX = collision.tilesZ = 1;
    collision.samplesPerTile = 8;
    scene.AddSystem<TerrainCollisionSystem>();
    auto* system = scene.GetSystem<TerrainCollisionSystem>();
    scene.Start();
    system->PrePhysicsUpdate(scene);
    auto first = scene.GetComponent<TransformComponent>(e).Children();
    ASSERT_EQ(first.size(), 1u);
    Entity old = first[0];
    scene.GetComponent<TransformComponent>(parent).LocalScale(Vector3(2, 1, 3));
    system->PrePhysicsUpdate(scene);
    EXPECT_FALSE(scene.IsValid(old));
    auto second = scene.GetComponent<TransformComponent>(e).Children();
    ASSERT_EQ(second.size(), 1u);
    EXPECT_NE(second[0], old);
    scene.Stop();
    EXPECT_TRUE(scene.GetComponent<TransformComponent>(e).Children().empty());
}

TEST(TerrainSystemDataTest, InitialCollisionIsPreparedBeforeFirstSimulationStep){
    Scene scene(true);
    Entity e = scene.AddEntity("Terrain");
    auto& data = scene.AddComponent<TerrainDataComponent>(e);
    data.size = Vector3(16, 100, 16);
    ASSERT_TRUE(data.Create(9, 9, 9, 9));
    auto& collision = scene.AddComponent<TerrainCollisionComponent>(e);
    collision.tilesX = collision.tilesZ = 2;
    collision.samplesPerTile = 8;
    collision.rebuildsPerFrame = 1;
    scene.AddSystem<TerrainCollisionSystem>();
    scene.Start();
    EXPECT_EQ(scene.GetComponent<TransformComponent>(e).Children().size(), 4u);
    scene.Stop();
    EXPECT_TRUE(scene.GetComponent<TransformComponent>(e).Children().empty());
}

TEST(TerrainSystemDataTest, SupportsResident8kHeightmapAcross32Km){
    TerrainDataComponent data;
    data.size = Vector3(32000, 1000, 32000);
    ASSERT_TRUE(data.Create(8193, 8193, 256, 256, 0.25f));
    EXPECT_EQ(data.heightmap->data.size(), size_t(8193) * 8193);
    EXPECT_FLOAT_EQ(data.View().Sample(32000, 32000), 250);
    auto* original = data.heightmap.get();
    auto b = RaiseAt(16000, 16000);
    b.radius = 16;
    ASSERT_TRUE(data.ApplyBrush(b));
    EXPECT_EQ(data.heightmap.get(), original);
    EXPECT_NEAR(data.View().Sample(16000, 16000), 260, 0.0001);
    ASSERT_TRUE(data.Undo());
    EXPECT_FLOAT_EQ(data.View().Sample(16000, 16000), 250);
}

TEST(TerrainSystemDataTest, SaveAsThenAssetSaveAndSharedEditingPersistOnSceneReload){
    // Scene::Load logs its result; this fixture also runs without engine startup.
    struct RestoreLogger{
        std::shared_ptr<spdlog::logger> previous = Log::GetLogger();

        ~RestoreLogger(){
            Log::GetLogger() = previous;
        }
    } restoreLogger;

    if(!Log::GetLogger()){
        Log::GetLogger() = std::make_shared<spdlog::logger>("TerrainSaveTest");
    }

    auto dir = std::filesystem::temp_directory_path() / "OneDudeTerrainSceneSaveTest";
    std::filesystem::create_directories(dir);
    auto sceneFile = (dir / "Prototype2.scene").generic_string();
    auto heightFile = (dir / "Assets" / "Terrain.heightmap").generic_string();
    auto paintFile = (dir / "Assets" / "Terrain.terrainpaint").generic_string();
    SceneManager::Get().RegisterCoreComponent<TerrainDataComponent>("TerrainDataComponent", "Terrain System");
{
        Scene scene(true);
        auto e = scene.AddEntity("Terrain");
        auto& data = scene.AddComponent<TerrainDataComponent>(e);
        data.size = Vector3(8, 100, 8);
        ASSERT_TRUE(data.Create(9, 9, 9, 9));
        scene.Save(sceneFile.c_str(), EntityNull);
        ASSERT_EQ(scene.Path(), sceneFile);
        ASSERT_TRUE(data.SaveAssets(heightFile, paintFile));
        TerrainDataComponent shared = data;
        ASSERT_TRUE(data.ApplyBrush(RaiseAt()));
        EXPECT_EQ(data.heightmap->Path(), heightFile);
        EXPECT_EQ(data.paintmap->Path(), paintFile);
        ASSERT_TRUE(data.SaveAssets(heightFile, paintFile));
        scene.Save(scene.Path().c_str(), EntityNull);
        std::ifstream savedScene(sceneFile);
        cereal::JSONInputArchive sceneArchive(savedScene);
        std::vector<TerrainDataComponent> terrains;
        sceneArchive(cereal::make_nvp("TerrainDataComponents", terrains));
        ASSERT_FALSE(terrains.empty());
        auto& restored = terrains.front();
        ASSERT_TRUE(restored.View().Valid());
        EXPECT_NEAR(restored.View().Sample(4, 4), 10, 0.0001);
        ASSERT_TRUE(restored.paintmap);
        EXPECT_TRUE(restored.paintmap->Valid());
    }
    std::filesystem::remove(heightFile);
    std::filesystem::remove(paintFile);
    std::filesystem::remove(sceneFile);
    std::filesystem::remove(dir / "Assets");
    std::filesystem::remove(dir);
}
}
}
