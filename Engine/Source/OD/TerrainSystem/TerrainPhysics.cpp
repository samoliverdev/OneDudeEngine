#include "OD/pch.h"
#include "TerrainPhysics.h"
#include "TerrainComponents.h"
#include "OD/Physics/PhysicsSystem.h"

namespace OD{
struct TerrainCollisionSystem::State{
    struct Entry{
        std::vector<Entity> tiles;
        std::vector<bool> dirty;
        int x = 0, z = 0, samples = 0, layer = 0;
        Vector3 size = Vector3(0);
        Matrix4 root = Matrix4(1);
        uint64_t revision = 0, request = 0;
        WeakRef<Heightmap> source;
    };

    std::unordered_map<Entity, Entry> entries;
    bool initializing = false;

    static void Destroy(Scene& scene, Entry& entry){
        for(auto tile : entry.tiles)
            if(scene.IsValid(tile))
                scene.DestroyEntityImmediate(tile);
        entry.tiles.clear();
        entry.dirty.clear();
    }
};

TerrainCollisionSystem::TerrainCollisionSystem() : state(std::make_unique<State>()){
    name = "TerrainCollisionSystem";
}

TerrainCollisionSystem::~TerrainCollisionSystem() = default;

void TerrainCollisionSystem::OnStart(Scene& scene){
    state->initializing = true;
    PrePhysicsUpdate(scene);
    state->initializing = false;
}

void TerrainCollisionSystem::PrePhysicsUpdate(Scene& scene){
    // No physics helper entities in the edit scene, including during scene cloning.
    if(!scene.Running())
        return;
    auto& cache = state->entries;
    for(auto it = cache.begin(); it != cache.end();){
        auto* data = scene.GetRegistry().try_get<TerrainDataComponent>(it->first);
        auto* collision = scene.GetRegistry().try_get<TerrainCollisionComponent>(it->first);
        auto* info = scene.GetRegistry().try_get<InfoComponent>(it->first);
        if(!data || !collision || !collision->enabled || !info || !info->enable || !data->View().Valid() ||
           scene.GetRegistry().any_of<SelfDisable>(it->first)){
            State::Destroy(scene, it->second);
            it = cache.erase(it);
        } else
            ++it;
    }
    auto view =
        scene.GetRegistry().view<TerrainDataComponent, TerrainCollisionComponent, TransformComponent, InfoComponent>(
            entt::exclude<SelfDisable>);
    for(auto e : view){
        auto& data = view.get<TerrainDataComponent>(e);
        auto& c = view.get<TerrainCollisionComponent>(e);
        auto& info = view.get<InfoComponent>(e);
        if(!info.enable || !c.enabled || !data.View().Valid())
            continue;
        c.Validate();
        auto& entry = cache[e];
        Matrix4 root = view.get<TransformComponent>(e).GlobalModelMatrix();
        if(entry.root != root)
            std::fill(entry.dirty.begin(), entry.dirty.end(), true);
        entry.root = root;
        bool layout = entry.x != c.tilesX || entry.z != c.tilesZ || entry.samples != c.samplesPerTile ||
                      entry.size != data.size || entry.layer != info.layer;
        if(layout){
            State::Destroy(scene, entry);
            entry.x = c.tilesX;
            entry.z = c.tilesZ;
            entry.samples = c.samplesPerTile;
            entry.size = data.size;
            entry.layer = info.layer;
            entry.tiles.assign(size_t(c.tilesX) * c.tilesZ, EntityNull);
            entry.dirty.assign(entry.tiles.size(), true);
        }
        bool manual = entry.request != c.rebuild;
        if(manual || (c.autoRebuild && (entry.revision != data.Revision() || entry.source.lock() != data.heightmap))){
            auto region = data.ChangesSince(entry.revision, false);
            bool full = manual || entry.source.lock() != data.heightmap;
            auto heights = data.View();
            float x0 = float(region.x0) / (std::max)(1, heights.columns - 1),
                  x1 = float(region.x1) / (std::max)(1, heights.columns - 1);
            float z0 = float(region.z0) / (std::max)(1, heights.rows - 1),
                  z1 = float(region.z1) / (std::max)(1, heights.rows - 1);
            for(int z = 0; z < c.tilesZ; ++z)
                for(int x = 0; x < c.tilesX; ++x){
                    if(full || (float(x + 1) / c.tilesX >= x0 && float(x) / c.tilesX <= x1 &&
                                float(z + 1) / c.tilesZ >= z0 && float(z) / c.tilesZ <= z1))
                        entry.dirty[size_t(z) * c.tilesX + x] = true;
                }
            entry.request = c.rebuild;
            entry.revision = data.Revision();
            entry.source = data.heightmap;
        }
        int budget = state->initializing && c.buildAllOnStart ? int(entry.tiles.size()) : c.rebuildsPerFrame;
        for(size_t i = 0; i < entry.tiles.size() && budget > 0; ++i){
            if(!entry.dirty[i] && scene.IsValid(entry.tiles[i]))
                continue;
            --budget;
            int x = int(i) % c.tilesX, z = int(i) / c.tilesX, n = c.samplesPerTile;
            float w = data.size.x / c.tilesX, l = data.size.z / c.tilesZ;
            std::vector<float> samples(size_t(n) * n);
            auto field = data.View();
            for(int rz = 0; rz < n; ++rz)
                for(int rx = 0; rx < n; ++rx)
                    samples[size_t(rz) * n + rx] =
                        field.Sample((x + float(rx) / (n - 1)) * w, (z + float(rz) / (n - 1)) * l) / data.size.y;
            if(scene.IsValid(entry.tiles[i]))
                scene.DestroyEntityImmediate(entry.tiles[i]);
            Entity tile = scene.AddEntity("Terrain collision " + std::to_string(x) + "," + std::to_string(z));
            scene.AddTagComponent<DontSave>(tile);
            scene.AddTagComponent<HideInEditor>(tile);
            scene.SetParent(e, tile);
            scene.GetComponent<InfoComponent>(tile).layer = info.layer;
            auto& transform = scene.GetComponent<TransformComponent>(tile);
            transform.LocalPosition(Vector3(x * w, 0, z * l));
            transform.LocalScale(Vector3(w / (n - 1), data.size.y, l / (n - 1)));
            auto& collider = scene.AddComponent<HeightmapColliderComponent>(tile);
            collider.width = float(n);
            collider.length = float(n);
            collider.scale = 1;
            collider.minHeight = 0;
            collider.maxHeight = 1;
            collider.offset = Vector3(0);
            collider.heights = std::move(samples);
            entry.tiles[i] = tile;
            entry.dirty[i] = false;
        }
    }
}

void TerrainCollisionSystem::OnStop(Scene& scene){
    for(auto& pair : state->entries)
        State::Destroy(scene, pair.second);
    state->entries.clear();
}

void TerrainCollisionSystem::OnEnd(Scene& scene){
    // Scene destruction clears the registry before system OnEnd; validity checks are essential.
    OnStop(scene);
}
}
