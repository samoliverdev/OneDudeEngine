#include "Defines.h"

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
