#pragma once

#include "OD/Defines.h"

namespace OD{

class Asset{
public:
    inline std::string& Path(){ return path; }
    inline void Path(const std::string& inPath){ path = inPath; }

    virtual void OnGui(){}

protected:
    std::string path = "Memory";
};

}