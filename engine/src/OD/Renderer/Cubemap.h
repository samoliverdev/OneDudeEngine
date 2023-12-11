#pragma once

#include "OD/Defines.h"
#include "OD/Core/Asset.h"

namespace OD{

class Renderer;

class Cubemap: public Asset{
    friend class Renderer;
public:
    static bool CreateFromFile(
        Cubemap& cubemap,
        const char* right, const char* left, const char* top,
        const char* bottom, const char* front, const char* back
    ); 

    static void Destroy(Cubemap& cubemap);
    static void Bind(Cubemap& cubemap, int index);

    bool IsValid();
    
private:
    unsigned int renderId;
};

}