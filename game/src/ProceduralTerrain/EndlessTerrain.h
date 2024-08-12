#pragma once
#include <OD/OD.h>
#include <thread>
#include <future>
#include "Noise.h"
#include "MeshGenerator.h"
#include "Ultis/Ultis.h"

using namespace OD;

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
    inline static const int levelOfDetail = 1;
    TransformComponent* viewer;

    template <class Archive>
    void serialize(Archive& ar){
        //ArchiveDumpNVP(ar, viewerPosition);
    }

    inline void OnStart() override{
        chunkSize = mapChunkSize - 1;
        chunkLoadDistance = 4;
        material = LoadFloorMaterial();
        loadingJobsDone = CreateRef<std::atomic<bool>>();
        loadingJobs = CreateRef<std::vector<std::thread>>();

        //GetEntity().GetComponent<TransformComponent>().LocalScale(Vector3(4,4,4));
    }

    inline virtual void OnDestroy() override{

    }
    
    inline virtual void OnUpdate() override{
        if(WaitingFinishMeshUpdate()) return;

        float loadDstThreshold = 50;

        TransformComponent& camTrans = GetEntity().GetScene()->GetMainCamera2().GetComponent<TransformComponent>();
        viewPos = Vector2(camTrans.Position().x, -camTrans.Position().z);

        if(math::distance2(viewPos, lastViewPos) > loadDstThreshold*loadDstThreshold || loadedChunks.size() <= 0){
            UpdateVisibleChunks();
            lastViewPos = viewPos;
        }
    }

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

    inline int GetLoadInfo(Vector2 viewPos, Vector2 coordPos){
        int lodIndex = detailLevels[0].lod;
        for(int i = 1; i < detailLevels.size(); i++){
            if(math::distance2(viewPos, coordPos) > detailLevels[i].visibleDstThreshold*detailLevels[i].visibleDstThreshold){
                lodIndex = detailLevels[i].lod;
            }
        }

        return lodIndex;
    }

    inline bool WaitingFinishMeshUpdate(){
        const int maxMeshSubmitByFrame = 1;

        if(loadingJobs->size() > 0 || _loadingJobs < toLoadCount){
            if(*loadingJobsDone == false) return true; 
            OD_LOG_PROFILE("EndlessTerrain::WaitingFinishMeshUpdate");
            for(auto& i: *loadingJobs) i.join();
            loadingJobs->clear();

            for(int i = 0; _loadingJobs < toLoadCount; i++, _loadingJobs++){
                if(i > maxMeshSubmitByFrame) break;

                IVector2 coord = toLoadDatas[_loadingJobs].coord;

                if(loadedChunks.count(coord)){
                    Entity chunk = loadedChunks[coord];
                    MeshRendererComponent& meshRenderer = chunk.GetComponent<MeshRendererComponent>();
                    meshRenderer.mesh = toLoadDatas[_loadingJobs].mesh->CreateMesh();
                    meshRenderer.material = material;
                    meshRenderer.UpdateAABB();

                    RigidbodyComponent& rb = chunk.GetComponent<RigidbodyComponent>();
                    rb.SetShape(CollisionShape::MeshShape(toLoadDatas[_loadingJobs].mesh->shapeData /*meshRenderer.mesh*/));

                    meshRenderer.mesh->ClearRuntimeData();
                } else {
                    Vector3 pos(coord.x * (float)chunkSize, 0, -(coord.y * (float)chunkSize));
                    Assert(toLoadDatas[_loadingJobs].mesh != nullptr);

                    Entity chunk = GetEntity().GetScene()->AddEntity("Chunk");
                    GetEntity().GetScene()->SetParent(GetEntity().Id(), chunk.Id());
                    TransformComponent& trans = chunk.GetComponent<TransformComponent>();
                    trans.LocalPosition(pos);
                    MeshRendererComponent& meshRenderer = chunk.AddComponent<MeshRendererComponent>();
                    meshRenderer.mesh = toLoadDatas[_loadingJobs].mesh->CreateMesh();
                    meshRenderer.material = material;
                    meshRenderer.UpdateAABB();

                    RigidbodyComponent& rb = chunk.AddOrGetComponent<RigidbodyComponent>();
                    rb.SetShape(CollisionShape::MeshShape(toLoadDatas[_loadingJobs].mesh->shapeData /*meshRenderer.mesh*/));
                    rb.SetType(RigidbodyComponent::Type::Static);
                    rb.Mass(0);
                    
                    loadedChunks[coord] = chunk;

                    meshRenderer.mesh->ClearRuntimeData();
                }
            }
            return true;
        }
        return false;
    }

    inline void UpdateVisibleChunks(){
        //OD_LOG_PROFILE("EndlessTerrain::UpdateVisibleChunks2");
        
        currentCoord = IVector2(
            math::round(viewPos.x / chunkSize), 
            math::round(viewPos.y / chunkSize)
        );

        {
        OD_LOG_PROFILE("EndlessTerrain::UpdateVisibleChunks2::1");
        //toLoad.clear();
        //toLoadCoords.clear();
        //toLoadLod.clear();
        //toLoadNoise.clear();
        //toLoadMesh.clear();
        //toLoadDatas.clear();
        toLoadCount = 0;
        }

        {
        OD_LOG_PROFILE("EndlessTerrain::UpdateVisibleChunks2::2");
        for(int yOffset = -chunkLoadDistance; yOffset <= chunkLoadDistance; yOffset++){
            for(int xOffset = -chunkLoadDistance; xOffset <= chunkLoadDistance; xOffset++){
                IVector2 viewedChunkCoord(currentCoord.x + xOffset, currentCoord.y + yOffset);
                
                if(loadedChunks.count(viewedChunkCoord) > 0) continue;
                if(toLoadDatas.size() <= toLoadCount) toLoadDatas.push_back(ToLoadData());
                
                toLoad[viewedChunkCoord] = true;
                toLoadDatas[toLoadCount].coord = viewedChunkCoord;
                //toLoadDatas[toLoadCount].lod = GetLoadInfo(viewPos, Vector2(viewedChunkCoord.x * chunkSize, viewedChunkCoord.y * chunkSize));
                toLoadDatas[toLoadCount].noise = loadedDatas.count(viewedChunkCoord) ? loadedDatas[viewedChunkCoord] : nullptr;
                toLoadDatas[toLoadCount].mesh = nullptr;
                
                toLoadCount += 1;
            }
        }
        }

        /*{
        OD_LOG_PROFILE("EndlessTerrain::UpdateVisibleChunks2::3");
        std::vector<IVector2> toRemove;
        for(auto i: loadedChunks){
            if(toLoad.count(i.first) == false){
                GetEntity().GetScene()->DestroyEntity(i.second.Id());
                toRemove.push_back(i.first);
            }
        }
        for(auto i: toRemove){
            loadedChunks.erase(i);
        }
        }*/

        {
        OD_LOG_PROFILE("EndlessTerrain::UpdateVisibleChunks2::4");
        *loadingJobsDone = false;
        loadingJobs->push_back(std::thread([&](){
            //Platform::BeginOffscreenContextCurrent();
            for(int i = 0; i < toLoadCount; i++){
                Vector2 coord = toLoadDatas[i].coord;
                Vector2 center = {coord.x * chunkSize, coord.y * chunkSize};

                if(toLoadDatas[i].noise == nullptr) toLoadDatas[i].noise = Noise::GenerateNoiseMap(mapChunkSize, mapChunkSize, 50, 1, 4, 1, 1, center);
                toLoadDatas[i].mesh = MeshGenerator::GenerateTerrainMesh(toLoadDatas[i].noise, 50, levelOfDetail /*toLoadDatas[i].lod*/);
                //toLoadMesh[i]->out = toLoadMesh[i]->CreateMeshOff();
            }
            //Platform::EndOffscreenContextCurrent();
            *loadingJobsDone = true;
        }));
        _loadingJobs = 0;
        }
    }
};