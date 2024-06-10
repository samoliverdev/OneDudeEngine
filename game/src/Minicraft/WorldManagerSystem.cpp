#include "WorldManagerSystem.h"
#include "ChunkComponent.h"
#include "ChunkBuilderLayer.h"
#include <algorithm>

inline IVector3 InvertZ(IVector3 v){ return {v.x, v.y, -v.z}; }

IVector3 WorldToGrid(int cellSize, Vector3 worldPosition, bool invertZ = true){
    int x = math::floor(worldPosition.x / cellSize);
    int y = math::floor(worldPosition.y / cellSize);
    int z = math::floor(worldPosition.z / cellSize);
    if(invertZ) z = math::floor((-worldPosition.z) / cellSize);
    return IVector3(x, y, z);
}

Vector3 GridToWorld(int cellSize, IVector3 coord){
    return coord * IVector3(cellSize);
}

WorldManagerSystem::WorldManagerSystem(Scene* inScene):System(inScene){
    chunkSize = 16*2;
    chunkHeigtCount = 24/2;
    chunkLoadDistance = 8/2;
    floorY = 0;
    chunkBuilderLayer = CreateRef<ChunkBuilderLayer>(this);
}

void WorldManagerSystem::Update(){
    OD_PROFILE_SCOPE("WorldManagerSystem::Update");

    camPos = scene->GetMainCamera2().GetComponent<TransformComponent>().Position();
    camPos.y = 0;

    /*if(loadedChunks.size() == 0) LoadChunk(IVector3(0, 0, 0)); 
    return;*/

    HandleLoadUnload3();
    if(loadingMeshJobs > 0 || loadingMeshJobs > 0) return;
    if(loadingJobs.size() > 0) return;

    HandleChunkEdit();
    HandleInput();
    HandleChunkDirt();
}

void WorldManagerSystem::OnRender(){
    if(hasPlaceAndHighlightPos == false) return;
    Graphics::DrawWireCube(Transform(placeBlockPos+Vector3(0.5f, 0.5f, 0.5f)).GetLocalModelMatrix(), Vector3(0, 0, 1), 1);
}

void WorldManagerSystem::OnDrawGizmos(){
    return;
    TransformComponent& camTrans = scene->GetMainCamera2().GetComponent<TransformComponent>();
    Vector3 editVoxelPos = camTrans.Position() + (-camTrans.Forward() * 2.0f);
    editVoxelPos.x = math::floor(editVoxelPos.x);
    editVoxelPos.y = math::floor(editVoxelPos.y);
    editVoxelPos.z = math::floor(editVoxelPos.z);

    /*if(CheckForVoxel(editVoxelPos)){
        Graphics::DrawWireCube(Transform(editVoxelPos).GetLocalModelMatrix(), Vector3(0, 0, 1), 1);
    } else {
        Graphics::DrawWireCube(Transform(editVoxelPos).GetLocalModelMatrix(), Vector3(0, 1, 0), 1);
    }
    return;*/

    //Source: https://github.com/b3agz/Code-A-Game-Like-Minecraft-In-Unity/blob/master/08-create-and-destroy-blocks/Assets/Scripts/Player.cs
    float checkIncrement = 0.1f;
    float step = checkIncrement;
    float reach = 8.0f;
    Vector3 lastPos;
    while(step < reach){
        Vector3 pos = camTrans.Position() + (-camTrans.Forward() * step);
        if(CheckForVoxelNotEmpty(pos)){
            Vector3 addPos = Vector3(math::floor(pos.x), math::floor(pos.y), math::floor(pos.z));
            //highlightBlock.position = addPos
            //placeBlock.position = lastPos;
            Graphics::DrawWireCube(Transform(lastPos+Vector3(0.5f, 0.5f, 0.5f)).GetLocalModelMatrix(), Vector3(0, 1, 0), 1);
            return;
        }

        lastPos = Vector3(math::floor(pos.x), math::floor(pos.y), math::floor(pos.z));
        step += checkIncrement;

        if(Input::IsKeyDown(KeyCode::Q)){
            ChunkComponent* chunkData = GetChunkFromWorldPos(lastPos);
            if(chunkData != nullptr && chunkData->data.chunkData != nullptr){
                chunkData->data.chunkData->SetVoxelFromGlobalVector3(InvertZ(lastPos), Voxel{1});
                chunkData->isDirt = true;
                LogInfo("Try Edit Chunk");
            }
        }
    }
    return;

    Transform trans;
    for(auto i: toLoadCoords){
        Vector3 pos = GridToWorld(chunkSize, i.first);// i * IVector3(chunkSize, 0, chunkSize);
        //pos += Vector3((chunkSize/2), 0, (chunkSize/2));
        //pos.y = -(chunkSize*chunkHeigtCount);

        trans.LocalPosition(pos + Vector3(chunkSize/2, 0, (-chunkSize/2)));
        trans.LocalScale(Vector3(chunkSize, 0.1f, chunkSize));

        Graphics::DrawWireCube(trans.GetLocalModelMatrix(), Vector3(0, 1, 0), 1);

        trans.LocalPosition(pos);
        trans.LocalScale(Vector3(1, 1, 1));
        Graphics::DrawWireCube(trans.GetLocalModelMatrix(), Vector3(0, 0, 1), 1);
    }
}

