#pragma once
#include "OD/Defines.h"
#include <string>
#include <vector>

namespace OD{

class OD_API Package{
public:
    virtual bool ReadFile(const char* path, void*& outData, size_t& outSize) = 0;
};

#define TAR_HEADER_ALIGNMENT  512

enum class TarEntryType{
    normal,
    hardlink,
    symlink,
    chardev,
    blockdev,
    dir,
    pipe
};

struct OD_API TarHeader{
    char filename[100];
    char mode[8];
    char uid[8];
    char gid[8];
    char size[12];
    char mtime[12];
    char checksum[8];
    char typeflag[1];
    char linkname[100];

    char ustar_indicator[6];
    char ustar_version[2];

    char user[32];
    char group[32];

    char device_major[8];
    char device_minor[8];

    char filename_prefix[155];

    const void* GetData() const;
    const size_t GetSize() const;
    TarEntryType GetType() const;
};

class OD_API TarPackage: public Package{
public:
    TarPackage(const char* path);
    TarPackage(const void *addr);
    const TarHeader *find_header(const std::string &filepath) const;
    bool ReadFile(const char* path, void*& outData, size_t& outSize) override;

private:
    char* data = nullptr;
    std::vector<const TarHeader*> m_headers;
};

}