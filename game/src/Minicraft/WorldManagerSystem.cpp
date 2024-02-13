#include "WorldManagerSystem.h"
#include "ChunkComponent.h"

WorldManagerSystem::WorldManagerSystem(Scene* inScene):System(inScene){

}

void WorldManagerSystem::Update(){
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