void WorldManagerSystem::HandleChunkEdit(){
    TransformComponent& camTrans = scene->GetMainCamera2().GetComponent<TransformComponent>();

    float checkIncrement = 0.1f;
    float step = checkIncrement;
    float reach = 8.0f*4;
    Vector3 lastPos;
    hasPlaceAndHighlightPos = false;

    while(step < reach){
        Vector3 pos = camTrans.Position() + (-camTrans.Forward() * step);
        if(CheckForVoxelNotEmpty(pos)){
            Vector3 addPos = Vector3(math::floor(pos.x), math::floor(pos.y), math::floor(pos.z));
            highlightBlockPos = addPos;
            placeBlockPos = lastPos;
            hasPlaceAndHighlightPos = true;
            return;
        } 
        lastPos = Vector3(math::floor(pos.x), math::floor(pos.y), math::floor(pos.z));
        step += checkIncrement;
    }

    hasPlaceAndHighlightPos = false;
}

void WorldManagerSystem::HandleInput(){
    if(Input::IsKeyDown(KeyCode::Q) && hasPlaceAndHighlightPos){
        ChunkComponent* chunkData = GetChunkFromWorldPos(placeBlockPos);
        if(chunkData != nullptr && chunkData->data.chunkData != nullptr){
            chunkData->data.chunkData->SetVoxelFromGlobalVector3(InvertZ(placeBlockPos), Voxel{1});
            chunkData->isDirt = true;
            LogInfo("Try Edit Chunk");
        }
    }

    if(Input::IsKeyDown(KeyCode::E) && hasPlaceAndHighlightPos){
        ChunkComponent* chunkData = GetChunkFromWorldPos(highlightBlockPos);
        if(chunkData != nullptr && chunkData->data.chunkData != nullptr){
            chunkData->data.chunkData->SetVoxelFromGlobalVector3(InvertZ(highlightBlockPos), Voxel{0});
            chunkData->isDirt = true;
            LogInfo("Try Edit Chunk");
        }
    }
}

void WorldManagerSystem::HandleChunkDirt(){
    auto chunkView = scene->GetRegistry().view<TransformComponent, ChunkComponent>();
    for(auto e: chunkView){
        TransformComponent& trans = chunkView.get<TransformComponent>(e);
        ChunkComponent& chunk = chunkView.get<ChunkComponent>(e);
        
        if(chunk.isDirt){
            chunkBuilderLayer->BuildMesh2(chunk.data);
            UpdateChunkMeshEntity(chunk.opaqueMesh, chunk.data.opaqueMesh, chunkBuilderLayer->materialOpaque);
            UpdateChunkMeshEntity(chunk.waterMesh, chunk.data.waterMesh, chunkBuilderLayer->materialWater);
            LogInfo("Updating Chunk");
            chunk.isDirt = false;
        }
    }
}

