#include "Model.h"
#include "OD/Loader/AssimpLoader.h"

namespace OD{

bool Model::CreateFromFile(Model& model, std::string const &path, Ref<Shader> customShader){
    return AssimpLoadModel(model, path, customShader);
}

}