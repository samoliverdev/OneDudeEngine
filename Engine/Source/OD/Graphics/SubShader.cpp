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

namespace Experimental{

bool MatchFunctionHeader(const std::string& code, size_t& pos, const std::string& functionName, const std::string& prefix = "void", bool checkParentheses = true) {
    size_t i = pos;

    // Skip leading whitespace
    while (i < code.size() && std::isspace(code[i])) ++i;

    // Match the prefix (e.g., "void" or "inline")
    if (!prefix.empty()) {
        if (code.compare(i, prefix.length(), prefix) != 0) {
            return false;
        }
        i += prefix.length();
    }

    // Skip whitespace after the prefix
    while (i < code.size() && std::isspace(code[i])) ++i;

    // Match the function name
    if (code.compare(i, functionName.length(), functionName) != 0) {
        return false;
    }
    i += functionName.length();

    // Skip whitespace after the function name
    while (i < code.size() && std::isspace(code[i])) ++i;

    // Optionally check for parentheses
    if (checkParentheses) {
        // Match the opening parenthesis '('
        if (i >= code.size() || code[i] != '(') return false;
        i++; // Skip '('

        // Skip any whitespace inside the parentheses
        while (i < code.size() && std::isspace(code[i])) ++i;

        // Match the closing parenthesis ')'
        if (i >= code.size() || code[i] != ')') return false;
        i++; // Skip ')'

        // Skip whitespace after the closing parenthesis
        while (i < code.size() && std::isspace(code[i])) ++i;
    }

    // Match the opening brace '{'
    if (i >= code.size() || code[i] != '{') {
        return false;
    }

    // Update pos to the position after '{'
    pos = i;
    return true;
}

std::string WrapFunctionWithIfDef(const std::string& shaderCode, const std::string& functionName, const std::string& prefix = "void", bool checkParentheses = true) {
    size_t pos = 0;
    while (pos < shaderCode.length()) {
        size_t temp = pos;
        if (MatchFunctionHeader(shaderCode, temp, functionName, prefix, checkParentheses)) {
            pos = temp;

            // Find beginning of "void ..."
            size_t funcStart = shaderCode.rfind("void", pos);
            if (funcStart == std::string::npos) return shaderCode;

            // Find opening brace
            size_t braceStart = shaderCode.find('{', pos);
            if (braceStart == std::string::npos) return shaderCode;

            // Find matching closing brace
            size_t braceEnd = braceStart + 1;
            int braceCount = 1;
            while (braceEnd < shaderCode.size() && braceCount > 0) {
                if (shaderCode[braceEnd] == '{') braceCount++;
                else if (shaderCode[braceEnd] == '}') braceCount--;
                ++braceEnd;
            }

            if (braceCount != 0) return shaderCode;

            // Extract full function
            size_t funcEnd = braceEnd;
            std::string fullFunc = shaderCode.substr(funcStart, funcEnd - funcStart);

            // Build wrapped function
            std::string wrappedFunc =
                "#if defined(" + functionName + ")\n" +
                fullFunc + "\n" +
                "#endif\n";

            // Replace in code
            std::string newCode = shaderCode.substr(0, funcStart)
                                + wrappedFunc
                                + shaderCode.substr(funcEnd);

            return newCode;
        }
        pos++;
    }

    return shaderCode;
}

std::string WrapFunctionWithIfDef2(const std::string& shaderCode,
                                  const std::string& functionName,
                                  const std::string& prefix = "void",
                                  bool checkParentheses = true,
                                  const std::string& directiveName = "",
                                  const std::string& replaceFunctionName = "") {
    size_t pos = 0;
    while (pos < shaderCode.length()) {
        size_t temp = pos;
        if (MatchFunctionHeader(shaderCode, temp, functionName, prefix, checkParentheses)) {
            pos = temp;

            // Localizar início da função com base no prefixo
            size_t funcStart = shaderCode.rfind(prefix, pos);
            if (funcStart == std::string::npos) return shaderCode;

            // Localizar início do bloco
            size_t braceStart = shaderCode.find('{', pos);
            if (braceStart == std::string::npos) return shaderCode;

            // Encontrar o final do bloco
            size_t braceEnd = braceStart + 1;
            int braceCount = 1;
            while (braceEnd < shaderCode.size() && braceCount > 0) {
                if (shaderCode[braceEnd] == '{') braceCount++;
                else if (shaderCode[braceEnd] == '}') braceCount--;
                ++braceEnd;
            }
            if (braceCount != 0) return shaderCode;

            // Extrair função inteira
            size_t funcEnd = braceEnd;
            std::string fullFunc = shaderCode.substr(funcStart, funcEnd - funcStart);

            // Substituir nome da função se necessário
            if (!replaceFunctionName.empty()) {
                size_t namePos = fullFunc.find(functionName);
                if (namePos != std::string::npos)
                    fullFunc.replace(namePos, functionName.length(), replaceFunctionName);
            }

            // Construir nome da diretiva
            std::string defineName = directiveName.empty() ? functionName : directiveName;

            // Construir função com diretiva
            std::string wrappedFunc =
                "#if defined(" + defineName + ")\n" +
                fullFunc + "\n" +
                "#endif\n";

            // Substituir no código
            return shaderCode.substr(0, funcStart) + wrappedFunc + shaderCode.substr(funcEnd);
        }
        ++pos;
    }

    return shaderCode;
}

std::string ConvertBlockToIfDef(const std::string& code, const std::string& blockName, const std::string& ifdefName = "") {
    size_t pos = 0;

    while (pos < code.size()) {
        // Skip whitespace
        while (pos < code.size() && std::isspace(code[pos])) pos++;

        size_t namePos = pos;
        if (code.compare(pos, blockName.length(), blockName) != 0) {
            pos++;
            continue;
        }

        pos += blockName.length();

        // Skip whitespace
        while (pos < code.size() && std::isspace(code[pos])) pos++;

        // Must be followed by {
        if (code[pos] != '{') continue;
        size_t braceStart = pos;

        // Find matching closing brace
        int braceCount = 1;
        size_t braceEnd = braceStart + 1;
        while (braceEnd < code.size() && braceCount > 0) {
            if (code[braceEnd] == '{') braceCount++;
            else if (code[braceEnd] == '}') braceCount--;
            ++braceEnd;
        }

        if (braceCount != 0) return code; // malformed

        std::string inner = code.substr(braceStart + 1, braceEnd - braceStart - 2);

        std::string macroName = ifdefName.empty() ? blockName : ifdefName;

        // Convert macro to upper case (optional trimming logic)
        for (char& c : macroName) c = std::toupper(c);

        // Build replacement
        std::string wrapped =
            "#if defined(" + macroName + ")\n" +
            inner + "\n" +
            "#endif";

        // Replace in code
        return code.substr(0, namePos) + wrapped + code.substr(braceEnd);
    }

    return code;
}

std::string readFileToString(const std::string& path) {
    std::ifstream file(path, std::ios::in | std::ios::binary);
    if (!file) {
        throw std::runtime_error("Failed to open file: " + path);
    }

    std::ostringstream ss;
    ss << file.rdbuf(); // Read the whole file into the stream buffer
    return ss.str();
}

std::string ConvertSequentialBlocksToIfDefs(const std::string& code, const std::string& blockName = "Pass") {
    std::string output;
    size_t pos = 0;
    size_t lastPos = 0;
    int blockIndex = 0;

    while (pos < code.size()) {
        // Localiza o nome do bloco
        size_t namePos = code.find(blockName, pos);
        if (namePos == std::string::npos) break;

        // Verifica se é seguido de '{'
        size_t bracePos = namePos + blockName.length();
        while (bracePos < code.size() && std::isspace(code[bracePos])) ++bracePos;
        if (bracePos >= code.size() || code[bracePos] != '{') {
            pos = bracePos;
            continue;
        }

        // Localiza o final do bloco com contagem de chaves
        size_t braceStart = bracePos;
        size_t braceEnd = braceStart + 1;
        int braceCount = 1;
        while (braceEnd < code.size() && braceCount > 0) {
            if (code[braceEnd] == '{') braceCount++;
            else if (code[braceEnd] == '}') braceCount--;
            ++braceEnd;
        }
        if (braceCount != 0) break;

        // Extrai conteúdo anterior ao bloco atual
        output += code.substr(lastPos, namePos - lastPos);

        // Extrai conteúdo do bloco (sem as chaves externas)
        std::string inner = code.substr(braceStart + 1, braceEnd - braceStart - 2);

        // Adiciona bloco com diretiva condicional
        output += "#if defined(" + blockName + "_" + std::to_string(blockIndex) + ")\n";
        output += inner + "\n";
        output += "#endif\n";

        // Avança
        pos = lastPos = braceEnd;
        blockIndex++;
    }

    // Adiciona o restante do código que sobrou
    output += code.substr(lastPos);
    return output;
}

inline std::string trim(const std::string& s) {
    size_t start = s.find_first_not_of(" \t\r\n");
    size_t end = s.find_last_not_of(" \t\r\n");

    if (start == std::string::npos || end == std::string::npos)
        return "";

    return s.substr(start, end - start + 1);
}

std::string ConvertBlockToUniformFormat(
    const std::string& input,
    const std::string& blockName,       // ex: "MaterialData"
    const std::string& beginUniform,    // ex: "BeginUniform(0, 0, Main)"
    const std::string& endUniform       // ex: "EndUniform()"
) {
    std::string output;
    size_t pos = 0;

    while (pos < input.size()) {
        size_t startPos = input.find(blockName, pos);
        if (startPos == std::string::npos) {
            output += input.substr(pos);
            break;
        }

        output += input.substr(pos, startPos - pos);

        size_t bracePos = input.find('{', startPos + blockName.size());
        if (bracePos == std::string::npos) {
            output += input.substr(startPos);
            break;
        }

        size_t braceEnd = bracePos + 1;
        int braceCount = 1;
        while (braceEnd < input.size() && braceCount > 0) {
            if (input[braceEnd] == '{') braceCount++;
            else if (input[braceEnd] == '}') braceCount--;
            braceEnd++;
        }
        if (braceCount != 0) {
            output += input.substr(startPos);
            break;
        }

        std::string inner = input.substr(bracePos + 1, braceEnd - bracePos - 2);

        output += beginUniform + "\n";

        std::stringstream ss(inner);
        std::string line;
        while (std::getline(ss, line)) {
            std::string trimmedLine = trim(line);
            if (!trimmedLine.empty()) {
                std::stringstream lineStream(trimmedLine);
                std::string token;
                while (std::getline(lineStream, token, ';')) {
                    std::string decl = trim(token);
                    if (!decl.empty()) {
                        output += "    Uniform " + decl + ";\n";
                    }
                }
            }
        }

        output += endUniform + "\n";

        pos = braceEnd;
    }

    return output;
}

std::string ConvertSequentialBlocksToIfDefs(const std::string& input, std::vector<ShaderPassData>& outPasses) {
    std::stringstream in(input);
    std::string line;
    std::string output;
    int passIndex = 0;
    bool inPass = false;
    int braceDepth = 0;
    ShaderPassData currentPass;
    std::string currentPassContent;

    while (std::getline(in, line)) {
        std::string trimmed = trim(line);

        // Detecta início do bloco Pass, aceitando espaço opcional antes da chave
        if (!inPass) {
            if (trimmed.size() >= 4 && trimmed.compare(0, 4, "Pass") == 0) {
                size_t posBrace = trimmed.find('{', 4);
                if (posBrace != std::string::npos) {
                    inPass = true;
                    braceDepth = 1;
                    currentPass = ShaderPassData();
                    currentPassContent.clear();

                    output += "#if defined(Pass_" + std::to_string(passIndex) + ")\n";

                    // Extrai o que vem depois da primeira chave '{' na mesma linha, se houver
                    std::string afterBrace = trimmed.substr(posBrace + 1);
                    if (!afterBrace.empty()) {
                        currentPassContent += afterBrace + "\n";
                    }
                    continue;
                }
            }
        } else {
            // Estamos dentro do bloco Pass

            // Atualiza contagem de chaves no texto da linha
            for (char c : line) {
                if (c == '{') braceDepth++;
                else if (c == '}') braceDepth--;
            }

            // Trata linhas que começam com #pragma para propriedades (mesmo código seu)
            std::string trimmedLine = trim(line);
            if (trimmedLine.rfind("#pragma", 0) == 0) {
                std::istringstream iss(trimmedLine);
                std::string token;
                std::vector<std::string> pragmaTokens;

                iss >> token; // descarta "#pragma"
                while (iss >> token) {
                    pragmaTokens.push_back(token);
                }

                if (!pragmaTokens.empty()) {
                    currentPass.properties.push_back(pragmaTokens);
                }
            }

            // Adiciona linha ao conteúdo temporário do Pass, **sem** a última chave `}`
            // Para isso, remove todas as chaves fechando no final, só se braceDepth == 0 após ler
            if (braceDepth > 0) {
                currentPassContent += line + "\n";
            } else {
                // Se braceDepth zerou, precisa remover a última chave da linha antes de adicionar
                size_t lastBracePos = line.find_last_of('}');
                if (lastBracePos != std::string::npos) {
                    // Adiciona a parte antes da última chave
                    currentPassContent += line.substr(0, lastBracePos) + "\n";
                }
            }

            if (braceDepth == 0) {
                inPass = false;
                outPasses.push_back(currentPass);
                output += currentPassContent;
                output += "#endif\n";
                //output += "//__END_PASS__\n";
                //output += "#endif // PASS_" + std::to_string(passIndex) + "\n\n";

                passIndex++;
            }
            continue;
        }

        // Fora do bloco Pass, adiciona linha normal
        output += line + "\n";
    }

    return output;
}

std::string _Load(const std::string& path, ShaderSourceData& out) {
    static bool isRecursiveCall = false;

    std::string shaderCode = readFileToString(path);
    if (shaderCode.empty()) {
        std::cerr << "ERROR: could not open or empty shader file: " << path << "\n";
        return "";
    }

    // Etapa 1: converte blocos "Pass { ... }" sequenciais para #if defined(Pass_X)
    std::string modified = ConvertSequentialBlocksToIfDefs(shaderCode, out.passes);

    // Etapa 2: envolve as funções vertex() e fragment() com #if defined(...)
    modified = WrapFunctionWithIfDef2(modified, "vertex", "void", true, "VERTEX", "main");
    modified = WrapFunctionWithIfDef2(modified, "fragment", "void", true, "FRAGMENT", "main");

    modified = ConvertBlockToUniformFormat(modified, "MaterialData", "BeginUniform(0, 0, Main)", "EndUniform()");

    // Etapa 3: converte os blocos VertexInOut e FragInOut para #if defined(...)
    modified = ConvertBlockToIfDef(modified, "VertexInOut", "VERTEX");
    modified = ConvertBlockToIfDef(modified, "FragInOut", "FRAGMENT");

    // Etapa 4: resolve includes recursivos e trata marcadores especiais + Properties block
    std::stringstream ss(modified);
    std::string fullSourceCode;
    std::string lineBuffer;

    bool insidePropertiesBlock = false;  // flag para "Properties { ... }"

    while (std::getline(ss, lineBuffer)) {
        const std::string includeIdentifier = "#include ";
        size_t includePos = lineBuffer.find(includeIdentifier);

        // --- Suporte ao bloco Properties { ... } ---
        if (!insidePropertiesBlock) {
            // Detecta início do bloco Properties {
            size_t pos = lineBuffer.find("Properties");
            if (pos != std::string::npos) {
                size_t bracePos = lineBuffer.find("{", pos);
                if (bracePos != std::string::npos) {
                    insidePropertiesBlock = true;
                    // Ignora a linha que contém "Properties {"
                    continue;
                }
            }
        } else {
            // Estamos dentro do bloco Properties {...}
            // Verifica fechamento do bloco
            if (lineBuffer.find("}") != std::string::npos) {
                insidePropertiesBlock = false;
                continue;  // Ignora a linha com "}"
            }

            // Parseia a linha e adiciona a out.properties
            std::vector<std::string> tokens;
            std::stringstream lineSS(lineBuffer);
            std::string token;
            while (lineSS >> token) {
                tokens.push_back(token);
            }
            if (!tokens.empty()) {
                out.properties.push_back(tokens);
            }

            // Não adiciona a linha ao código fonte, só extrai propriedades
            continue;
        }

        // --- Continua código original ---

        // Etapa 4.1: processa includes recursivos
        if (includePos != std::string::npos) {
            std::string rawInclude = lineBuffer.substr(includePos + includeIdentifier.length());
            rawInclude.erase(std::remove(rawInclude.begin(), rawInclude.end(), ' '), rawInclude.end());
            rawInclude.erase(std::remove(rawInclude.begin(), rawInclude.end(), '\"'), rawInclude.end());
            if (!rawInclude.empty() && rawInclude.back() == '\r') rawInclude.pop_back();

            std::string includePath;
            if (rawInclude.find("Engine/") == 0 || rawInclude.find("Shaders/") == 0 || rawInclude[0] == '/') {
                includePath = rawInclude;
            } else {
                std::string pathOfThisFile;
                getFilePath(path, pathOfThisFile);
                includePath = pathOfThisFile + rawInclude;
            }

            isRecursiveCall = true;
            fullSourceCode += _Load(includePath, out);
            continue;
        }

        // Etapa 4.2: verifica se chegou ao final de um bloco Pass
        if (trim(lineBuffer) == "//__END_PASS__") {
            /*if (!out.passes.empty()) {
                out.passes.back().UpdateProperties();
            }*/
            continue;
        }

        // Etapa 4.3: adiciona a linha normalmente
        fullSourceCode += lineBuffer + "\n";
    }

    if (!isRecursiveCall) fullSourceCode += '\0';
    return fullSourceCode;
}

}

bool ShaderLoadFile(const std::string& path, ShaderSourceData& out){
    auto getFileExtension = [](const std::string& path) -> std::string {
        size_t dotPos = path.rfind('.');
        if (dotPos == std::string::npos) return ""; // No extension found
        return path.substr(dotPos);
    };

    auto extension = getFileExtension(path);
    if(extension == ".shader"){
        using namespace Experimental;

        /*auto shaderCode = readFileToString(path);
        std::string modified = ConvertSequentialBlocksToIfDefs(shaderCode);
        modified = WrapFunctionWithIfDef2(modified, "vertex", "void", true, "VERTEX", "main");
        modified = WrapFunctionWithIfDef2(modified, "fragment", "void", true, "FRAGMENT", "main");
        modified = ConvertBlockToIfDef(modified, "FragInOut", "FRAGMENT");
        modified = ConvertBlockToIfDef(modified, "VertexInOut", "VERTEX");
        LogInfo("%s", modified.c_str());*/

        out.baseSource = _Load(path, out);
        for(auto& i: out.passes) i.UpdateProperties();
        
        //LogInfo("%s", out.baseSource.c_str());
        return true;
    }

    out.baseSource = _load(path, out);
    return true;
}

}