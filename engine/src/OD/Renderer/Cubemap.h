#pragma once

#include "OD/Defines.h"

namespace OD{

class Renderer;

class Cubemap{
    friend class Renderer;
public:
    bool IsValid();
    void Destroy();
    void Bind(int index);

    static Ref<Cubemap> CreateFromFile(
        const char* right,
        const char* left,
        const char* top,
        const char* bottom,
        const char* front,
        const char* back
    ); 

private:
    unsigned int _id;
};

}