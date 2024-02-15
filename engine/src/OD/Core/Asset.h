#pragma once

#include "OD/Defines.h"

namespace OD{

class Asset{
public:
    virtual ~Asset(){}

    inline std::string& Path(){ return path; }
    inline void Path(const std::string& inPath){ path = inPath; }

    virtual void OnGui(){}
    virtual void Reload(){}
    virtual void Save(){}

protected:
    std::string path = "Memory";
};

}