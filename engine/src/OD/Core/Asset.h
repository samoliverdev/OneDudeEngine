#pragma once

#include "OD/Defines.h"

namespace OD{

class Asset{
public:
    inline std::string& path(){ return _path; }

protected:
    std::string _path;
};

}