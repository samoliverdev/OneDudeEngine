#pragma once
#include "OD/Defines.h"

namespace OD{

class OD_API UniformBuffer{
public:
    static Ref<UniformBuffer> Create();
    static void Destroy(UniformBuffer& buffer);
    static void Bind(UniformBuffer& buffer, int bind);

    void SetData(const void* data, unsigned int size, unsigned int offset = 0);

    inline bool IsValid(){ return rendererId != 0; }
    inline unsigned int RendererId(){ return rendererId; }
    //inline int GetBind(){ return bind; }
private:
    unsigned int rendererId = 0;
    //int bind = 0;
};

}