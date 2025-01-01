#include "Base.h"
#include "Defines.h"
#include <random>

void* operator new[](size_t size, const char* name, int flags, unsigned debugFlags, const char* file, int line){
    return new uint8_t[size];
}

void* operator new[](unsigned __int64 size, unsigned __int64 alignment, unsigned __int64 offset, char const* pName, int flags, unsigned int debugFlags, char const* file, int line){
    return new uint8_t[size];
}

namespace OD {

static std::random_device randomizer;
static std::mt19937_64 generator(randomizer());
static std::uniform_int_distribution<uuid64> distribution;

uuid64 GenerateUUID(){
    uuid64 uuid = INVALID_ID;
    do{
        uuid = distribution(generator); 
    } while (uuid == INVALID_ID);
    return uuid;
}

}
