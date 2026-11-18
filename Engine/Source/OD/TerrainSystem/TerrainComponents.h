#pragma once
#include "TerrainData.h"
#include "OD/Graphics/Texture.h"

namespace OD{

struct OD_API TerrainSurfaceLayer{
    Ref<Texture2D> albedo, normal;
    Vector2 tileSize = Vector2(8);
    Vector4 tint = Vector4(1);
    float metallic = 0, roughness = 0.8f, normalStrength = 1;

    template <class Archive> void serialize(Archive& ar){
        ResourceRefSerialize<Texture2D> color(albedo), bump(normal);
        ArchiveDumpNVP(ar, color);
        ArchiveDumpNVP(ar, bump);
        ArchiveDumpNVP(ar, tileSize);
        ArchiveDumpNVP(ar, tint);
        ArchiveDumpNVP(ar, metallic);
        ArchiveDumpNVP(ar, roughness);
        ArchiveDumpNVP(ar, normalStrength);
    }
};

struct OD_API TerrainRendererComponent{
    bool enabled = true, useInstancing = true, castShadows = true;
    int patchCells = 32, maxDepth = 12, maxPatches = 2048;
    float minPatchSize = 128, pixelError = 6, maxDrawDistance = 100000;
    float textureUpdateInterval = 0.1f;
    bool heightTexture32Bit = true;
    std::array<TerrainSurfaceLayer, 4> layers;
    // Diagnostic only: per most recent collecting camera, never used by other systems.
    int selectedPatches = 0, submittedPatches = 0;
    void Validate();
    static void OnGui(Entity e, Scene& scene);

    template <class Archive> void serialize(Archive& ar){
        ArchiveDumpNVP(ar, enabled);
        ArchiveDumpNVP(ar, useInstancing);
        ArchiveDumpNVP(ar, castShadows);
        ArchiveDumpNVP(ar, patchCells);
        ArchiveDumpNVP(ar, maxDepth);
        ArchiveDumpNVP(ar, maxPatches);
        ArchiveDumpNVP(ar, minPatchSize);
        ArchiveDumpNVP(ar, pixelError);
        ArchiveDumpNVP(ar, maxDrawDistance);
        ArchiveDumpNVP(ar, textureUpdateInterval);
        ArchiveDumpNVP(ar, heightTexture32Bit);
        ArchiveDumpNVP(ar, layers);
        if constexpr(Archive::is_loading::value)
            Validate();
    }
};

struct OD_API TerrainCollisionComponent{
    bool enabled = true, autoRebuild = true, buildAllOnStart = true;
    // Each physics tile uses a square sample grid; world dimensions may be rectangular.
    int tilesX = 16, tilesZ = 16, samplesPerTile = 257, rebuildsPerFrame = 2;
    uint64_t rebuild = 1;
    void Validate();
    static void OnGui(Entity e, Scene& scene);

    template <class Archive> void serialize(Archive& ar){
        ArchiveDumpNVP(ar, enabled);
        ArchiveDumpNVP(ar, autoRebuild);
        ArchiveDumpNVP(ar, buildAllOnStart);
        ArchiveDumpNVP(ar, tilesX);
        ArchiveDumpNVP(ar, tilesZ);
        ArchiveDumpNVP(ar, samplesPerTile);
        ArchiveDumpNVP(ar, rebuildsPerFrame);
        if constexpr(Archive::is_loading::value)
            Validate();
    }
};

struct OD_API TerrainNavmeshDataComponent{
    bool enabled = true;
    float sampleSpacing = 8;
    int maxCellsPerAxis = 256;
    void Validate();
    static void OnGui(Entity e, Scene& scene);

    template <class Archive> void serialize(Archive& ar){
        ArchiveDumpNVP(ar, enabled);
        ArchiveDumpNVP(ar, sampleSpacing);
        ArchiveDumpNVP(ar, maxCellsPerAxis);
        if constexpr(Archive::is_loading::value)
            Validate();
    }
};

struct OD_API TerrainEditorComponent{
    bool edit = false;
    TerrainCpu::BrushMode tool = TerrainCpu::BrushMode::Raise;
    float radius = 64, strength = 10, hardness = 0.25f, flattenHeight = 100, paintStrength = 2;
    float rayDistance = 100000, rayStep = 2;
    int layer = 0;
    Vector4 brushColor = Vector4(1, 0.75f, 0.1f, 1);
    void Validate();
    static void OnGui(Entity e, Scene& scene);

    template <class Archive> void serialize(Archive& ar){
        ArchiveDumpNVP(ar, tool);
        ArchiveDumpNVP(ar, radius);
        ArchiveDumpNVP(ar, strength);
        ArchiveDumpNVP(ar, hardness);
        ArchiveDumpNVP(ar, flattenHeight);
        ArchiveDumpNVP(ar, paintStrength);
        ArchiveDumpNVP(ar, rayDistance);
        ArchiveDumpNVP(ar, rayStep);
        ArchiveDumpNVP(ar, layer);
        ArchiveDumpNVP(ar, brushColor);
    }
};

// Public entry point; automatically called with the engine core modules.
OD_API void TerrainSystemModuleInit();

}
