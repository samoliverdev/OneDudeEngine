#include "HeightmapGenerator.h"
#include "Standard/Ultis/FastNoiseLiteCpp.h"
#include "Standard/Ultis/Ultis.h"
#include "OD/Terrain/Terrain.h"
#include "OD/Core/ImGui.h"
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
        ImGui::Begin("Curve Editor", &show_curve_editor, ImGuiWindowFlags_AlwaysAutoResize);
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
                    noise = curve.Evaluate(noise);
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

}