#pragma once
#include <OD/OD.h>
//#include "Ultis/FastNoiseLite.h"
#include "Ultis/FastNoiseLiteCpp.h"
#include "Ultis/Ultis.h"
#include <limits>
#include <float.h>

using namespace OD;

struct NoiseData{
    NoiseData(int inWidth, int inHeight){
        width = inWidth;
        height = inHeight;
        data.resize(width * height);
    }

    inline void Set(int x, int y, float value){
        data[y * width + x] = value;  
        //data[x * width + y] = value;  
    }

    inline float Get(int x, int y){
        return data[y * width + x];  
        //return data[x * width + y];  
    }

    inline void Invert(){
        std::vector<float> newData;

        newData.resize(width*height);
        int _size = width;
        for(int i = 0, j = data.size() - _size; i < data.size(); i += _size, j -= _size){
            for(int k = 0; k < _size; ++k) {
                newData[i + k] = data[j + k];
            }
        }

        data = newData;
    }

    inline void TransposeTo(std::vector<float>& newData){
        newData.resize(width*height);
        int _size = width;
        for(int i = 0, j = data.size() - _size; i < data.size(); i += _size, j -= _size){
            for(int k = 0; k < _size; ++k) {
                newData[i + k] = data[j + k];
            }
        }
    }

    template <class Archive>
    void serialize(Archive& ar){
        ArchiveDumpNVP(ar, data);
        ArchiveDumpNVP(ar, width);
        ArchiveDumpNVP(ar, height);
    }

    std::vector<float> data;
    int width;
    int height;
};

namespace Noise{

inline float Remap(float In, Vector2 InMinMax, Vector2 OutMinMax){
    return OutMinMax.x + (In - InMinMax.x) * (OutMinMax.y - OutMinMax.x) / (InMinMax.y - InMinMax.x);
}
    
inline float InverseLerp(float xx, float yy, float value){
    return (value - xx)/(yy - xx);
}

inline Ref<NoiseData> GenerateNoiseMap(int mapWidth, int mapHeight, int seed, float scale, int octaves, float persistance, float lacunarity, Vector2 offset){
    //auto noise = fnlCreateState();
    //noise.noise_type = FNL_NOISE_VALUE;

    FastNoiseLite noise2;
    noise2.SetSeed(seed);
    noise2.SetNoiseType(FastNoiseLite::NoiseType_OpenSimplex2);
    noise2.SetFractalType(FastNoiseLite::FractalType_FBm);
    noise2.SetFractalOctaves(octaves);
    noise2.SetFractalLacunarity(lacunarity);
    noise2.SetFractalGain(persistance);

    Random prng(seed);
    std::vector<Vector2> octaveOffsets(octaves); 
    for(int i = 0; i < octaves; i++){
        float offsetX = (float)prng.Range(-100000, 100000);
        float offsetY = (float)prng.Range(-100000, 100000);
        octaveOffsets[i] = Vector2(offsetX + offset.x, offsetY + offset.y);
    }

    Ref<NoiseData> noiseMap = CreateRef<NoiseData>(mapWidth, mapHeight);

    if(scale <= 0) scale = 0.0001f;

    float maxNoiseHeight = FLT_MIN;
    float minNoiseHeight = FLT_MAX;

    float halfWidth = mapWidth / 2.0f;
    float halfHeight = mapHeight / 2.0f;

    for(int y = 0; y < mapHeight; y++){
        for(int x = 0; x < mapWidth; x++){
            noiseMap->Set(x, y, noise2.GetNoise((x+offset.x)*scale, (y+offset.y)*scale) * 0.5f + 0.5f);

            /*float amplitude = 1;
            float frequency = 1;
            float noiseHeight = 0;

            for(int i = 0; i < octaves; i++){
                float sampleX = (x-halfWidth) / scale * frequency + octaveOffsets[i].x;
                float sampleY = (y-halfHeight) / scale * frequency + octaveOffsets[i].y;

                float perlinValue = fnlGetNoise2D(&noise, sampleX, sampleY) * 0.5f + 0.5f;
                noiseHeight += perlinValue * amplitude;

                amplitude *= frequency;
                frequency *= lacunarity;
            }

            if(noiseHeight > maxNoiseHeight){
                maxNoiseHeight = noiseHeight;
            } else if(noiseHeight < minNoiseHeight){
                minNoiseHeight = noiseHeight;
            }

            noiseMap->Set(x, y, noiseHeight);*/
        }
    }

    for(int y = 0; y < mapHeight; y++){
        for(int x = 0; x < mapWidth; x++){
            //noiseMap->Set(x, y, InverseLerp(minNoiseHeight, maxNoiseHeight, noiseMap->Get(x, y)));
            //noiseMap->Set(x, y, Remap(noiseMap->Get(x, y), Vector2(-1,1), Vector2(0,1)));
        }
    }

    return noiseMap;
}

};