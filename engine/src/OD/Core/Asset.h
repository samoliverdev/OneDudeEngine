#pragma once

#include "OD/Defines.h"

namespace OD{

class Asset{
public:
    inline std::string& path(){ return _path; }

    virtual void OnGui(){}

protected:
    std::string _path;
};

}