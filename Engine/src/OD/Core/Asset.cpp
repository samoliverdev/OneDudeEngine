#include "Asset.h"

namespace OD{

AssetManager global;

AssetManager& AssetManager::Get(){
    return global;
}

}