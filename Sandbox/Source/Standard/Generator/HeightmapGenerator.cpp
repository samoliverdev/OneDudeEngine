#include "HeightmapGenerator.h"
#include "Standard/Ultis/FastNoiseLiteCpp.h"
#include "Standard/Ultis/Ultis.h"
#include <OD/Terrain/Terrain.h>
#include <OD/Graphics/Texture.h>
#include <OD/Core/ImGui.h>
#include <taskflow/taskflow.hpp>

namespace Standard{

void HeightmapGenerator::OnGui(Entity& e, Scene& scene){
    // Helper function to check if two floats are approximately equal
    auto approxEqual = [](float a, float b, float epsilon = 0.001f) {
        return std::abs(a - b) < epsilon;
    };

    // Test 1: Empty curve
    {
        AnimationCurve curve;
        assert(approxEqual(curve.Evaluate(0.0f), 0.0f));
        assert(approxEqual(curve.Evaluate(0.5f), 0.0f));
        assert(approxEqual(curve.Evaluate(1.0f), 0.0f));
    }

    // Test 2: Single keyframe
    {
        AnimationCurve curve;
        curve.AddKeyframe(0.5f, 1.0f, CurveType::Linear);
        assert(approxEqual(curve.Evaluate(0.0f), 1.0f));
        assert(approxEqual(curve.Evaluate(0.5f), 1.0f));
        assert(approxEqual(curve.Evaluate(1.0f), 1.0f));
    }

    // Test 3: Linear interpolation
    {
        AnimationCurve curve;
        curve.AddKeyframe(0.0f, 0.0f, CurveType::Linear);
        curve.AddKeyframe(1.0f, 1.0f, CurveType::Linear);
        assert(approxEqual(curve.Evaluate(0.0f), 0.0f));
        assert(approxEqual(curve.Evaluate(0.5f), 0.5f));
        assert(approxEqual(curve.Evaluate(1.0f), 1.0f));
        assert(approxEqual(curve.Evaluate(0.25f), 0.25f));
    }

    // Test 4: Constant interpolation
    {
        AnimationCurve curve;
        curve.AddKeyframe(0.0f, 1.0f, CurveType::Constant);
        curve.AddKeyframe(1.0f, 2.0f, CurveType::Linear);
        assert(approxEqual(curve.Evaluate(0.0f), 1.0f));
        assert(approxEqual(curve.Evaluate(0.5f), 1.0f));
        assert(approxEqual(curve.Evaluate(0.999f), 1.0f));
        assert(approxEqual(curve.Evaluate(1.0f), 2.0f));
    }

    // Test 5: Smooth interpolation (Bezier with tangents)
    {
        AnimationCurve curve;
        Keyframe k0(0.0f, 0.0f, CurveType::Smooth);
        k0.out_tangent = ImVec2(10.0f, 10.0f); // Scaled to 0.1, -0.2 (y inverted, value_range=2.0)
        curve.keyframes.push_back(k0);
        Keyframe k1(1.0f, 1.0f, CurveType::Linear);
        k1.in_tangent = ImVec2(-10.0f, -10.0f); // Scaled to -0.1, 0.2 (y inverted)
        curve.keyframes.push_back(k1);
        // Expected: Bezier curve, approximate midpoint value
        float mid_value = curve.Evaluate(0.5f);
        assert(mid_value > 0.4f && mid_value < 0.6f); // Rough check for Bezier curve shape
        assert(approxEqual(curve.Evaluate(0.0f), 0.0f));
        assert(approxEqual(curve.Evaluate(1.0f), 1.0f));
    }

    // Test 6: Out-of-range time
    {
        AnimationCurve curve;
        curve.AddKeyframe(0.2f, 0.5f, CurveType::Linear);
        curve.AddKeyframe(0.8f, 1.5f, CurveType::Linear);
        assert(approxEqual(curve.Evaluate(-1.0f), 0.5f)); // Before first keyframe
        assert(approxEqual(curve.Evaluate(2.0f), 1.5f));  // After last keyframe
    }


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

    //DrawAnimationCurveEditor(heightmapGenerator.curve, {200, 100});

    static bool show_curve_editor;
    DrawCurvePreview(heightmapGenerator.curve, ImVec2(100, 50), &show_curve_editor);

    // Curve editor window
    if (show_curve_editor) {
        ImGui::Begin("Curve Editor", &show_curve_editor, ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_NoDocking);
        DrawAnimationCurveEditor(heightmapGenerator.curve, ImVec2(400, 200));
        ImGui::End();
    }

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
            noise = curve.Evaluate(noise);
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

Ref<Heightmap> HeightmapGenerator::GenerateHeightmapFast(int seed){
    FastNoiseLite noise2;
    noise2.SetSeed(seed);
    noise2.SetNoiseType(FastNoiseLite::NoiseType_OpenSimplex2);
    noise2.SetFractalType(FastNoiseLite::FractalType_FBm);
    noise2.SetFractalOctaves(octaves);
    noise2.SetFractalLacunarity(lacunarity);
    noise2.SetFractalGain(persistance);

    Ref<Heightmap> noiseMap = CreateRef<Heightmap>(width, height);

    if(scale <= 0) scale = 0.0001f;

    float halfWidth  = width  / 2.0f;
    float halfHeight = height / 2.0f;

    // --- Taskflow setup ---
    tf::Taskflow taskflow;
    tf::Executor executor;

    unsigned num_threads = std::thread::hardware_concurrency();
    if(num_threads == 0) num_threads = 4; // fallback

    // Divide rows into chunks
    int rows_per_task = (height + num_threads - 1) / num_threads;

    for(unsigned t = 0; t < num_threads; t++) {
        int y_start = t * rows_per_task;
        int y_end   = std::min<int>(y_start + rows_per_task, height);

        if(y_start >= y_end) continue;

        taskflow.emplace([=, &noise2, &noiseMap]() {
            for(int y = y_start; y < y_end; y++) {
                for(int x = 0; x < width; x++) {
                    float _x = x / (float)width * 2 - 1;
                    float _y = y / (float)height * 2 - 1;
                    float _falloff = math::max(math::abs(_x), math::abs(_y));
                    float a = 3;
                    float b = 2.2f;
                    _falloff = 1 - math::pow(_falloff, a) / 
                               (math::pow(_falloff, a) + math::pow(b - b * _falloff, a));

                    float noise = noise2.GetNoise((x+offset.x)*scale, (y+offset.y)*scale);
                    if(to01) noise = noise * 0.5f + 0.5f;
                    if(useCurver) noise = curve.Evaluate(noise);
                    noise = math::pow(noise, power);
                    noise = math::pow(noise, power2);

                    if(falloff) {
                        noise = noise * _falloff;
                    }

                    noiseMap->Set(x, y, noise);
                }
            }
        });
    }

    executor.run(taskflow).wait();

    return noiseMap;
}

Ref<Heightmap> HeightmapGeneratorAdvanced::GenerateHeightmap(int seed, int genOnlyLayer){
    Ref<Heightmap> noiseMap = CreateRef<Heightmap>(width, height);

    FastNoiseLite layer0;
    layer0.SetSeed(seed);
    layer0.SetNoiseType(FastNoiseLite::NoiseType_OpenSimplex2);
    layer0.SetFractalType(FastNoiseLite::FractalType_FBm);
    layer0.SetFractalOctaves(erosion.octaves);
    layer0.SetFractalLacunarity(erosion.lacunarity);
    layer0.SetFractalGain(erosion.persistance);
    if(erosion.scale <= 0) erosion.scale = 0.0001f;

    FastNoiseLite layer1;
    layer1.SetSeed(seed+1);
    layer1.SetNoiseType(FastNoiseLite::NoiseType_OpenSimplex2);
    layer1.SetFractalType(FastNoiseLite::FractalType_FBm);
    layer1.SetFractalOctaves(continentalness.octaves);
    layer1.SetFractalLacunarity(continentalness.lacunarity);
    layer1.SetFractalGain(continentalness.persistance);
    if(continentalness.scale <= 0) continentalness.scale = 0.0001f;

    FastNoiseLite layer2;
    layer2.SetSeed(seed+2);
    layer2.SetNoiseType(FastNoiseLite::NoiseType_OpenSimplex2);
    layer2.SetFractalType(FastNoiseLite::FractalType_FBm);
    layer2.SetFractalOctaves(peaksValleys.octaves);
    layer2.SetFractalLacunarity(peaksValleys.lacunarity);
    layer2.SetFractalGain(peaksValleys.persistance);
    if(peaksValleys.scale <= 0) peaksValleys.scale = 0.0001f;

    float halfWidth  = width  / 2.0f;
    float halfHeight = height / 2.0f;

    // --- Taskflow setup ---
    tf::Taskflow taskflow;
    tf::Executor executor;

    unsigned num_threads = std::thread::hardware_concurrency();
    if(num_threads == 0) num_threads = 4; // fallback

    // Divide rows into chunks
    int rows_per_task = (height + num_threads - 1) / num_threads;

    for(unsigned t = 0; t < num_threads; t++) {
        int y_start = t * rows_per_task;
        int y_end   = std::min<int>(y_start + rows_per_task, height);

        if(y_start >= y_end) continue;

        taskflow.emplace([=, &layer0, &layer1, &layer2, &noiseMap]() {
            for(int y = y_start; y < y_end; y++) {
                for(int x = 0; x < width; x++) {
                    /*float _x = x / (float)width * 2 - 1;
                    float _y = y / (float)height * 2 - 1;
                    float _falloff = math::max(math::abs(_x), math::abs(_y));
                    float a = 3;
                    float b = 2.2f;
                    _falloff = 1 - math::pow(_falloff, a) / (math::pow(_falloff, a) + math::pow(b - b * _falloff, a));*/

                    float nx = (x / (float)width ) * 2 - 1;   // [-1,1]
                    float ny = (y / (float)height) * 2 - 1;   // [-1,1]
                    float dist = sqrt(nx * nx + ny * ny);     // radial distance
                    dist = math::clamp(dist, 0.0f, 1.0f);     // keep inside [0,1]
                    float a = 3.0f;
                    float b = 2.2f;
                    float _falloff = 1.0f - pow(dist, a) / (pow(dist, a) + pow(b - b * dist, a));

                    float noise0 = layer0.GetNoise((x+erosion.offset.x)*erosion.scale, (y+erosion.offset.y)*erosion.scale);
                    if(erosion.to01) noise0 = noise0 * 0.5f + 0.5f;
                    if(erosion.useCurver) noise0 = erosion.curve.Evaluate(noise0);
                    noise0 = math::pow(noise0, erosion.power);
                    noise0 = math::pow(noise0, erosion.power2);

                    float noise1 = layer1.GetNoise((x+continentalness.offset.x)*continentalness.scale, (y+continentalness.offset.y)*continentalness.scale);
                    if(continentalness.to01) noise1 = noise1 * 0.5f + 0.5f;
                    if(continentalness.useCurver) noise1 = continentalness.curve.Evaluate(noise1);
                    noise1 = math::pow(noise1, continentalness.power);
                    noise1 = math::pow(noise1, continentalness.power2);

                    float noise2 = layer2.GetNoise((x+peaksValleys.offset.x)*peaksValleys.scale, (y+peaksValleys.offset.y)*peaksValleys.scale);
                    if(peaksValleys.to01) noise2 = noise2 * 0.5f + 0.5f;
                    if(peaksValleys.useCurver) noise2 = peaksValleys.curve.Evaluate(noise2);
                    noise2 = math::pow(noise2, peaksValleys.power);
                    noise2 = math::pow(noise2, peaksValleys.power2);
                    
                    float finalNoise = noise1;

                    // Remap from one range to another
                    auto Remap = [](float v, float minIn, float maxIn, float minOut, float maxOut){
                        return minOut + (v - minIn) * (maxOut - minOut) / (maxIn - minIn);
                    };

                    // Smooth step curve
                    auto SmoothStep = [](float edge0, float edge1, float x) {
                        float t = std::clamp((x - edge0) / (edge1 - edge0), 0.0f, 1.0f);
                        return t * t * (3.0f - 2.0f * t);
                    };

                    /*float cont = Remap(noise1, 0, 1, -1, 1);   // continentalness
                    float ero  = Remap(noise0, 0, 1, 0, 1);     // erosion
                    float pv   = Remap(noise2, 0, 1, -1, 1);     // peaks & valleys

                    // --- Step 1: Base terrain from continentalness ---
                    float oceanLevel      = 0.2f; //0.25f;  // sea level (0–1 space)
                    float continentHeight = 0.5f; //0.55f;  // average land height
                    float baseHeight = math::mix(oceanLevel, continentHeight, cont * 0.5f + 0.5f);

                    // --- Step 2: Peaks & valleys modulation ---
                    float mountainFactor = SmoothStep(0.0f, 0.4f, cont);  //SmoothStep(0.2f, 0.8f, cont); // mountains appear inland
                    float maxMountainHeight = 1.0f; //0.35f; // how tall mountains can be (in [0–1])
                    float mountainHeight = pv * mountainFactor * maxMountainHeight;

                    // --- Step 3: Apply erosion ---
                    float ruggedness = mountainHeight * (1.0f - ero);

                    // --- Step 4: Combine ---
                    float height = baseHeight + ruggedness;
                    height = math::mix<float>(height, baseHeight * 0.9f, ero);

                    // --- Step 5: Clamp/normalize ---
                    finalNoise = std::clamp(height, 0.0f, 1.0f);*/

                    //finalNoise = math::mix(noise1, noise0, noise2 * noise0);
                    
                    /*float cont = Remap(noise1, 0, 1, -1, 1);   // continentalness
                    float ero  = Remap(noise0, 0, 1, 0, 1);     // erosion
                    float pv   = Remap(noise2, 0, 1, -1, 1);     // peaks & valleys

                    finalNoise = cont * (1 - ero);
                    //finalNoise = finalNoise + math::min<float>(pv * 1.0f, 0);
                    finalNoise = finalNoise + ((pv * 0.35f) * (1 - ero));
                    finalNoise = Remap(finalNoise, -1, 1, 0, 1);*/

                    //finalNoise = noise1;

                    /*float cont = Remap(noise1, 0, 1, 0, 1);   // continentalness
                    float ero  = Remap(noise0, 0, 1, 0, 1);     // erosion
                    float pv   = Remap(noise2, 0, 1, 0, 1);     // peaks & valleys
                    finalNoise = (cont * 0.6f) + ((ero * 0.4f));*/
                    //finalNoise = Remap(finalNoise, -1, 1, 0, 1);

                    auto peaksAndValleys = [](float weirdness){
                        return weirdness;
                        return -(math::abs(math::abs(weirdness) - 0.6666667f) - 0.33333334f) * 3.0f;
                    };
                    auto peaksAndValleys2 = [&](float weirdness){
                        weirdness = Remap(weirdness, 0, 1, -1, 1);
                        float value =  1.0f - math::abs((3.0f * math::abs(weirdness)) - 2.0f);
                        return Remap(value, -1, 1, 0, 1);
                    };

                    float continentalness = Remap(noise1, 0, 1, -1, 1); 
                    float erosion = Remap(noise0, 0, 1, 0, 1);
                    float weirdness = Remap(noise2, 0, 1, 0, 1);  

                    float base = continentalness * 0.4f;
                    float ridges = peaksAndValleys2(weirdness);

                    float mountains = base + ridges * (1.0f - erosion);
                    float height = math::mix(mountains, base, erosion);
                    finalNoise = math::clamp<float>(Remap(height, -1, 1, 0, 1), 0, 1);

                    if(genOnlyLayer == 0) finalNoise = noise0;
                    if(genOnlyLayer == 1) finalNoise = noise1;
                    if(genOnlyLayer == 2) finalNoise = ridges;

                    if(falloff) {
                        finalNoise = finalNoise * _falloff;
                    }

                    noiseMap->Set(x, y, finalNoise);
                }
            }
        });
    }

    executor.run(taskflow).wait();

    return noiseMap;
}

}