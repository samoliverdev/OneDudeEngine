#include <taskflow/taskflow.hpp>
#include <taskflow/algorithm/for_each.hpp>
#include "EndlessTerrain.h"
#include "Noise.h"
#include "MeshGenerator.h"
#include "Ultis/Ultis.h"

tf::Taskflow taskflow;
tf::Executor executor;

void EndlessTerrain::OnStart(){
    chunkSize = mapChunkSize - 1;
    chunkLoadDistance = 8;
    material = LoadFloorMaterial();
    
    loadingJobsDone = CreateRef<std::atomic<bool>>();
    loadingJobs = CreateRef<std::vector<std::thread>>();

    //GetEntity().GetComponent<TransformComponent>().LocalScale(Vector3(4,4,4));
}

void EndlessTerrain::OnDestroy(){

}

void EndlessTerrain::OnUpdate(){
    if(WaitingFinishMeshUpdate()) return;

    float loadDstThreshold = 50;

    TransformComponent& camTrans = GetEntity().GetScene()->GetMainCamera2().GetComponent<TransformComponent>();
    viewPos = Vector2(camTrans.Position().x, -camTrans.Position().z);

    if(math::distance2(viewPos, lastViewPos) > loadDstThreshold*loadDstThreshold || loadedChunks.size() <= 0){
        UpdateVisibleChunks();
        lastViewPos = viewPos;
    }
}

int EndlessTerrain::GetLoadInfo(Vector2 viewPos, Vector2 coordPos){
    int lodIndex = detailLevels[0].lod;
    for(int i = 1; i < detailLevels.size(); i++){
        if(math::distance2(viewPos, coordPos) > detailLevels[i].visibleDstThreshold*detailLevels[i].visibleDstThreshold){
            lodIndex = detailLevels[i].lod;
        }
    }

    return lodIndex;
}

bool EndlessTerrain::WaitingFinishMeshUpdate(){
    const int maxMeshSubmitByFrame = 1;

    if(
        //loadingJobs->size() > 0
        loadingJobState != LoadingJobState::None 
        || _loadingJobs < toLoadCount
    ){
        OD_LOG_PROFILE("EndlessTerrain::WaitingFinishMeshUpdate");

        /*if(*loadingJobsDone == false) return true; 
        for(auto& i: *loadingJobs) i.join();
        loadingJobs->clear();*/
        
        if(loadingJobState == LoadingJobState::RunningJob) return true;
        if(loadingJobState == LoadingJobState::EndJob){
            loadingJobState = LoadingJobState::None;
            executor.wait_for_all();
            taskflow.clear();
        }
        
        for(int i = 0; _loadingJobs < toLoadCount; i++, _loadingJobs++){
            if(i >= maxMeshSubmitByFrame) break;

            IVector2 coord = toLoadDatas[_loadingJobs].coord;

            if(loadedChunks.count(coord)){
                Entity chunk = loadedChunks[coord];
                MeshRendererComponent& meshRenderer = chunk.GetComponent<MeshRendererComponent>();
                meshRenderer.mesh = toLoadDatas[_loadingJobs].mesh->CreateMesh();
                meshRenderer.material = material;
                meshRenderer.UpdateAABB();

                RigidbodyComponent& rb = chunk.GetComponent<RigidbodyComponent>();
                rb.SetShape(CollisionShape::MeshShape(toLoadDatas[_loadingJobs].mesh->shapeData /*meshRenderer.mesh*/));

                //meshRenderer.mesh->ClearRuntimeData();
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

                //meshRenderer.mesh->ClearRuntimeData();
            }
        }
        return true;
    }

    return false;
}

void EndlessTerrain::UpdateVisibleChunks(){
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

    if(toLoadCount == 0) return;

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

    _loadingJobs = 0;

    /*
    *loadingJobsDone = false;
    loadingJobs->push_back(std::thread([&](){
        //Platform::BeginOffscreenContextCurrent();
        for(int i = 0; i < toLoadCount; i++){
            Vector2 coord = toLoadDatas[i].coord;
            Vector2 center = {coord.x * chunkSize, coord.y * chunkSize};

            if(toLoadDatas[i].noise == nullptr) toLoadDatas[i].noise = Noise::GenerateNoiseMap(mapChunkSize, mapChunkSize, 50, 1, 4, 1, 1, center);
            toLoadDatas[i].mesh = MeshGenerator::GenerateTerrainMesh(toLoadDatas[i].noise, 50, levelOfDetail);
            //toLoadMesh[i]->out = toLoadMesh[i]->CreateMeshOff();
        }
        //Platform::EndOffscreenContextCurrent();
        *loadingJobsDone = true;
    }));
    */

    ///*
    
    //taskflow.clear();
    loadingJobState = LoadingJobState::RunningJob;
    taskflow.for_each_index(0, toLoadCount, 1, [this](int i){
        Vector2 coord = toLoadDatas[i].coord;
        Vector2 center = {coord.x * chunkSize, coord.y * chunkSize};
        if(toLoadDatas[i].noise == nullptr) toLoadDatas[i].noise = Noise::GenerateNoiseMap(mapChunkSize, mapChunkSize, 50, 1, 4, 1, 1, center);
        toLoadDatas[i].mesh = MeshGenerator::GenerateTerrainMesh(toLoadDatas[i].noise, 50, levelOfDetail);
    }, tf::GuidedPartitioner(20));

    /*taskflow.clear();
    loadingJobState = LoadingJobState::RunningJob;
    for(int i = 0; i < toLoadCount; i++){
        taskflow.emplace([&, i](){
            Vector2 coord = toLoadDatas[i].coord;
            Vector2 center = {coord.x * chunkSize, coord.y * chunkSize};
            if(toLoadDatas[i].noise == nullptr) toLoadDatas[i].noise = Noise::GenerateNoiseMap(EndlessTerrain::mapChunkSize, EndlessTerrain::mapChunkSize, 50, 1, 4, 1, 1, center);
            toLoadDatas[i].mesh = MeshGenerator::GenerateTerrainMesh(toLoadDatas[i].noise, 50, levelOfDetail);
        });
    }*/

    executor.run(taskflow, [this](){ loadingJobState = LoadingJobState::EndJob; });
    //*/
    }
}