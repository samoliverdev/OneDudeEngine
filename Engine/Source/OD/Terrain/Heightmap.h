#pragma once
#include "OD/Defines.h"
#include <vector>

namespace OD{

struct OD_API Heightmap{
    Heightmap(){
        width = 128;
        height = 128;
    }

    Heightmap(int inWidth, int inHeight){
        width = inWidth;
        height = inHeight;
        data.resize(width * height);
    }

    inline int ToFlatCoord(int x, int y){
        return y * width + x;
    }

    inline void Set(int x, int y, float value){
        data[y * width + x] = value;  
        //data[x * width + y] = value;  
    }

    inline float Get(int x, int y){
        return data[y * width + x];  
        //return data[x * width + y];  
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

    inline void Transpose(){
        std::vector<float> newData;
        TransposeTo(newData);
        data = newData;
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

}