#pragma once
#include "OD/Core/Math.h"
#include "OD/Scene/Scene.h"
#include "OD/Terrain/Heightmap.h"
#include "Standard/Ultis/AnimationCurve.h"

using namespace OD;

namespace OD{
    class Texture2D;
}

namespace Standard{

struct HeightmapGenerator{
    int width = (1024 * 2)+1;
    int height = (1024 * 2)+1; 
    float scale = 0.075f; //0.25f/(2*1); 
    int octaves = 8;
    float persistance = 0.25f; 
    float lacunarity = 2.5f;
    Vector2 offset = {0, 0};
    float power = 4;
    float power2 = 0.5f;
    bool to01 = true;
    bool falloff = false;

    bool useCurver = false;
    AnimationCurve curve = {
        {
            Keyframe(0.0f, 0.0f, CurveType::Linear),
            Keyframe(1.0f, 1.0f, CurveType::Linear)
        }
    };

    static void OnGui(Entity& e, Scene& scene);

    Ref<Heightmap> GenerateHeightmap(int seed);
    Ref<Heightmap> GenerateHeightmapFast(int seed);

    template <class Archive>
    void serialize(Archive& ar){
        ArchiveDumpNVP(ar, width);
        ArchiveDumpNVP(ar, height);
        ArchiveDumpNVP(ar, scale);
        ArchiveDumpNVP(ar, octaves);
        ArchiveDumpNVP(ar, persistance);
        ArchiveDumpNVP(ar, lacunarity);
        ArchiveDumpNVP(ar, offset);
        ArchiveDumpNVP(ar, power);
        ArchiveDumpNVP(ar, power2);
        ArchiveDumpNVP(ar, to01);
        ArchiveDumpNVP(ar, falloff);
        ArchiveDumpNVP(ar, useCurver);
        ArchiveDumpNVP(ar, curve);
    }
};

struct HeightmapGeneratorAdvanced{
    struct Layer{
        float scale = 0.075f; //0.25f/(2*1); 
        int octaves = 8;
        float persistance = 0.25f; 
        float lacunarity = 2.5f;
        Vector2 offset = {0, 0};
        float power = 4;
        float power2 = 0.5f;
        bool to01 = true;

        bool useCurver = false;
        AnimationCurve curve = {
            {
                Keyframe(0.0f, 0.0f, CurveType::Linear),
                Keyframe(1.0f, 1.0f, CurveType::Linear)
            }
        };

        template <class Archive>
        void serialize(Archive& ar){
            ArchiveDumpNVP(ar, scale);
            ArchiveDumpNVP(ar, octaves);
            ArchiveDumpNVP(ar, persistance);
            ArchiveDumpNVP(ar, lacunarity);
            ArchiveDumpNVP(ar, offset);
            ArchiveDumpNVP(ar, power);
            ArchiveDumpNVP(ar, power2);
            ArchiveDumpNVP(ar, to01);
            ArchiveDumpNVP(ar, useCurver);
            ArchiveDumpNVP(ar, curve);
        }
    };

    int width = (1024 * 2)+1;
    int height = (1024 * 2)+1; 
    Layer erosion;
    Layer continentalness;
    Layer peaksValleys;
    bool falloff = false;

    Ref<Heightmap> GenerateHeightmap(int seed, int genOnlyLayer = -1);

    template <class Archive>
    void serialize(Archive& ar){
        ArchiveDumpNVP(ar, width);
        ArchiveDumpNVP(ar, height);
        ArchiveDumpNVP(ar, erosion);
        ArchiveDumpNVP(ar, continentalness);
        ArchiveDumpNVP(ar, peaksValleys);
        ArchiveDumpNVP(ar, falloff);
    }
};

}