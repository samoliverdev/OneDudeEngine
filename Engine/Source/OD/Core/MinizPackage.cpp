#include "MinizPackage.h"

namespace OD{

MinizPackage::MinizPackage(const char* zipPath){
    memset(&m_zip, 0, sizeof(m_zip));
    mz_zip_reader_init_file(&m_zip, zipPath, 0);
}

MinizPackage::~MinizPackage(){
    mz_zip_reader_end(&m_zip);
}

bool MinizPackage::HasFile(const char* path){
    return mz_zip_reader_locate_file(&m_zip, path, nullptr, 0) >= 0;
}

size_t MinizPackage::GetFileSize(const char* path) const{
    int index = mz_zip_reader_locate_file(const_cast<mz_zip_archive*>(&m_zip), path, nullptr, 0);

    if(index < 0) return 0;

    mz_zip_archive_file_stat stat;
    mz_zip_reader_file_stat(const_cast<mz_zip_archive*>(&m_zip), index, &stat);

    return (size_t)stat.m_uncomp_size;
}

bool MinizPackage::ReadFileData(const char* path, void*& outData, size_t& outSize){
    size_t size = 0;
    void* data = mz_zip_reader_extract_file_to_heap(&m_zip, path, &size, 0);

    if(!data) return false;

    outData = data;
    outSize = size;
    return true;
}

void MinizPackage::FreeFileData(void*& data){
    mz_free(data);
    data = nullptr;
}

bool MinizPackage::ReadFile(const char* path, std::vector<uint8_t>& outData){
    size_t size = 0;
    void* data = mz_zip_reader_extract_file_to_heap(&m_zip, path, &size, 0);

    if(!data) return false;

    outData.resize(size);
    memcpy(outData.data(), data, size);
    mz_free(data);

    return true;
}

std::vector<std::string> MinizPackage::ListFiles(const char* directory) const{
    std::vector<std::string> files;

    int count = mz_zip_reader_get_num_files(const_cast<mz_zip_archive*>(&m_zip));

    for(int i = 0; i < count; i++){
        mz_zip_archive_file_stat stat;
        mz_zip_reader_file_stat(const_cast<mz_zip_archive*>(&m_zip), i, &stat);

        std::string name = stat.m_filename;

        if(name.find(directory) == 0) files.push_back(name);
    }

    return files;
}


}