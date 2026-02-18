#pragma once
#include "Package.h"
#include <physfs.h>

namespace OD{

class OD_API PhysFS{
public:
    static void Init();
    static void Shutdown();

    static bool Mount(const char* path, bool highPriority = false);
    static bool Unmount(const char* path);

    static Package* GetPackage();
};

}