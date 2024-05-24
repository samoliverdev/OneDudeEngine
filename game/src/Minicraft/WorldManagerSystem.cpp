#include "WorldManagerSystem.h"
#include "ChunkComponent.h"
#include <algorithm>

IVector3 WorldToGrid(int cellSize, Vector3 worldPosition){
    int x = math::floor<int>(worldPosition.x / cellSize);
    int y = math::floor<int>(worldPosition.y / cellSize);
    int z = math::floor<int>(worldPosition.z / cellSize);
    return IVector3(x, y, z);
}

Vector3 GridToWorld(int cellSize, IVector3 coord){
    return coord * IVector3(cellSize);
}

WorldManagerSystem::WorldManagerSystem(Scene* inScene):System(inScene){
    chunkSize = 16*4;
    chunkHeigtCount = 24/4;
    chunkLoadDistance = 64/4;
}

void WorldManagerSystem::Update(){
    OD_PROFILE_SCOPE("WorldManagerSystem::Update");

    camPos = scene->GetMainCamera2().GetComponent<TransformComponent>().Position();
    camPos.y = 0;

    HandleLoadUnload();

    auto chunkView = scene->GetRegistry().view<TransformComponent, ChunkComponent, MeshRendererComponent>();
    for(auto e: chunkView){
        TransformComponent& trans = chunkView.get<TransformComponent>(e);
        ChunkComponent& chunk = chunkView.get<ChunkComponent>(e);
        MeshRendererComponent& renderer = chunkView.get<MeshRendererComponent>(e);

        if(chunk.isDirt){
            chunkBuilderLayer.BuildMesh(chunk.data, renderer);
            chunk.isDirt = false;
        }
    }
}

void WorldManagerSystem::OnDrawGizmos(){
    return;

    Transform trans;

    for(auto i: toLoadCoords){
        Vector3 pos = GridToWorld(chunkSize, i.first);// i * IVector3(chunkSize, 0, chunkSize);
        //pos += Vector3((chunkSize/2), 0, (chunkSize/2));
        //pos.y = -(chunkSize*chunkHeigtCount);

        //pos += Vector3(chunkSize/2, 0, chunkSize/2);
        trans.LocalPosition(pos);
        trans.LocalScale(Vector3(chunkSize, 0.1f, chunkSize));

        Graphics::DrawWireCube(trans.GetLocalModelMatrix(), Vector3(0, 1, 0), 1);
    }
}

void _LoadChunk(IVector3 coord, MeshRendererComponent& mesh){
}

void WorldManagerSystem::LoadChunk(IVector3 coord){
    OD_PROFILE_SCOPE("WorldManagerSystem::LoadChunk");

    Entity _chunk = scene->AddEntity("Chunk");
    //LogWarning("Add Chunk Entity: %d Coord(%d, %d, %d)", _chunk.Id(), coord.x, coord.y, coord.z);

    ChunkComponent& chunk = _chunk.AddComponent<ChunkComponent>();
    MeshRendererComponent& mesh = _chunk.AddComponent<MeshRendererComponent>();
    TransformComponent& trans = _chunk.GetComponent<TransformComponent>();

    chunk.data.SetSize(chunkSize, chunkSize*chunkHeigtCount);

    Vector3 pos = GridToWorld(chunkSize, coord); // coord * IVector3(chunkSize);
    pos += Vector3(-(chunkSize/2), 0, -(chunkSize/2));
    pos.y = -floorY;

    trans.Position(pos);
    loadedChunks[coord] = _chunk;

    chunkBuilderLayer.BuildData(coord, chunk.data);
    chunkBuilderLayer.BuildMesh(chunk.data, mesh);
    chunk.isDirt = false;
}

void WorldManagerSystem::UnLoadChunk(IVector3 coord){
    OD_PROFILE_SCOPE("WorldManagerSystem::UnLoadChunk"); 

    //LogWarning("Destroing Chunk Entity: %d Coord(%d, %d, %d)", loadedChunks[coord].Id(), coord.x, coord.y, coord.z);
    scene->DestroyEntity(loadedChunks[coord].Id());
}

void WorldManagerSystem::HandleLoadUnload(){
    OD_PROFILE_SCOPE("WorldManagerSystem::HandleLoadUnload");

    using namespace std::chrono_literals;
    
    {
    OD_PROFILE_SCOPE("WorldManagerSystem::HandleLoadUnload::0") 
    IVector3 currentCoord = WorldToGrid(chunkSize, camPos);
    toLoadCoords.clear();
    for(int x = -chunkLoadDistance; x <= chunkLoadDistance; x++){
        for(int z = -chunkLoadDistance; z <= chunkLoadDistance; z++){
            //toLoadCoords.push_back(currentCoord + IVector3(x, 0, z));
            toLoadCoords[currentCoord + IVector3(x, 0, z)] = true;
        }
    }
    }

    {
        OD_PROFILE_SCOPE("WorldManagerSystem::HandleLoadUnload::1") 
        std::vector<IVector3> toRemove;
        for(auto i: loadedChunks){
            //if(std::find(toLoadCoords.begin(), toLoadCoords.end(), i.first) == toLoadCoords.end()){
            if(toLoadCoords.count(i.first) == false){
                UnLoadChunk(IVector3(i.first.x, i.first.y, i.first.z));
                toRemove.push_back(IVector3(i.first.x, i.first.y, i.first.z));
            }
        }
        for(auto i: toRemove){
            loadedChunks.erase(i);
        }
    }

    {
    OD_PROFILE_SCOPE("WorldManagerSystem::HandleLoadUnload::2") 
    for(auto i: toLoadCoords){
        if(loadedChunks.count(i.first) <= 0){
            //JobSystem::Execute([&](){ LoadChunk(i); });
            LoadChunk(i.first);
        }
    }
    }

    //JobSystem::Wait();

    /*for(auto& i: test2){
        if(i.second.wait_for(0ms) == std::future_status::ready){

        }
    }*/
}