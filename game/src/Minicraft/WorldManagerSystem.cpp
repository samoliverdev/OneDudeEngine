#include "WorldManagerSystem.h"
#include "ChunkComponent.h"
#include <algorithm>

bool operator<(const _IVector3& a, const _IVector3& b){
    return a.x < b.x ||
           a.x == b.x && (a.y < b.y || a.y == b.y && a.z < b.z);
}

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

}

void WorldManagerSystem::Update(){
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
        Vector3 pos = GridToWorld(chunkSize, i);// i * IVector3(chunkSize, 0, chunkSize);
        //pos += Vector3((chunkSize/2), 0, (chunkSize/2));
        //pos.y = -(chunkSize*chunkHeigtCount);

        //pos += Vector3(chunkSize/2, 0, chunkSize/2);
        trans.LocalPosition(pos);
        trans.LocalScale(Vector3(chunkSize, 0.1f, chunkSize));

        Graphics::DrawWireCube(trans.GetLocalModelMatrix(), Vector3(0, 1, 0), 1);
    }
}

void WorldManagerSystem::LoadChunk(IVector3 coord){
    Entity _chunk = scene->AddEntity("Chunk");
    LogWarning("Add Chunk Entity: %d Coord(%d, %d, %d)", _chunk.Id(), coord.x, coord.y, coord.z);

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
    LogWarning("Destroing Chunk Entity: %d Coord(%d, %d, %d)", loadedChunks[coord].Id(), coord.x, coord.y, coord.z);
    scene->DestroyEntity(loadedChunks[coord].Id());
}

void WorldManagerSystem::HandleLoadUnload(){
    IVector3 currentCoord = WorldToGrid(chunkSize, camPos);
    toLoadCoords.clear();
    for(int x = -chunkLoadDistance; x <= chunkLoadDistance; x++){
        for(int z = -chunkLoadDistance; z <= chunkLoadDistance; z++){
            toLoadCoords.push_back(currentCoord + IVector3(x, 0, z));
        }
    }

    std::vector<IVector3> toRemove;
    for(auto i: loadedChunks){
        if(std::find(toLoadCoords.begin(), toLoadCoords.end(), ToIVector3(i.first)) == toLoadCoords.end()){
            UnLoadChunk(IVector3(i.first.x, i.first.y, i.first.z));
            toRemove.push_back(IVector3(i.first.x, i.first.y, i.first.z));
        }
    }
    for(auto i: toRemove){
        loadedChunks.erase(i);
    }

    for(auto i: toLoadCoords){
        if(loadedChunks.count(_IVector3(i)) <= 0){
            LoadChunk(i);
        }
    }
}