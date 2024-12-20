#pragma once
#include <OD/OD.h>

using namespace OD;

class MeshData;
class NoiseData;

struct TerrainChunkData{
    Ref<NoiseData> noiseData = nullptr;
    Ref<MeshData> meshData = nullptr;
};

struct LODInfo {
    int lod;
    float visibleDstThreshold;
};

class EndlessTerrain: public Script{
public:
    inline static const int mapChunkSize = (240*2) + 1;
    inline static const int levelOfDetail = 4;
    TransformComponent* viewer;

    template <class Archive>
    void serialize(Archive& ar){
        //ArchiveDumpNVP(ar, viewerPosition);
    }

    void OnStart() override;
    virtual void OnDestroy() override;
    virtual void OnUpdate() override;

private:
    std::vector<LODInfo> detailLevels = {
        {2, 0},
        {2, 200},
        {2, 400},
        {2, 600},
        {2, 800},
        //{5, 1000}
    };

    int chunkSize;
    int chunkLoadDistance;
    std::unordered_map<IVector2, Ref<NoiseData>> loadedDatas;
    std::unordered_map<IVector2, Entity> loadedChunks;
    Ref<Material> material;

    struct ToLoadData{
        IVector2 coord;
        //int lod;
        Ref<NoiseData> noise;
        Ref<MeshData> mesh;
    };

    std::unordered_map<IVector2, bool> toLoad;
    std::vector<ToLoadData> toLoadDatas;
    int toLoadCount;

    IVector2 currentCoord;
    Vector2 viewPos;
    Vector2 lastViewPos;

    int _loadingJobs;
    Ref<std::vector<std::thread>> loadingJobs;
    Ref<std::atomic<bool>> loadingJobsDone;

    enum class LoadingJobState{None, RunningJob, EndJob};
    LoadingJobState loadingJobState = LoadingJobState::None;

    int GetLoadInfo(Vector2 viewPos, Vector2 coordPos);
    bool WaitingFinishMeshUpdate();
    void UpdateVisibleChunks();
};