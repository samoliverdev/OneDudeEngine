#include "SubShader.h"
#include "OD/Defines.h"
#include "OD/Core/ImGui.h"
#include "OD/Graphics/Graphics.h"
#include <string.h>
#include <sstream>
#include <fstream>
#include <algorithm>
#include <magic_enum/magic_enum.hpp>

namespace OD{

void ShaderPassData::UpdateProperties(){
    for(auto& line: properties){
        if(line.size() > 1 && line[0] == "SupportInstancing" && line[1] == "true"){
            pipeline.supportInstancing = true;
        }

        if(line.size() > 1 && line[0] == "Blend" && line[1] != "Off"){
            Assert(line.size() == 3);
            auto value1 = magic_enum::enum_cast<BlendMode>(line[1]);
            auto value2 = magic_enum::enum_cast<BlendMode>(line[2]);
            if(value1.has_value() && value2.has_value()){
                pipeline.blend = true;
                pipeline.srcBlend = value1.value();
                pipeline.dstBlend = value2.value();
            }
        }

        if(line.size() > 1 && line[0] == "Name"){
            Assert(line.size() == 2);
            name = line[1];
        }

        if(line.size() > 1 && line[0] == "CullFace"){
            Assert(line.size() == 2);
            auto value1 = magic_enum::enum_cast<CullFace>(line[1]);
            if(value1.has_value()) pipeline.cullFace = value1.value();
        }

        if(line.size() > 1 && line[0] == "DepthTest"){
            Assert(line.size() == 2);
            auto value1 = magic_enum::enum_cast<DepthTest>(line[1]);
            if(value1.has_value()) pipeline.depthTest = value1.value(); 
        }

        if(line.size() > 1 && line[0] == "DepthMask"){
            Assert(line.size() == 2);
            if(line[1] == "True") pipeline.depthMask = true;
            if(line[1] == "False") pipeline.depthMask = false;
        }

        if(line.size() > 1 && line[0] == "ColorMask"){
            Assert(line.size() == 5);
            pipeline.colorMask.r = std::stof(line[1]);
            pipeline.colorMask.g = std::stof(line[2]);
            pipeline.colorMask.b = std::stof(line[3]);
            pipeline.colorMask.a = std::stof(line[4]);
        }
    }
}

void getFilePath(const std::string& fullPath, std::string& pathWithoutFileName){
    return;
    //LogInfo("FullPath: %s", fullPath.c_str());
    // Remove the file name and store the path to this folder
    size_t found = fullPath.find_last_of("/\\");
    pathWithoutFileName = fullPath.substr(0, found + 1);
}

std::string _load(std::string path, ShaderSourceData& out){
    std::string includeIndentifier = "#include ";
    static bool isRecursiveCall = false;

    std::string fullSourceCode = "";
    std::ifstream file(path);

    if(!file.is_open()){
        std::cerr << "ERROR: could not open the shader at: " << path << "\n" << std::endl;
        return fullSourceCode;
    }

    bool beginProperties = false;
    bool beginPass = false;
    bool beginPass2 = false;

    std::string lineBuffer;
    while(std::getline(file, lineBuffer)){
        std::vector<std::string> pragmaLine;
        std::stringstream ss(lineBuffer);
        std::string _out;
        int index = 0;

        if(lineBuffer.find("#pragma") != lineBuffer.npos){
            pragmaLine.clear();
            while(ss >> _out){
                if(index != 0) pragmaLine.push_back(_out);
                index += 1;
            }
        }

        if(lineBuffer.find("BeginProperties") != lineBuffer.npos){
            beginProperties = true;
            pragmaLine.clear();
            continue;
        }
        if(lineBuffer.find("EndProperties") != lineBuffer.npos){
            pragmaLine.clear();
            beginProperties = false;
            continue;
        }

        if(beginProperties == false && pragmaLine.size() > 0 && pragmaLine[0] == "BeginProperties"){
            beginProperties = true;
            continue;
        }
        if(beginProperties == true && pragmaLine.size() > 0 && pragmaLine[0] == "EndProperties"){
            beginProperties = false;
            continue;
        }
        if(beginProperties){
            pragmaLine.clear();
            while(ss >> _out){
                pragmaLine.push_back(_out);
                index += 1;
            }
            out.properties.push_back(pragmaLine);
            //LogInfo("Propertie: %s, %s", pragmaLine[0].c_str(), pragmaLine[1].c_str());
            continue;
        }

        if(beginPass == false && pragmaLine.size() > 0 && pragmaLine[0] == "BeginPassDef"){
            beginPass = true;
            out.passes.push_back(ShaderPassData());
            continue;
        }
        if(beginPass == true && pragmaLine.size() > 0 && pragmaLine[0] == "EndPassDef"){
            beginPass = false;
            out.passes[out.passes.size()-1].UpdateProperties();
            continue;
        }
        if(beginPass){
            pragmaLine.clear();
            while(ss >> _out){
                pragmaLine.push_back(_out);
                index += 1;
            }
            out.passes[out.passes.size()-1].properties.push_back(pragmaLine);
            //LogInfo("Propertie: %s, %s", pragmaLine[0].c_str(), pragmaLine[1].c_str());
            continue;
        }

        if(pragmaLine.size() > 0) out.pragmas.push_back(pragmaLine);

        std::string replaceIndentifier = "BeginPass";
        if(lineBuffer.find(replaceIndentifier) != lineBuffer.npos){
            int pos = lineBuffer.find(replaceIndentifier);
            lineBuffer.erase(pos, replaceIndentifier.size());
            lineBuffer.insert(pos, "#if defined(Pass_" + std::to_string(out.passes.size()) + ")");

            beginPass2 = true;
            out.passes.push_back(ShaderPassData());
        }

        replaceIndentifier = "EndPass";
        if(lineBuffer.find(replaceIndentifier) != lineBuffer.npos){
            int pos = lineBuffer.find(replaceIndentifier);
            lineBuffer.erase(pos, replaceIndentifier.size());
            lineBuffer.insert(pos, "#endif");
            
            out.passes[out.passes.size()-1].UpdateProperties();
            beginPass2 = false;
        }

        if(beginPass2 == true && pragmaLine.size() > 0){
            out.passes[out.passes.size()-1].properties.push_back(pragmaLine);
        }

        replaceIndentifier = "BeginVertex";
        if(lineBuffer.find(replaceIndentifier) != lineBuffer.npos){
            int pos = lineBuffer.find(replaceIndentifier);
            lineBuffer.erase(pos, replaceIndentifier.size());
            lineBuffer.insert(pos, "#if defined(VERTEX)");
        }
        replaceIndentifier = "EndVertex";
        if(lineBuffer.find(replaceIndentifier) != lineBuffer.npos){
            int pos = lineBuffer.find(replaceIndentifier);
            lineBuffer.erase(pos, replaceIndentifier.size());
            lineBuffer.insert(pos, "#endif");
        }
        replaceIndentifier = "BeginFrag";
        if(lineBuffer.find(replaceIndentifier) != lineBuffer.npos){
            int pos = lineBuffer.find(replaceIndentifier);
            lineBuffer.erase(pos, replaceIndentifier.size());
            lineBuffer.insert(pos, "#if defined(FRAGMENT)");
        }
        replaceIndentifier = "EndFrag";
        if(lineBuffer.find(replaceIndentifier) != lineBuffer.npos){
            int pos = lineBuffer.find(replaceIndentifier);
            lineBuffer.erase(pos, replaceIndentifier.size());
            lineBuffer.insert(pos, "#endif");
        }

        if(lineBuffer.find(includeIndentifier) != lineBuffer.npos){
            int includePos = lineBuffer.find(includeIndentifier);
            lineBuffer.erase(includePos, includeIndentifier.size());

            std::string pathOfThisFile;
            getFilePath(path, pathOfThisFile);
            lineBuffer.insert(0, pathOfThisFile);

            if(lineBuffer[lineBuffer.size()-1] == '\r') lineBuffer.erase(lineBuffer.length()-1);

            lineBuffer.erase(
                std::remove(lineBuffer.begin(), lineBuffer.end(), ' '), 
                lineBuffer.end()
            );
            lineBuffer.erase(
                std::remove(lineBuffer.begin(), lineBuffer.end(), '"'), 
                lineBuffer.end()
            );

            isRecursiveCall = true;
            fullSourceCode += _load(lineBuffer, out);
            continue;
        }

        fullSourceCode += lineBuffer + '\n';
    }
    if(!isRecursiveCall) fullSourceCode += '\0';

    file.close();

    return fullSourceCode;
}

bool ShaderLoadFile(const std::string& path, ShaderSourceData& out){
    out.baseSource = _load(path, out);
    return true;
}

}