bool WorldManagerSystem::CheckForVoxelNotEmpty(Vector3 worldPos, bool invertZ){
    IVector3 chunkCoord = WorldToGrid(chunkSize, worldPos, invertZ);
    chunkCoord.y = 0;
    if(invertZ) worldPos.z = -worldPos.z;
    //LogInfo("CheckForVoxel x: %d z: %d", chunkCoord.x, chunkCoord.z);
    if(loadedChunksB.count(chunkCoord) <= 0) return false;
    if(worldPos.y < 0 || worldPos.y > (chunkSize*chunkHeigtCount)) return false;
    //if(loadedChunksB[chunkCoord]->GetVoxelFromGlobalVector3(worldPos).id == 0) return false;
    //if(loadedChunksB[chunkCoord]->GetVoxelFromGlobalVector3(worldPos, Vector3(chunkCoord.x*chunkSize, 0.0f, chunkCoord.z*chunkSize)).id == 0) return false;
    //return true;
    return loadedChunksB[chunkCoord].chunkData->GetVoxelFromGlobalVector3(worldPos).id != 0;
}

bool WorldManagerSystem::CheckForVoxelIsEmpty(Vector3 worldPos, bool invertZ){
    IVector3 chunkCoord = WorldToGrid(chunkSize, worldPos, invertZ);
    chunkCoord.y = 0;
    if(invertZ) worldPos.z = -worldPos.z;
    if(loadedChunksB.count(chunkCoord) <= 0) return false;
    if(worldPos.y < 0 || worldPos.y > (chunkSize*chunkHeigtCount)) return false;
    return loadedChunksB[chunkCoord].chunkData->GetVoxelFromGlobalVector3(worldPos).id == 0;
}

int WorldManagerSystem::CheckForVoxel(Vector3 worldPos, bool invertZ){
    IVector3 chunkCoord = WorldToGrid(chunkSize, worldPos, invertZ);
    chunkCoord.y = 0;
    if(invertZ) worldPos.z = -worldPos.z;
    if(loadedChunksB.count(chunkCoord) <= 0) return -1;
    if(worldPos.y < 0 || worldPos.y > (chunkSize*chunkHeigtCount)) return -1;
    return (int)loadedChunksB[chunkCoord].chunkData->GetVoxelFromGlobalVector3(worldPos).id;
}

ChunkComponent* WorldManagerSystem::GetChunkFromWorldPos(Vector3 worldPos){
    IVector3 chunkCoord = WorldToGrid(chunkSize, worldPos);
    chunkCoord.y = 0;
    worldPos.z = -worldPos.z;
    if(loadedChunksB.count(chunkCoord) <= 0) return nullptr;

    return loadedChunks[chunkCoord].TryGetComponent<ChunkComponent>();
    
    //return loadedChunksB[chunkCoord];
}

void WorldManagerSystem::LoadChunk(IVector3 coord){
    OD_PROFILE_SCOPE("WorldManagerSystem::LoadChunk");
    Assert(false);
}

Entity WorldManagerSystem::BuildChunkMeshEntity(Entity chunkRoot, Ref<Mesh>& mesh, Ref<Material>& material){
    Entity subChunk = scene->AddEntity("SubChunk");
    scene->SetParent(chunkRoot.Id(), subChunk.Id());
    
    TransformComponent& trans = subChunk.GetComponent<TransformComponent>();
    trans.LocalPosition(Vector3Zero);
    trans.LocalEulerAngles(Vector3Zero);

    MeshRendererComponent& meshComponent = subChunk.AddComponent<MeshRendererComponent>();
    meshComponent.material = material;
    meshComponent.mesh = mesh;
    meshComponent.mesh->UpdateMesh();
    meshComponent.UpdateAABB();

    /*meshComponent.mesh->vertices.clear();
    meshComponent.mesh->normals.clear();
    meshComponent.mesh->uv.clear();
    meshComponent.mesh->indices.clear();*/

    return subChunk;
}

void WorldManagerSystem::UpdateChunkMeshEntity(Entity subChunk, Ref<Mesh>& mesh, Ref<Material>& material){
    MeshRendererComponent& meshComponent = subChunk.GetComponent<MeshRendererComponent>();
    meshComponent.material = material;
    meshComponent.mesh = mesh;
    meshComponent.mesh->UpdateMesh();
    meshComponent.UpdateAABB();
}

void WorldManagerSystem::UnLoadChunk(IVector3 coord){
    OD_PROFILE_SCOPE("WorldManagerSystem::UnLoadChunk"); 

    //LogWarning("Destroing Chunk Entity: %d Coord(%d, %d, %d)", loadedChunks[coord].Id(), coord.x, coord.y, coord.z);
    scene->DestroyEntity(loadedChunks[coord].Id());
}

