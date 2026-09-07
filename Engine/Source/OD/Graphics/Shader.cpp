#include "OD/pch.h"
#include "Shader.h"
#include "Graphics.h"
#include "GraphicsDevice.h"
#include "OD/Gfx/GfxReflection.h"
#include "OD/Core/Hash.h"
#include "OD/Serialization/SerializationFull.h"
#include <numeric>
#include <fstream>

namespace OD{

extern GraphicsDevice* graphicsDevice;

extern Gfx::Device* gfxDevice;
extern Gfx::BindGroupLayout emptyLayout;
extern Gfx::BindGroupLayout camGroupLayout;
extern Gfx::BindGroupLayout drawDrawMeshGroupLayout;

void _Combine_(std::vector<std::vector<std::string>> terms, std::string accum, std::vector<std::string>& combinations){
    bool last = (terms.size() == 1);
    int n = terms[0].size();
    for(int i = 0; i < n; i++){
        std::string item = accum + "_" + terms[0][i];
        if(last){
            combinations.push_back(item);
        } else{
            auto newTerms = terms;
            newTerms.erase(newTerms.begin());
            _Combine_(newTerms, item, combinations);
        }
    }
}

std::set<std::string> Split_(std::string s, std::string delimiter){
    size_t pos_start = 0, pos_end, delim_len = delimiter.size();
    std::string token;
    std::set<std::string> res;

    while((pos_end = s.find(delimiter, pos_start)) != std::string::npos){
        token = s.substr(pos_start, pos_end - pos_start);
        pos_start = pos_end + delim_len;
        if(token.empty() == false) res.insert(token);
    }
    
    std::string _s = s.substr(pos_start);
    if(_s.empty() == false) res.insert(_s);
    return res;
}

std::set<std::string> Split2_(std::string s, std::string delimiter){
    using namespace std;

    set<string> res;
    string token = "";
    for (int i = 0; i < s.size(); i++) {
        bool flag = true;
        for (int j = 0; j < delimiter.size(); j++) {
            if (s[i + j] != delimiter[j]) flag = false;
        }
        if (flag) {
            if (token.size() > 0) {
                res.insert(token);
                token = "";
                i += delimiter.size() - 1;
            }
        } else {
            token += s[i];
        }
    }
    res.insert(token);
    return res;
}

std::string GetKey_(const std::set<std::string>& keyworlds){
    if(keyworlds.size() == 0) return "";
    return std::accumulate(keyworlds.begin(), keyworlds.end(), std::string(""));
}

Shader::Shader(std::string path){
    Create(path);
}

Ref<Shader> Shader::CreateFromFile(const std::string& filepath){
    Ref<Shader> out = CreateRef<Shader>();
    if(out->LoadFromFile(filepath) == false){ //if(out->Create(filepath) == false){
        return nullptr;
    }
    return out;
}

bool Shader::LoadFromFile(const std::string& path){
    auto getExtension = [](const std::string& path) -> std::string {
        size_t dotPos = path.rfind('.');
        return (dotPos != std::string::npos) ? path.substr(dotPos + 1) : "";
    };

    LogInfo("Load Shader: {}", path);

    std::string fileType = getExtension(path);
    if(fileType == "shaderbin"){
        Destroy();

        std::ifstream file(path);
        if(!file.is_open()) return false;

        cereal::JSONInputArchive ar(file);
        ar(shaderSourceData);

        this->path = path;
        errors.clear();
        isComplete = true;
        sourcePath = path;
        //ShaderLoadFile(path, shaderSourceData);
        passes.resize(shaderSourceData.passes.size());
        for(int i = 0; i < passes.size(); i++){
            passes[i].name = shaderSourceData.passes[i].name;
            bool r = InitPass(i);
            if(r == false) break;
        }

        if(isComplete == false){
            LogError("Error To Compile Shader: {}", path);
            Destroy();
            return false;
        } 

        return true;
    }

    if(fileType == "glsl" || fileType == "shader"){
        Destroy();
    
        this->path = path;
        errors.clear();
        isComplete = true;
        sourcePath = path;
        ShaderLoadFile(path, shaderSourceData);
        passes.resize(shaderSourceData.passes.size());
        for(int i = 0; i < passes.size(); i++){
            passes[i].name = shaderSourceData.passes[i].name;
            bool r = InitPass(i);
            if(r == false) break;
        }

        if(isComplete == false){
            LogError("Error To Compile Shader: {}", path);
            Destroy();
            return false;
        } 

        return true;
    }

    #ifdef TestNewGPU_API
    Assert(false);
    return false;
    #else
    return graphicsDevice->ShaderCreate(*this, path);
    #endif
}

bool Shader::LoadFromPackage(const std::string& path, Package& package){
    auto getExtension = [](const std::string& path) -> std::string {
        size_t dotPos = path.rfind('.');
        return (dotPos != std::string::npos) ? path.substr(dotPos + 1) : "";
    };

    std::string fileType = getExtension(path);
    Assert(fileType != "glsl" && "File dont surpoted by package");
    Assert(fileType != "shader" && "File dont surpoted by package");

    void* data;
	size_t dataSize;
	if(package.ReadFileData(path.c_str(), data, dataSize) == false){
		package.FreeFileData(data);
		return false;
	}

    MemoryInputStream mem((char*)data, dataSize);
	cereal::JSONInputArchive ar(mem);

    ar(shaderSourceData);
    this->path = path;
    errors.clear();
    isComplete = true;
    sourcePath = path;
    //ShaderLoadFile(path, shaderSourceData);
    passes.resize(shaderSourceData.passes.size());
    for(int i = 0; i < passes.size(); i++){
        passes[i].name = shaderSourceData.passes[i].name;
        bool r = InitPass(i);
        if(r == false) break;
    }
    if(isComplete == false){
        LogError("Error To Compile Shader: {}", path);
        Destroy();
        return false;
    } 

    package.FreeFileData(data);
    return true;
}

std::vector<std::string> Shader::GetFileAssociations(){ 
    return std::vector<std::string>{
        ".shader",
        ".glsl",
        ".shaderbin"
    }; 
}

bool Shader::Save(const std::string& outPath, SaveType type){
    if(type == Resource::SaveType::AssetBinary){
		Assert(false);
    }

    std::ofstream file(outPath); 
    if(!file.is_open()) return false;

    cereal::JSONOutputArchive ar(file);
    ar(shaderSourceData);
    return true;
}

bool Shader::Create(std::string inPath){
    Assert(false && "Outdata");
    //LogInfo("Create Shader: %s", inPath.c_str());
    Destroy();
    
    path = inPath;
    errors.clear();
    isComplete = true;
    sourcePath = path;
    ShaderLoadFile(path, shaderSourceData);
    passes.resize(shaderSourceData.passes.size());
    for(int i = 0; i < passes.size(); i++){
        passes[i].name = shaderSourceData.passes[i].name;
        bool r = InitPass(i);
        if(r == false) break;
    }

    if(isComplete == false){
        LogError("Error To Compile Shader: {}", path);
        Destroy();
        return false;
    } 

    return true;
}

void Shader::Destroy(){
    for(auto& i: passes){
        for(auto& j: i.shaders){
            //if(j.second == nullptr) continue;
            //graphicsDevice->SubShaderDestroy(*j.second); //SubShader::Destroy(*j.second);

            //if(j.second.drawTypes[0] != nullptr) graphicsDevice->SubShaderDestroy(*j.second.drawTypes[0]);
            //if(j.second.drawTypes[1] != nullptr) graphicsDevice->SubShaderDestroy(*j.second.drawTypes[1]);
            //if(j.second.drawTypes[2] != nullptr) graphicsDevice->SubShaderDestroy(*j.second.drawTypes[2]);

            for(int _i = 0; _i < (int)Shader::DrawType::Count; _i++){
                if(j.second.drawTypes[_i] != nullptr){
                    #ifdef TestNewGPU_API
                    Assert(false);
                    #else
                    graphicsDevice->SubShaderDestroy(*j.second.drawTypes[_i]);
                    #endif
                }
            }
        }
    }
    passes.clear();
    errors.clear();
    shaderSourceData = {};
    keyworldSpaces.clear();
    curPass = 0;
    currentShader = {}; //nullptr;
    sourcePath = "";
    isComplete = false;
    path = "Memory";
}

bool Shader::InitPass(int pass){
    //passes[pass].shaders[""] = baseShader;
    //currentShader = baseShader;

    //AddShaderVaring("", std::set<std::string>(), pass, {"DefaultDraw", "SkinnedDraw", "InstancingDraw"});
    //if(isComplete == false) return false;

    std::vector<std::vector<std::string>> multCompile;
    std::vector<std::string> combinations;

    std::set<DrawType> drawTypes = { Shader::DrawType::DefaultDraw};
    

    for(auto i: shaderSourceData.passes[pass].properties /*baseShader->Pragmas()*/){
        if(i.size() < 2) continue;

        //LogInfo("!!!=>>> %s", i[0].c_str()); 
        //continue;
        
        if(i[0] == "MultiCompile"){
            multCompile.push_back(std::vector<std::string>());
            keyworldSpaces.push_back(KeyworldSpace());

            for(int j = 1; j < i.size(); j++){
                multCompile[multCompile.size()-1].push_back(i[j]);
                keyworldSpaces[keyworldSpaces.size()-1].keyworlds.push_back(i[j]);
            }
        }

        if(i[0] == "DrawType"){
            //TODO: On here i think is trigger SKINNED and SKINNED2 at same time
            for(int j = 1; j < i.size(); j++){
                if(i[j] == "SKINNED") drawTypes.insert(Shader::DrawType::SkinnedDraw);
                if(i[j] == "INSTANCING") drawTypes.insert(Shader::DrawType::InstancingDraw);
                if(i[j] == "INSTANCINGMATRIX43") drawTypes.insert(Shader::DrawType::InstancingDraw43);
                if(i[j] == "SKINNED2") drawTypes.insert(Shader::DrawType::SkinnedDraw2);
            }
        }

        if(i[0] == "Tags"){
            for(int j = 1; j < i.size(); j++){
                passes[pass].tagsString.push_back(i[j]);
                passes[pass].tagsHash.push_back(Hash::StringToHash(i[j]));
            } 
        }
    }
    for(auto& i: multCompile){
        bool c = false;
        for(auto j: i){
            if(j == "_") c = true;
        }
        if(c == false) i.push_back("_");
    }

    AddShaderVaring("", std::set<std::string>(), pass, drawTypes);
    if(isComplete == false) return false;
    
    if(multCompile.size() < 1) return true;

    _Combine_(multCompile, std::string(""), combinations);
    //LogInfo("-----------All Shader Varing-------------");
    for(std::string s: combinations){
        std::set<std::string> keywords = Split_(s, "_");
        std::string key = GetKey_(keywords);

        /*for(auto j: keywords){
            LogInfo("SplitValue: %s", j.c_str());
        }*/
        //LogInfo("Shader Varing Key: \"%s\" Original: \"%s\" KeywordsCount: %zd", key.c_str(), s.c_str(), keywords.size());
        if(passes[pass].shaders.count(key) == false) {
            AddShaderVaring(key, keywords, pass, drawTypes);
            if(isComplete == false) return false;
        }

        /*std::string shaderVaring = baseShader->Path();
        std::string suffix = ".glsl";
        shaderVaring = shaderVaring.substr(0, shaderVaring.length() - suffix.length());
        shaderVaring += key + ".shader";
        LogInfo("%s", shaderVaring.c_str());*/
    }

    return true;
}

/*
void Shader::DisableKeyword(std::string keyword){
    //enabledKeywords.erase(keyword);return;

    for(auto& i: keyworldSpaces){
        for(auto j: i.keyworlds){
            if(j == keyword){
                i.enabledKey = -1;
                break;
            }
        }
    }
}

void Shader::EnableKeyword(std::string keyword){
    //enabledKeywords.insert(keyword);return;

    for(auto& i: keyworldSpaces){
        int index = 0;
        for(auto j: i.keyworlds){
            if(j == keyword){
                i.enabledKey = index;
                break;
            }
            index += 1;
        }
    }
}

void Shader::SetPass(int pass){
    curPass = pass;
}
*/

/*std::set<std::string> Shader::GetEnabledKeywords(){
    //return enabledKeywords;

    std::set<std::string> out;
    for(auto i: keyworldSpaces){
        if(i.enabledKey < 0){
            out.insert("");
        } else {
            out.insert(i.keyworlds[i.enabledKey]);
        }
    }
    return out;
}*/

/*
void Shader::SetCurrentShader(){
    UpdateCurrentShader();
}

Ref<SubShader> Shader::GetCurrentShader(){
    return currentShader;
}
*/

/*void Shader::UpdateCurrentShader(){
    std::string key = GetKey_(GetEnabledKeywords());
    //LogInfo("Key: %s", key.c_str());

    if(passes[curPass].shaders.count(key)){
        currentShader = passes[curPass].shaders[key];
    } else {
        Assert(false);
        AddShaderVaring(key, GetEnabledKeywords(), curPass);
    }
}*/

std::string ExtractShaderName(const std::string& path){
    size_t slash = path.find_last_of("/\\");
    std::string filename = (slash == std::string::npos) ? path : path.substr(slash + 1);

    size_t dot = filename.find_last_of('.');
    if(dot != std::string::npos)
        filename = filename.substr(0, dot);

    return filename;
}

std::string BuildKeywordString(const std::vector<std::string>& keywords){
    std::string result;

    for(size_t i = 0; i < keywords.size(); i++){
        result += keywords[i];
        if(i != keywords.size() - 1)
            result += "_";
    }

    return result;
}

std::string DrawTypeToString(Shader::DrawType type){
    switch(type){
        case Shader::DrawType::DefaultDraw: return "DEFAULT";
        case Shader::DrawType::SkinnedDraw: return "SKINNED";
        case Shader::DrawType::InstancingDraw: return "INSTANCING";
        case Shader::DrawType::InstancingDraw43: return "INSTANCING43";
        case Shader::DrawType::SkinnedDraw2: return "SKINNED2";
    }
    return "UNKNOWN";
}

void Shader::AddShaderVaring(std::string key, const std::set<std::string>& keywords, int pass, const std::set<DrawType>& drawTypes){
    std::vector<std::string> _enabledKeywords(keywords.begin(), keywords.end());
    _enabledKeywords.push_back(passes[pass].name);
    _enabledKeywords.push_back("Pass_" + std::to_string(pass));

    int startSize = shaderSourceData.baseSource.size();
    int insirtSize = 0;
    for(auto& i: _enabledKeywords){
        std::string keyworld = "#define " + i + "\n";
        shaderSourceData.baseSource.insert(0, keyworld);
        insirtSize += keyworld.size();
    }

    //LogWarning("%s", shaderSourceData.baseSource.c_str());

    SubShaderTarget _shader;

    for(auto& drawType: drawTypes){
        std::string skinnedKeyworld = "#define SKINNED\n";
        std::string instancingKeyworld = "#define INSTANCING\n";
        std::string instancing43Keyworld = "#define INSTANCINGMATRIX43\n";
        std::string skinned2Keyworld = "#define SKINNED2\n";

        if(drawType == Shader::DrawType::SkinnedDraw) shaderSourceData.baseSource.insert(0, skinnedKeyworld);
        if(drawType == Shader::DrawType::InstancingDraw) shaderSourceData.baseSource.insert(0, instancingKeyworld);
        if(drawType == Shader::DrawType::InstancingDraw43) shaderSourceData.baseSource.insert(0, instancing43Keyworld);
        if(drawType == Shader::DrawType::SkinnedDraw2) shaderSourceData.baseSource.insert(0, skinned2Keyworld);

        Ref<SubShader> shader = CreateRef<SubShader>();

        std::string baseName = ExtractShaderName(path);
        std::string keywordStr = BuildKeywordString(_enabledKeywords);
        std::string drawTypeStr = DrawTypeToString(drawType);
        shader->name = baseName + "_" + keywordStr + "_" + drawTypeStr;

        #ifdef TestNewGPU_API
        //std::string GFX_API = "#define GFX_API\n";
        //shaderSourceData.baseSource.insert(0, GFX_API);

        //Assert(false);
        if(shader->_pipeline != Gfx::InvalidID) gfxDevice->DestroyPipeline(shader->_pipeline);

        Gfx::ShaderReflection reflection;
        Gfx::Reflect(shaderSourceData.baseSource.c_str(), reflection);

        Gfx::PipelineInfo pipelineInfo = {};

        std::vector<Gfx::BindGroupLayoutInfo> layoutsOut;
        Gfx::ShaderReflectionToPipelineInfo(reflection, pipelineInfo, layoutsOut);

        pipelineInfo.framebufferLayout = gfxDevice->GetWindowFrameBufferLayout();

        pipelineInfo.bindGroupLayouts[0] = layoutsOut[0].entriesCount == 0 ? emptyLayout : gfxDevice->CreateBindGroupLayout(layoutsOut[0]);
        pipelineInfo.bindGroupLayouts[1] = drawDrawMeshGroupLayout;
        pipelineInfo.bindGroupLayouts[2] = camGroupLayout;
        pipelineInfo.bindGroupLayoutCount = 3;
        
        /*for(int i = 0; i < layoutsOut.size(); i++){
            if(layoutsOut[i].entriesCount == 0){
                pipelineInfo.bindGroupLayouts[i] = Gfx::InvalidID;
            } else {
                auto info = layoutsOut[i];
                pipelineInfo.bindGroupLayouts[i] = gfxDevice->CreateBindGroupLayout(info);
            }
        }*/

        //pipelineInfo.bindGroupLayoutCount = _i;

        /*pipelineInfo.vertexLayout.attributes[0] = {Gfx::VertexSemantic::Position, Gfx::VertexFormat::Float3, 0, 0};
        pipelineInfo.vertexLayout.attributes[1] = {Gfx::VertexSemantic::UV0, Gfx::VertexFormat::Float3, 1, 0};
        pipelineInfo.vertexLayout.attributeCount = 2;
        pipelineInfo.vertexLayout.buffers[0] = {sizeof(float) * 3, Gfx::VertexInputRate::Vertex};
        pipelineInfo.vertexLayout.buffers[1] = {sizeof(float) * 3, Gfx::VertexInputRate::Vertex};
        pipelineInfo.vertexLayout.bufferCount = 2;*/

        pipelineInfo.vertexLayout.bufferCount = pipelineInfo.vertexLayout.attributeCount;
        for(int i = 0; i < pipelineInfo.vertexLayout.attributeCount; i++){
            pipelineInfo.vertexLayout.buffers[i] = {sizeof(Vector3), Gfx::VertexInputRate::Vertex};
        }

        shader->_pipeline = gfxDevice->CreatePipeline(shaderSourceData.baseSource.c_str(), pipelineInfo);

        //shaderSourceData.baseSource.erase(0, GFX_API.size());
        #else
        graphicsDevice->SubShaderCreateFromBaseSource(
            *shader,
            shaderSourceData.baseSource, 
            _enabledKeywords, 
            shaderSourceData.passes[pass].pipeline,
            errors
        );
        #endif

        if(shader == nullptr){
            isComplete = false;
        }

        if(drawType == Shader::DrawType::SkinnedDraw) shaderSourceData.baseSource.erase(0, skinnedKeyworld.size());
        if(drawType == Shader::DrawType::InstancingDraw) shaderSourceData.baseSource.erase(0, instancingKeyworld.size());
        if(drawType == Shader::DrawType::InstancingDraw43) shaderSourceData.baseSource.erase(0, instancing43Keyworld.size());
        if(drawType == Shader::DrawType::SkinnedDraw2) shaderSourceData.baseSource.erase(0, skinned2Keyworld.size());

        _shader.drawTypes[(int)drawType] = shader;
    }
    passes[pass].shaders[key] = _shader;

    /*Ref<SubShader> shader = CreateRef<SubShader>();
    graphicsDevice->SubShaderCreateFromBaseSource(
        *shader,
        shaderSourceData.baseSource, 
        _enabledKeywords, 
        shaderSourceData.passes[pass].pipeline,
        errors
    );
    if(shader == nullptr){
        isComplete = false;
    }
    passes[pass].shaders[key] = shader;*/
    //currentShader = shader;

    shaderSourceData.baseSource.erase(0, insirtSize);
    Assert(shaderSourceData.baseSource.size() == startSize);
}

}