#include "Model.h"
#include "OD/Loader/AssimpLoader.h"

namespace OD{

Ref<Model> Model::CreateFromFile(std::string const &path, Ref<Shader> customShader){
    return AssimpLoadModel(path, customShader);

}

}