void WorldManagerSystem::HandleLoadUnload(){
    OD_PROFILE_SCOPE("WorldManagerSystem::HandleLoadUnload");
    using namespace std::chrono_literals;

    if(loadingChunkDataJobs > 0){
        if(threadPool->busy() == true) return;
        loadingChunkDataJobs = 0;

        {
        //SimpleTimer timer([](float duration){ LogInfo("WorldManagerSystem::HandleLoadUnload::2 %.3f.ms", duration); });
        for(int i = 0; i < toLoadCoordsA.size(); i++){
            loadedChunksB[toLoadCoordsA[i]].chunkData = toLoadCoordsB[i].chunkData;
            //*(toLoadCoordsB[i].done) = false;
        }
        }

        {
        //SimpleTimer timer([](float duration){ LogInfo("WorldManagerSystem::HandleLoadUnload::3 %.3f.ms", duration); });
        for(int i = 0; i < toLoadCoordsA.size(); i++){
            threadPool->enqueue([&, i](){
                //chunkBuilderLayer.BuildMesh(*toLoadCoordsB[i].chunkData, toLoadCoordsB[i].mesh);
                chunkBuilderLayer->BuildMesh2(toLoadCoordsB[i]);
                //*toLoadCoordsB[i].done = true; 
            });
            loadingMeshJobs += 1;
        }
        }
        /*{
        //SimpleTimer timer([](float duration){ LogInfo("WorldManagerSystem::HandleLoadUnload::3 %.3f.ms", duration); });
        loadingMeshJobs = toLoadCoordsA.size();
        threadPool->enqueue([&](){
            for(int i = 0; i < toLoadCoordsA.size(); i++){
                chunkBuilderLayer->BuildMesh2(toLoadCoordsB[i]);
            }
        });
        }*/
        return;
    }

    if(loadingMeshJobs > 0){
        if(threadPool->busy() == true) return;
        loadingMeshJobs = 0;

        for(int i = 0; i < toLoadCoordsA.size(); i++){
            Entity _chunk = scene->AddEntity("Chunk");
            ChunkComponent& chunk = _chunk.AddComponent<ChunkComponent>();
            TransformComponent& trans = _chunk.GetComponent<TransformComponent>();
            
            chunk.opaqueMesh = BuildChunkMeshEntity(_chunk, toLoadCoordsB[i].opaqueMesh, chunkBuilderLayer->materialOpaque);
            chunk.waterMesh = BuildChunkMeshEntity(_chunk, toLoadCoordsB[i].waterMesh, chunkBuilderLayer->materialWater);

            chunk.data = toLoadCoordsB[i];
            chunk.isDirt = false;
            Vector3 pos = GridToWorld(chunkSize, toLoadCoordsA[i]); // coord * IVector3(chunkSize);
            pos.z = -pos.z;

            trans.Position(pos);
            loadedChunks[toLoadCoordsA[i]] = _chunk;
        }
        return;
    }

    int loadOffset = 0;

    lastCoord = currentCoord;
    currentCoord = WorldToGrid(chunkSize, camPos);
    //LogInfo("Current Coord x: %d z: %d", currentCoord.x, currentCoord.z);
    
    if(currentCoord != lastCoord || loadedChunks.size() <= 0){
        toLoadCoords.clear();
        toLoadCoordsA.clear();
        toLoadCoordsB.clear();
        loadedIndexRange = {0, loadedRangeStep};

        for(int x = -(chunkLoadDistance+loadOffset); x <= (chunkLoadDistance+loadOffset); x++){
            for(int z = -(chunkLoadDistance+loadOffset); z <= (chunkLoadDistance+loadOffset); z++){
                IVector3 _c = currentCoord + IVector3(x, 0, z);
                //LogInfo("To Load Coord x: %d z: %d", _c.x, _c.z);
                toLoadCoords[_c] = true;
                if(loadedChunks.count(_c) > 0) continue;

                toLoadCoordsA.push_back(_c);
                toLoadCoordsB.push_back({
                    CreateRef<ChunkData>(_c, _c*chunkSize, chunkSize, chunkSize*chunkHeigtCount),
                    CreateRef<Mesh>(),
                    CreateRef<Mesh>(),
                    CreateRef<Mesh>(),
                    CreateRef<Mesh>(),
                });
            }
        }

        std::vector<IVector3> toRemove;
        for(auto i: loadedChunks){
            if(toLoadCoords.count(i.first) == false){
                UnLoadChunk(IVector3(i.first.x, i.first.y, i.first.z));
                toRemove.push_back(IVector3(i.first.x, i.first.y, i.first.z));
            }
        }
        for(auto i: toRemove){
            loadedChunks.erase(i);
            loadedChunksB.erase(i);
        }

        // ------------------ Deffered Load By ThreadPool ----------------------
        {
        //SimpleTimer timer([](float duration){ LogInfo("WorldManagerSystem::HandleLoadUnload::5 %.3f.ms", duration); });
        for(int i = 0; i < toLoadCoordsA.size(); i++){
            threadPool->enqueue([&, i](){
                chunkBuilderLayer->BuildData(toLoadCoordsA[i], toLoadCoordsB[i]); 
                //*toLoadCoordsB[i].done = true; 
            });
            loadingChunkDataJobs += 1;
        }
        }
        /*{
        //SimpleTimer timer([](float duration){ LogInfo("WorldManagerSystem::HandleLoadUnload::5 %.3f.ms", duration); });
        loadingChunkDataJobs = toLoadCoordsA.size();
        threadPool->enqueue([&](){
            for(int i = 0; i < toLoadCoordsA.size(); i++){
                chunkBuilderLayer->BuildData(toLoadCoordsA[i], toLoadCoordsB[i]); 
            }
        });
        }*/
    }
}

