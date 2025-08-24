#include "HeightmapGenerator.h"
#include "Standard/Ultis/FastNoiseLiteCpp.h"
#include "Standard/Ultis/Ultis.h"
#include "OD/Terrain/Terrain.h"
#include "OD/Core/ImGui.h"

namespace Standard{

void HeightmapGenerator::OnGui(Entity& e, Scene& scene){
    HeightmapGenerator& heightmapGenerator = scene.GetComponent<HeightmapGenerator>(e);

    ImGui::DragInt("width", &heightmapGenerator.width);
    ImGui::DragInt("height", &heightmapGenerator.height);
    ImGui::DragFloat("scale", &heightmapGenerator.scale);
    ImGui::DragInt("octaves", &heightmapGenerator.octaves);
    ImGui::DragFloat("persistance", &heightmapGenerator.persistance);
    ImGui::DragFloat("lacunarity", &heightmapGenerator.lacunarity);
    ImGui::DragFloat2("offset", &heightmapGenerator.offset.x);
    ImGui::DragFloat("power", &heightmapGenerator.power);
    ImGui::DragFloat("power2", &heightmapGenerator.power2);
    ImGui::Checkbox("to01", &heightmapGenerator.to01);
    ImGui::Checkbox("falloff", &heightmapGenerator.falloff);

    if(scene.HasComponent<TerrainComponent>(e)){
        TerrainComponent& terrain = scene.GetComponent<TerrainComponent>(e);

        static int seed = 0;
        ImGui::DragInt("Seed", &seed);

        if(ImGui::Button("Generate Heightmap")){
            Ref<Heightmap> heightmap = heightmapGenerator.GenerateHeightmap(seed);
            terrain.SetHeightmap(heightmap);
        }
    }
}

Ref<Heightmap> HeightmapGenerator::GenerateHeightmap(int seed){
    FastNoiseLite noise2;
    noise2.SetSeed(seed);
    noise2.SetNoiseType(FastNoiseLite::NoiseType_OpenSimplex2);
    noise2.SetFractalType(FastNoiseLite::FractalType_FBm);
    noise2.SetFractalOctaves(octaves);
    noise2.SetFractalLacunarity(lacunarity);
    noise2.SetFractalGain(persistance);

    /*Random2 prng(seed);
    std::vector<Vector2> octaveOffsets(octaves); 
    for(int i = 0; i < octaves; i++){
        float offsetX = (float)prng.Range(-100000, 100000);
        float offsetY = (float)prng.Range(-100000, 100000);
        octaveOffsets[i] = Vector2(offsetX + offset.x, offsetY + offset.y);
    }*/

    Ref<Heightmap> noiseMap = CreateRef<Heightmap>(width, height);

    if(scale <= 0) scale = 0.0001f;

    float maxNoiseHeight = FLT_MIN;
    float minNoiseHeight = FLT_MAX;

    float halfWidth = width / 2.0f;
    float halfHeight = height / 2.0f;

    for(int y = 0; y < height; y++){
        for(int x = 0; x < width; x++){
            float _x = x / (float)width * 2 - 1;
			float _y = y / (float)height * 2 - 1;
            float _falloff = math::max(math::abs(_x), math::abs(_y));
            float a = 3;
		    float b = 2.2f;
		    _falloff = 1 - math::pow(_falloff, a) / (math::pow(_falloff, a) + math::pow(b - b * _falloff, a));

            float noise = noise2.GetNoise((x+offset.x)*scale, (y+offset.y)*scale);// * 0.5f + 0.5f;
            if(to01) noise = noise * 0.5f + 0.5f;
            noise = math::pow(noise, power);
            noise = math::pow(noise, power2);

            if(falloff){
                noise = noise * _falloff;
            }

            noiseMap->Set(x, y, noise);
        }
    }

    for(int y = 0; y < height; y++){
        for(int x = 0; x < width; x++){
            //noiseMap->Set(x, y, InverseLerp(minNoiseHeight, maxNoiseHeight, noiseMap->Get(x, y)));
            //noiseMap->Set(x, y, Remap(noiseMap->Get(x, y), Vector2(-1,1), Vector2(0,1)));
        }
    }

    return noiseMap;
}

}