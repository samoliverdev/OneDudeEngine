#pragma once

#include "OD/Defines.h"

namespace OD{

class Asset{
public:
    inline std::string& path(){ return _path; }
    inline void path(const std::string& path){ _path = path; }

    virtual void OnGui(){}

protected:
    std::string _path = "Memory";
};

}