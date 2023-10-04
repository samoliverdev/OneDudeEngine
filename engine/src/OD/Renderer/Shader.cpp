#include "Shader.h"
#include "OD/Defines.h"
#include "OD/Platform/GL.h"
#include <string.h>

namespace OD{

void getFilePath(const std::string & fullPath, std::string & pathWithoutFileName){
    // Remove the file name and store the path to this folder
    size_t found = fullPath.find_last_of("/\\");
    pathWithoutFileName = fullPath.substr(0, found + 1);
}

std::string Shader::load(std::string path){
    std::string includeIndentifier = "#include ";
    //includeIndentifier += ' ';
    static bool isRecursiveCall = false;

    std::string fullSourceCode = "";
    std::ifstream file(path);

    if (!file.is_open()){
        std::cerr << "ERROR: could not open the shader at: " << path << "\n" << std::endl;
        return fullSourceCode;
    }

    bool beginProperties = false;

    std::string lineBuffer;
    while (std::getline(file, lineBuffer)){
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

        /*for(auto i: pragmaLine){
            LogInfo("%s", i.c_str());
        }*/

        if(beginProperties == false && pragmaLine.size() > 0 && pragmaLine[0] == "BeginProperties"){
            beginProperties = true;
            continue;
        }
        if(beginProperties == true && pragmaLine.size() > 0 && pragmaLine[0] == "EndProperties"){
            beginProperties = false;
            continue;
        }

        if(pragmaLine.size() > 1 && pragmaLine[0] == "SupportInstancing" && pragmaLine[1] == "true"){
            _supportInstancing = true;
        }

        if(pragmaLine.size() > 1 && pragmaLine[0] == "BlendMode" && pragmaLine[1] == "Blend"){
            _blendMode = BlendMode::Blend;
        }

        if(beginProperties){
            pragmaLine.clear();
            while(ss >> _out){
                pragmaLine.push_back(_out);
                index += 1;
            }
            _properties.push_back(pragmaLine);

            //LogInfo("Propertie: %s, %s", pragmaLine[0].c_str(), pragmaLine[1].c_str());

            continue;
        }

        // Look for the new shader include identifier
        if (lineBuffer.find(includeIndentifier) != lineBuffer.npos){
            // Remove the include identifier, this will cause the path to remain
            lineBuffer.erase(0, includeIndentifier.size());

            // The include path is relative to the current shader file path
            std::string pathOfThisFile;
            getFilePath(path, pathOfThisFile);
            lineBuffer.insert(0, pathOfThisFile);

            // By using recursion, the new include file can be extracted
            // and inserted at this location in the shader source code
            isRecursiveCall = true;
            fullSourceCode += load(lineBuffer);

            // Do not add this line to the shader source code, as the include
            // path would generate a compilation issue in the final source code
            continue;
        }

        fullSourceCode += lineBuffer + '\n';
    }

    // Only add the null terminator at the end of the complete file,
    // essentially skipping recursive function calls this way
    if (!isRecursiveCall)
        fullSourceCode += '\0';

    file.close();

    /*for(auto i: _properties){
        LogInfo("Propertie: %s, %s", i[0].c_str(), i[1].c_str());
    }*/

    return fullSourceCode;
}

void GetInfo(unsigned int shaderID){
    GLint count;

    GLint size; // size of the variable
    GLenum type; // type of the variable (float, vec3 or mat4, etc)

    const GLsizei bufSize = 16; // maximum name length
    GLchar name[bufSize]; // variable name in GLSL
    GLsizei length; // name length

    printf("\n\n----------ShaderInfo----------\n");

    glGetProgramiv(shaderID, GL_ACTIVE_ATTRIBUTES, &count);
    printf("Active Attributes: %d\n", count);

    for (int i = 0; i < count; i++){
        glGetActiveAttrib(shaderID, (GLuint)i, bufSize, &length, &size, &type, name);

        printf("Attribute #%d Type: %u Name: %s\n", i, type, name);
    }

    glGetProgramiv(shaderID, GL_ACTIVE_UNIFORMS, &count);
    printf("Active Uniforms: %d\n", count);

    for (int i = 0; i < count; i++){
        glGetActiveUniform(shaderID, (GLuint)i, bufSize, &length, &size, &type, name);

        printf("Uniform #%d Type: %u Name: %s\n", i, type, name);
    }
}

GLenum ShaderTypeFromString(const std::string& type){
    if (type == "vertex")
        return GL_VERTEX_SHADER;
    if (type == "fragment" || type == "pixel")
        return GL_FRAGMENT_SHADER;

    Assert(false && "Unknown shader type!");
    return 0;
}

Ref<Shader> Shader::CreateFromFile(const std::string& filepath){
    Ref<Shader> out = CreateRef<Shader>();

    std::string source = out->load(filepath);

    auto shaderSources = out->PreProcess(source);
    out->Compile(shaderSources);

    // Extract name from filepath
    auto lastSlash = filepath.find_last_of("/\\");
    lastSlash = lastSlash == std::string::npos ? 0 : lastSlash + 1;
    auto lastDot = filepath.rfind('.');
    auto count = lastDot == std::string::npos ? filepath.size() - lastSlash : lastDot - lastSlash;
    //out->name = filepath.substr(lastSlash, count);

    out->_path = filepath;
    return out;
}

Shader::~Shader(){
    glDeleteProgram(_rendererId);
}

std::unordered_map<GLenum, std::string> Shader::PreProcess(const std::string& source){
    std::unordered_map<GLenum, std::string> shaderSources;
    
    std::string toSeach = "#version 330 core\n";
    size_t pos = source.find(toSeach) + toSeach.size();

    if(source.find("VERTEX") != std::string::npos){
        std::string ss = source;
        ss.insert(pos, "\n#define VERTEX\n");
        shaderSources[GL_VERTEX_SHADER] = ss;
    }  

    if(source.find("FRAGMENT") != std::string::npos){
        std::string ss = source;
        ss.insert(pos, "\n#define FRAGMENT\n");
        shaderSources[GL_FRAGMENT_SHADER] = ss;
    }   

    //LogInfo("%s", fs.c_str());

    return shaderSources;
}

void Shader::Compile(const std::unordered_map<GLenum, std::string>& shaderSources){
    GLuint program = glCreateProgram();
    Assert(shaderSources.size() <= 2 && "We only support 2 shaders for now");
    GLenum glShaderIDs[2];
    int glShaderIDIndex = 0;
    for (auto& kv : shaderSources){
        GLenum type = kv.first;
        const std::string& source = kv.second;

        GLuint shader = glCreateShader(type);

        const GLchar* sourceCStr = source.c_str();
        glShaderSource(shader, 1, &sourceCStr, 0);
        glCompileShader(shader);
        glCheckError();

        GLint isCompiled = 0;
        glGetShaderiv(shader, GL_COMPILE_STATUS, &isCompiled);
        if (isCompiled == GL_FALSE){
            GLint maxLength = 0;
            glGetShaderiv(shader, GL_INFO_LOG_LENGTH, &maxLength);

            std::vector<GLchar> infoLog(maxLength);
            glGetShaderInfoLog(shader, maxLength, &maxLength, &infoLog[0]);

            glDeleteShader(shader);

            printf("%s", infoLog.data());
            Assert(false && "Shader compilation failure!");
            break;
        }

        glAttachShader(program, shader);
        glShaderIDs[glShaderIDIndex++] = shader;
    }
    
    _rendererId = program;

    // Link our program
    glLinkProgram(program);
    glCheckError();

    // Note the different functions here: glGetProgram* instead of glGetShader*.
    GLint isLinked = 0;
    glGetProgramiv(program, GL_LINK_STATUS, (int*)&isLinked);
    if (isLinked == GL_FALSE){
        GLint maxLength = 0;
        glGetProgramiv(program, GL_INFO_LOG_LENGTH, &maxLength);

        // The maxLength includes the NULL character
        std::vector<GLchar> infoLog(maxLength);
        glGetProgramInfoLog(program, maxLength, &maxLength, &infoLog[0]);

        // We don't need the program anymore.
        glDeleteProgram(program);
        
        for (auto id : glShaderIDs)
            glDeleteShader(id);

        printf("%s", infoLog.data());
        Assert(false && "Shader link failure!");
        return;
    }

    for (auto id : glShaderIDs)
        glDetachShader(program, id);

    glCheckError();
}

void Shader::Bind(){
    glUseProgram(_rendererId);
}

void Shader::Unbind(){
    glUseProgram(0);
}

void Shader::Destroy(){
    if(_rendererId != 0) glDeleteProgram(_rendererId);
    _rendererId = 0;
}

bool Shader::IsValid(){
    return _rendererId != 0;
}

GLint Shader::GetLocation(const char* name){
    if(_uniforms.count(name)) return _uniforms[name];
    
    GLint location = glGetUniformLocation(_rendererId, name);
    _uniforms[name] = location;
    return location;
}

void Shader::SetFloat(const char* name, float value){
    glUniform1f(GetLocation(name), value);
    glCheckError();
}

void Shader::SetInt(const char* name, int value){
    glUniform1i(GetLocation(name), value);
    glCheckError();
}

void Shader::SetVector2(const char* name, Vector2 value){
    glUniform2f(GetLocation(name), value.x, value.y);
    glCheckError();
}

void Shader::SetVector3(const char* name, Vector3 value){
    glUniform3f(GetLocation(name), value.x, value.y, value.z);
    glCheckError();
}

void Shader::SetVector4(const char* name, Vector4 value){
    glUniform4f(GetLocation(name), value.x, value.y, value.z, value.w);
    glCheckError();
}

void Shader::SetMatrix4(const char* name, Matrix4 value){
    glUniformMatrix4fv(GetLocation(name), 1, GL_FALSE, glm::value_ptr(static_cast<glm::mat4>(value)));
    glCheckError();
}

void Shader::SetTexture2D(const char* name, Texture2D& value, int index){
    value.Bind(index);
    SetInt(name, index);
}

void Shader::SetCubemap(const char* name, Cubemap& value, int index){
    value.Bind(index);
    SetInt(name, index);
}

void Shader::SetFramebuffer(const char* name, Framebuffer& framebuffer, int index, int colorAttachmentIndex){
    glActiveTexture(GL_TEXTURE0 + index);
    if(colorAttachmentIndex == -1){
        glBindTexture(GL_TEXTURE_2D, framebuffer.DepthAttachmentId());
    } else {
        glBindTexture(GL_TEXTURE_2D, framebuffer.ColorAttachmentId(colorAttachmentIndex));
    }
    glCheckError();
    SetInt(name, index);
}

}