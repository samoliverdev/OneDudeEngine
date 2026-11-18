#pragma once
#include "OD/Scene/Scene.h"
#include "OD/Core/ResourceManager.h"
#include "OD/Terrain/Heightmap.h"
#include "TerrainAlgorithms.h"

namespace OD{

// Separate, independently sized, linear RGBA weights. Four surface layers.
struct OD_API TerrainPaintMap : Resource{
    int width = 0, height = 0;
    std::vector<uint8_t> weights;

    bool Valid() const{
        return width >= 2 && height >= 2 && weights.size() == size_t(width) * height * 4;
    }

    bool Resize(int columns, int rows);
    bool LoadFromFile(const std::string& file) override;
    bool Save(const std::string& file, SaveType type) override;

    std::vector<std::string> GetFileAssociations() override{
        return {".terrainpaint"};
    }

    size_t RamUsage() override{
        return weights.size();
    }
};

struct TerrainEditingState;

struct TerrainDirtyRegion{
    int x0 = 0, z0 = 0, x1 = 0, z1 = 0;
    uint64_t revision = 0;
};

struct OD_API TerrainDataComponent{
    Ref<Heightmap> heightmap;
    Ref<TerrainPaintMap> paintmap;
    Vector3 size = Vector3(32000, 1000, 32000);
    int historyMiB = 64;
    int maxBrushSamples = 1048576;

    TerrainDataComponent() = default;
    TerrainDataComponent(const TerrainDataComponent& other);
    TerrainDataComponent& operator=(const TerrainDataComponent& other);
    TerrainDataComponent(TerrainDataComponent&&) noexcept = default;
    TerrainDataComponent& operator=(TerrainDataComponent&&) noexcept = default;

    TerrainCpu::HeightfieldView View() const;
    void Validate();
    bool Create(int columns, int rows, int paintColumns = 1024, int paintRows = 1024, float initialHeight = 0);
    bool ImportRaw16(const std::string& file, int columns, int rows, bool bigEndian = false);
    bool ExportRaw16(const std::string& file, bool bigEndian = false) const;
    bool SaveAssets(const std::string& heightFile, const std::string& paintFile);
    void Touch(bool paint = false);
    void TouchRegion(bool paint, int x0, int z0, int x1, int z1);
    TerrainDirtyRegion ChangesSince(uint64_t from, bool paint) const;

    uint64_t Revision() const{
        return revision;
    }

    uint64_t PaintRevision() const{
        return paintRevision;
    }

    void BeginStroke();
    void EndStroke();
    bool ApplyBrush(const TerrainCpu::Brush& brush, int paintLayer = 0);
    bool Undo();
    bool Redo();
    bool CanUndo() const;
    bool CanRedo() const;
    void ClearHistory();
    static void OnGui(Entity e, Scene& scene);

    template <class Archive> void serialize(Archive& ar){
        ArchiveDumpNVP(ar, size);
        ArchiveDumpNVP(ar, historyMiB);
        ArchiveDumpNVP(ar, maxBrushSamples);
        ResourceRefSerialize<Heightmap> heights(heightmap);
        ArchiveDumpNVP(ar, heights);
        ResourceRefSerialize<TerrainPaintMap> paint(paintmap);
        ArchiveDumpNVP(ar, paint);
        if constexpr(Archive::is_loading::value){
            Validate();
            Touch();
            Touch(true);
            ClearHistory();
        }
    }

private:
    uint64_t revision = 1, paintRevision = 1;
    Ref<TerrainEditingState> editing;
    std::vector<TerrainDirtyRegion> heightChanges, paintChanges;
    void MakeEditable(bool paint);
};

}