void WorldManagerSystem::HandleLoadUnload2(){
    OD_PROFILE_SCOPE("WorldManagerSystem::HandleLoadUnload");
    using namespace std::chrono_literals;

    lastCoord = currentCoord;
    currentCoord = WorldToGrid(chunkSize, camPos);
    if(currentCoord != lastCoord || loadedChunks.size() <= 0){
        
        toLoadCoords.clear();
        toLoadCoordsA.clear();
        toLoadCoordsB.clear();
        loadedIndexRange = {0, loadedRangeStep};

        for(int x = -chunkLoadDistance; x <= chunkLoadDistance; x++){
            for(int z = -chunkLoadDistance; z <= chunkLoadDistance; z++){
                IVector3 _c = currentCoord + IVector3(x, 0, z);
                toLoadCoords[_c] = true;
                if(loadedChunks.count(_c) > 0) continue;

                toLoadCoordsA.push_back(_c);
                toLoadCoordsB.push_back({
                    CreateRef<ChunkData>(_c, _c*chunkSize, chunkSize, chunkSize*chunkHeigtCount),
                    CreateRef<Mesh>(),
                    CreateRef<Mesh>(),
                    CreateRef<Mesh>(),
                    CreateRef<Mesh>(),
                });
            }
        }

        std::vector<IVector3> toRemove;
        for(auto i: loadedChunks){
            if(toLoadCoords.count(i.first) == false){
                UnLoadChunk(IVector3(i.first.x, i.first.y, i.first.z));
                toRemove.push_back(IVector3(i.first.x, i.first.y, i.first.z));
            }
        }
        for(auto i: toRemove){
            loadedChunks.erase(i);
            loadedChunksB.erase(i);
        }

        //------------------ NoDeffered Load By JobSystem ----------------------
        /*JobSystem::Dispatch(toLoadCoordsA.size(), toLoadCoordsA.size()/4, [&](JobDispatchArgs args){
            chunkBuilderLayer->BuildData(toLoadCoordsA[args.jobIndex], toLoadCoordsB[args.jobIndex]); 
            chunkBuilderLayer->BuildMesh(toLoadCoordsB[args.jobIndex]); 
        });
        JobSystem::Wait();*/
        for(int i = 0; i < toLoadCoordsA.size(); i++){
            chunkBuilderLayer->BuildData(toLoadCoordsA[i], toLoadCoordsB[i]);  
            loadedChunksB[toLoadCoordsA[i]].chunkData = toLoadCoordsB[i].chunkData;
        }
        for(int i = 0; i < toLoadCoordsB.size(); i++){
            chunkBuilderLayer->BuildMesh2(toLoadCoordsB[i]);
        }
        for(int i = 0; i < toLoadCoordsA.size(); i++){
            Entity _chunk = scene->AddEntity("Chunk");
            ChunkComponent& chunk = _chunk.AddComponent<ChunkComponent>();
            TransformComponent& trans = _chunk.GetComponent<TransformComponent>();
            
            chunk.opaqueMesh = BuildChunkMeshEntity(_chunk, toLoadCoordsB[i].opaqueMesh, chunkBuilderLayer->materialOpaque);
            chunk.waterMesh = BuildChunkMeshEntity(_chunk, toLoadCoordsB[i].waterMesh, chunkBuilderLayer->materialWater);

            chunk.data = toLoadCoordsB[i];
            chunk.isDirt = false;
            Vector3 pos = GridToWorld(chunkSize, toLoadCoordsA[i]); // coord * IVector3(chunkSize);
            pos.z = -pos.z;

            trans.Position(pos);
            loadedChunks[toLoadCoordsA[i]] = _chunk;
        }
    }
}

