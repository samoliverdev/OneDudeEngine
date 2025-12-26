#include "OD/pch.h"
#include "TarPackage.h"
#include "OD/Base.h"
#include <cstring>
#include <cstddef>
#include <cstdint>
#include <fstream>

namespace OD{

static size_t parse_size(const char *octal_size){
    size_t size = 0;
    int digit_count = 1;

    for(int digit_idx = 11; digit_idx > 0; digit_idx--, digit_count *= 8){
        size += ((octal_size[digit_idx - 1] - '0') * digit_count);
    }

    return size;
}

const void *TarHeader::GetData() const {
    return (void *) ((uintptr_t) this + TAR_HEADER_ALIGNMENT);
}

const size_t TarHeader::GetSize() const {
    return parse_size(size);
}

TarEntryType TarHeader::GetType() const {
    switch (typeflag[0]) {
        case '1': return TarEntryType::hardlink;
        case '2': return TarEntryType::symlink;
        case '3': return TarEntryType::chardev;
        case '4': return TarEntryType::blockdev;
        case '5': return TarEntryType::dir;
        case '6': return TarEntryType::pipe;
        case '0':
        default:
        return TarEntryType::normal;
    }
}

TarPackage::TarPackage(const char* path){
    std::ifstream file(path, std::ios::binary | std::ios::ate);
    std::streamsize size = file.tellg();
    file.seekg(0, std::ios::beg);

    data = new char[size];
    if(file.read(data, size)){
        size_t offset = 0;
        while(true){
            const TarHeader *header = (const TarHeader *) ((uintptr_t) data + offset);

            if(header->filename[0] == '\0'){
                break;
            }

            size_t size = header->GetSize();
            m_headers.push_back(header);

            offset += ((size / TAR_HEADER_ALIGNMENT) + 1) * TAR_HEADER_ALIGNMENT;
            if (size % TAR_HEADER_ALIGNMENT != 0){
                offset += TAR_HEADER_ALIGNMENT;
            }
        }
    } else {
        Assert(false);
    }
}

TarPackage::TarPackage(const void *addr){
    size_t offset = 0;

    while(true){
        const TarHeader *header = (const TarHeader *) ((uintptr_t) addr + offset);

        if(header->filename[0] == '\0'){
            break;
        }

        size_t size = header->GetSize();
        m_headers.push_back(header);

        offset += ((size / TAR_HEADER_ALIGNMENT) + 1) * TAR_HEADER_ALIGNMENT;
        if (size % TAR_HEADER_ALIGNMENT != 0){
            offset += TAR_HEADER_ALIGNMENT;
        }
    }
}

const TarHeader *TarPackage::find_header(const std::string& filepath) const {
    //auto archive_path = "./" + filepath;
    auto archive_path = filepath;

    for(auto header : m_headers){
        auto header_path = header->filename;

        if(strncmp(header_path, archive_path.c_str(), 100) == 0){
            return header;
        }
    }

    return nullptr;
}

bool TarPackage::HasFile(const char* path){
    auto h = find_header(path);
    if(h == nullptr) return false;
    return true;
}

bool TarPackage::ReadFileData(const char* path, void*& outData, size_t& outSize){
    auto h = find_header(path);
    if(h == nullptr) return false;

    outData = (void*)h->GetData();
    outSize = (size_t)h->GetSize();
    return true;
}

void TarPackage::FreeFileData(void*& data){

}

}