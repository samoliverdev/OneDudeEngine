#pragma once

namespace OD{

class UniformBuffer{
public:
    static bool Create(UniformBuffer& buffer); 
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