void WorldManagerSystem::HandleLoadUnload3(){
    OD_PROFILE_SCOPE("WorldManagerSystem::HandleLoadUnload");
    using namespace std::chrono_literals;

    if(loadingJobs.size() > 0){
        if(loadingJobsDone == false) return;
        for(auto& i: loadingJobs) i.join();

        for(int i = 0; i < toLoadCoordsA.size(); i++){
            Entity _chunk = scene->AddEntity("Chunk");
            ChunkComponent& chunk = _chunk.AddComponent<ChunkComponent>();
            TransformComponent& trans = _chunk.GetComponent<TransformComponent>();
            
            chunk.opaqueMesh = BuildChunkMeshEntity(_chunk, toLoadCoordsB[i].opaqueMesh, chunkBuilderLayer->materialOpaque);
            chunk.waterMesh = BuildChunkMeshEntity(_chunk, toLoadCoordsB[i].waterMesh, chunkBuilderLayer->materialWater);

            chunk.data = toLoadCoordsB[i];
            chunk.isDirt = false;
            Vector3 pos = GridToWorld(chunkSize, toLoadCoordsA[i]); // coord * IVector3(chunkSize);
            pos.z = -pos.z;

            trans.Position(pos);
            loadedChunks[toLoadCoordsA[i]] = _chunk;
        }
        loadingJobs.clear();
        return;
    }

    lastCoord = currentCoord;
    currentCoord = WorldToGrid(chunkSize, camPos);
    if(currentCoord != lastCoord || loadedChunks.size() <= 0){
        
        toLoadCoords.clear();
        toLoadCoordsA.clear();
        toLoadCoordsB.clear();
        loadedIndexRange = {0, loadedRangeStep};

        for(int x = -(chunkLoadDistance+2); x <= (chunkLoadDistance+2); x++){
            for(int z = -(chunkLoadDistance+2); z <= (chunkLoadDistance+2); z++){
                IVector3 _c = currentCoord + IVector3(x, 0, z);
                toLoadCoords[_c] = true;
                if(loadedChunks.count(_c) > 0) continue;

                toLoadCoordsA.push_back(_c);
                toLoadCoordsB.push_back({
                    CreateRef<ChunkData>(_c, _c*chunkSize, chunkSize, chunkSize*chunkHeigtCount),
                    CreateRef<Mesh>(),
                    CreateRef<Mesh>(),
                    CreateRef<Mesh>(),
                    CreateRef<Mesh>(),
                });
            }
        }

        std::vector<IVector3> toRemove;
        for(auto i: loadedChunks){
            if(toLoadCoords.count(i.first) == false){
                UnLoadChunk(IVector3(i.first.x, i.first.y, i.first.z));
                toRemove.push_back(IVector3(i.first.x, i.first.y, i.first.z));
            }
        }
        for(auto i: toRemove){
            loadedChunks.erase(i);
            loadedChunksB.erase(i);
        }

        loadingJobsDone = false;
        loadingJobs.push_back(std::thread([&](){
            for(int i = 0; i < toLoadCoordsA.size(); i++){
                chunkBuilderLayer->BuildData(toLoadCoordsA[i], toLoadCoordsB[i]);  
                loadedChunksB[toLoadCoordsA[i]].chunkData = toLoadCoordsB[i].chunkData;
            }
            for(int i = 0; i < toLoadCoordsB.size(); i++){
                chunkBuilderLayer->BuildMesh2(toLoadCoordsB[i]);
            }
            loadingJobsDone = true;
        }));
    }
}