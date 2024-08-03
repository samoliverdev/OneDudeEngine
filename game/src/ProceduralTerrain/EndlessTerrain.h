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
    inline static const int mapChunkSize = 241;
    inline static const int levelOfDetail = 0;
    TransformComponent* viewer;

    template <class Archive>
    void serialize(Archive& ar){
        //ArchiveDumpNVP(ar, viewerPosition);
    }

    inline void OnStart() override{
        chunkSize = mapChunkSize-1;
        chunkLoadDistance = 8;
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
            UpdateVisibleChunks2();
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

    std::unordered_map<IVector2, bool> toLoad;
    std::vector<IVector2> toLoadCoords;
    std::vector<int> toLoadLod;
    std::vector<Ref<NoiseData>> toLoadNoise;
    std::vector<Ref<MeshData>> toLoadMesh;

    IVector2 currentCoord;
    Vector2 viewPos;
    Vector2 lastViewPos;

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

    inline void UpdateVisibleChunks(){
        currentCoord = IVector2(
            math::round(viewPos.x / chunkSize), 
            math::round(viewPos.y / chunkSize)
        );

        toLoad.clear();
        toLoadCoords.clear();
        toLoadLod.clear();
        toLoadNoise.clear();
        toLoadMesh.clear();

        for(int yOffset = -chunkLoadDistance; yOffset <= chunkLoadDistance; yOffset++){
            for(int xOffset = -chunkLoadDistance; xOffset <= chunkLoadDistance; xOffset++){
                IVector2 viewedChunkCoord(currentCoord.x + xOffset, currentCoord.y + yOffset);
                toLoad[viewedChunkCoord] = true;
                
                //if(loadedChunks.count(viewedChunkCoord) > 0) continue;

                toLoadCoords.push_back(viewedChunkCoord);
                toLoadLod.push_back(GetLoadInfo(viewPos, Vector2(viewedChunkCoord.x * chunkSize, viewedChunkCoord.y * chunkSize)));
                toLoadNoise.push_back(nullptr);
                toLoadMesh.push_back(nullptr);
            }
        }

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

        JobSystem::Dispatch(toLoadCoords.size(), toLoadCoords.size()/4, [&](JobDispatchArgs args){
            Vector2 coord = toLoadCoords[args.jobIndex];
            Vector2 center = {coord.x * chunkSize, coord.y * chunkSize};

            toLoadNoise[args.jobIndex] = Noise::GenerateNoiseMap(mapChunkSize, mapChunkSize, 50, 1, 4, 1, 1, center);
            toLoadMesh[args.jobIndex] = MeshGenerator::GenerateTerrainMesh(toLoadNoise[args.jobIndex], 50, toLoadLod[args.jobIndex]);
        });
        JobSystem::Wait();
        /*for(int i = 0; i < toLoadCoords.size(); i++){
            Vector2 coord = toLoadCoords[i];
            Vector2 center = {coord.x * chunkSize, coord.y * chunkSize};

            toLoadNoise[i] = Noise::GenerateNoiseMap(mapChunkSize, mapChunkSize, 50, 1, 4, 1, 1, center);
            toLoadMesh[i] = MeshGenerator::GenerateTerrainMesh(toLoadNoise[i], 50, levelOfDetail);
        }*/

        for(int i = 0; i < toLoadCoords.size(); i++){
            IVector2 coord = toLoadCoords[i];

            if(loadedChunks.count(coord)){
                Entity chunk = loadedChunks[coord];
                MeshRendererComponent& meshRenderer = chunk.GetComponent<MeshRendererComponent>();
                meshRenderer.mesh = toLoadMesh[i]->CreateMesh();
                meshRenderer.material = material;
                meshRenderer.UpdateAABB();

                /*meshRenderer.mesh->vertices.clear();
                meshRenderer.mesh->uv.clear();
                meshRenderer.mesh->normals.clear();*/
            } else {
                Vector3 pos(coord.x * (float)chunkSize, 0, -(coord.y * (float)chunkSize));
                Assert(toLoadMesh[i] != nullptr);

                Entity chunk = GetEntity().GetScene()->AddEntity("Chunk");
                GetEntity().GetScene()->SetParent(GetEntity().Id(), chunk.Id());
                TransformComponent& trans = chunk.GetComponent<TransformComponent>();
                trans.LocalPosition(pos);
                MeshRendererComponent& meshRenderer = chunk.AddComponent<MeshRendererComponent>();
                meshRenderer.mesh = toLoadMesh[i]->CreateMesh();
                meshRenderer.material = material;
                meshRenderer.UpdateAABB();

                /*meshRenderer.mesh->vertices.clear();
                meshRenderer.mesh->uv.clear();
                meshRenderer.mesh->normals.clear();*/
                
                loadedChunks[coord] = chunk;
                loadedDatas[coord] = toLoadNoise[i];
            }
        }
    }

    inline bool WaitingFinishMeshUpdate(){
        if(loadingJobs->size() > 0){
            if(*loadingJobsDone == false) return true; 
            OD_LOG_PROFILE("EndlessTerrain::WaitingFinishMeshUpdate");
            for(auto& i: *loadingJobs) i.join();

            for(int i = 0; i < toLoadCoords.size(); i++){
                IVector2 coord = toLoadCoords[i];

                if(loadedChunks.count(coord)){
                    Entity chunk = loadedChunks[coord];
                    MeshRendererComponent& meshRenderer = chunk.GetComponent<MeshRendererComponent>();
                    meshRenderer.mesh = toLoadMesh[i]->CreateMesh();
                    meshRenderer.material = material;
                    meshRenderer.UpdateAABB();
                } else {
                    Vector3 pos(coord.x * (float)chunkSize, 0, -(coord.y * (float)chunkSize));
                    Assert(toLoadMesh[i] != nullptr);

                    Entity chunk = GetEntity().GetScene()->AddEntity("Chunk");
                    GetEntity().GetScene()->SetParent(GetEntity().Id(), chunk.Id());
                    TransformComponent& trans = chunk.GetComponent<TransformComponent>();
                    trans.LocalPosition(pos);
                    MeshRendererComponent& meshRenderer = chunk.AddComponent<MeshRendererComponent>();
                    meshRenderer.mesh = toLoadMesh[i]->CreateMesh();
                    meshRenderer.material = material;
                    meshRenderer.UpdateAABB();
                    
                    loadedChunks[coord] = chunk;
                }
            }

            loadingJobs->clear();
            return true;
        }
        return false;
    }

    inline void UpdateVisibleChunks2(){
        currentCoord = IVector2(
            math::round(viewPos.x / chunkSize), 
            math::round(viewPos.y / chunkSize)
        );

        toLoad.clear();
        toLoadCoords.clear();
        toLoadLod.clear();
        toLoadNoise.clear();
        toLoadMesh.clear();

        for(int yOffset = -chunkLoadDistance; yOffset <= chunkLoadDistance; yOffset++){
            for(int xOffset = -chunkLoadDistance; xOffset <= chunkLoadDistance; xOffset++){
                IVector2 viewedChunkCoord(currentCoord.x + xOffset, currentCoord.y + yOffset);
                toLoad[viewedChunkCoord] = true;
                
                //if(loadedChunks.count(viewedChunkCoord) > 0) continue;

                toLoadCoords.push_back(viewedChunkCoord);
                toLoadLod.push_back(GetLoadInfo(viewPos, Vector2(viewedChunkCoord.x * chunkSize, viewedChunkCoord.y * chunkSize)));
                toLoadNoise.push_back(loadedDatas.count(viewedChunkCoord) ? loadedDatas[viewedChunkCoord] : nullptr);
                toLoadMesh.push_back(nullptr);
            }
        }

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

        *loadingJobsDone = false;
        loadingJobs->push_back(std::thread([&](){
            for(int i = 0; i < toLoadCoords.size(); i++){
                Vector2 coord = toLoadCoords[i];
                Vector2 center = {coord.x * chunkSize, coord.y * chunkSize};

                if(toLoadNoise[i] == nullptr) toLoadNoise[i] = Noise::GenerateNoiseMap(mapChunkSize, mapChunkSize, 50, 1, 4, 1, 1, center);
                toLoadMesh[i] = MeshGenerator::GenerateTerrainMesh(toLoadNoise[i], 50, toLoadLod[i]);
            }
            *loadingJobsDone = true;
        }));
    }
};