#include "Shader.h"
#include "OD/Defines.h"
#include "OD/Platform/GL.h"
#include <string.h>
#include <magic_enum/magic_enum.hpp>

namespace OD{

//int curBindShaderRenderId = 0;

void getFilePath(const std::string& fullPath, std::string& pathWithoutFileName){
    // Remove the file name and store the path to this folder
    size_t found = fullPath.find_last_of("/\\");
    pathWithoutFileName = fullPath.substr(0, found + 1);
}

bool Shader::Create(const std::string& filepath, std::vector<std::string>& keyworlds){
    Destroy(*this);

    enabledKeyworlds = keyworlds;

    this->path = filepath;

    std::string source = this->load(filepath);
    auto shaderSources = this->PreProcess(source, keyworlds);
    this->Compile(shaderSources);

    // Extract name from filepath
    auto lastSlash = filepath.find_last_of("/\\");
    lastSlash = lastSlash == std::string::npos ? 0 : lastSlash + 1;
    auto lastDot = filepath.rfind('.');
    auto count = lastDot == std::string::npos ? filepath.size() - lastSlash : lastDot - lastSlash;
    //out->name = filepath.substr(lastSlash, count);

    
    return true;
}

std::string Shader::load(std::string path){
    //LogWarning("Path: %s", path.c_str());

    std::string includeIndentifier = "#include ";
    //includeIndentifier += ' ';
    static bool isRecursiveCall = false;

    std::string fullSourceCode = "";
    std::ifstream file(path);

    if(!file.is_open()){
        std::cerr << "ERROR: could not open the shader at: " << path << "\n" << std::endl;
        return fullSourceCode;
    }

    bool beginProperties = false;

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

        pragmas.push_back(pragmaLine);

        /*for(auto i: pragmaLine){
            LogInfo("===> %s", i.c_str());
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
            supportInstancing = true;
        }

        if(pragmaLine.size() > 1 && pragmaLine[0] == "Blend" && pragmaLine[1] != "Off"){
            Assert(pragmaLine.size() == 3);

            auto value1 = magic_enum::enum_cast<BlendMode>(pragmaLine[1]);
            auto value2 = magic_enum::enum_cast<BlendMode>(pragmaLine[2]);

            if(value1.has_value() && value2.has_value()){
                blend = true;
                srcBlend = value1.value();
                dstBlend = value2.value();
            }
        }

        if(pragmaLine.size() > 1 && pragmaLine[0] == "CullFace"){
            Assert(pragmaLine.size() == 2);
            auto value1 = magic_enum::enum_cast<CullFace>(pragmaLine[1]);
            if(value1.has_value()){
                cullFace = value1.value();
            }
        }

        if(pragmaLine.size() > 1 && pragmaLine[0] == "DepthTest"){
            Assert(pragmaLine.size() == 2);

            auto value1 = magic_enum::enum_cast<DepthTest>(pragmaLine[1]);
            if(value1.has_value()){
                depthTest = value1.value(); 
            }
        }

        if(pragmaLine.size() > 1 && pragmaLine[0] == "DepthMask"){
            Assert(pragmaLine.size() == 2);
            if(pragmaLine[1] == "true"){
                depthMask = true;
            }
            if(pragmaLine[1] == "false"){
                depthMask = false;
            }
        }

        if(beginProperties){
            pragmaLine.clear();
            while(ss >> _out){
                pragmaLine.push_back(_out);
                index += 1;
            }
            properties.push_back(pragmaLine);

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

            //Remove "\r"
            //lineBuffer.erase(std::remove(lineBuffer.begin(), lineBuffer.end(), "\r"), lineBuffer.cend());
            if(lineBuffer[lineBuffer.size()-1] == '\r')
                lineBuffer.erase(lineBuffer.length()-1);

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
    if(!isRecursiveCall)
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
    std::vector<std::string> keyworlds;

    Ref<Shader> out = CreateRef<Shader>();
    if(out->Create(filepath, keyworlds) == false){
        return nullptr;
    }
    return out;

    /*
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

    out->path = filepath;
    return out;
    */
}

Ref<Shader> Shader::CreateFromFile(const std::string& filepath, std::vector<std::string>& keyworlds){
    Ref<Shader> out = CreateRef<Shader>();
    if(out->Create(filepath, keyworlds) == false){
        return nullptr;
    }
    return out;
}

void Shader::Bind(Shader& shader){
    //curBindShaderRenderId = shader.rendererId;

    glUseProgram(shader.rendererId);
    glCheckError();
}

void Shader::Unbind(){
    //curBindShaderRenderId = 0;

    glUseProgram(0);
    glCheckError();
}

void Shader::Destroy(Shader& shader){
    if(shader.IsValid() == false) return;

    glDeleteProgram(shader.rendererId);
    glCheckError();
    shader.rendererId = 0;
}

Shader::~Shader(){
    Destroy(*this);
}

std::unordered_map<GLenum, std::string> Shader::PreProcess(const std::string& source, std::vector<std::string>& keyworlds){
    std::unordered_map<GLenum, std::string> shaderSources;

    std::string _source = source;
    
    std::string toSeach = "#version 330 core\n";
    size_t pos = _source.find(toSeach) + toSeach.size();

    for(auto i: keyworlds){
        std::string keyworld = "#define " + i + "\n";
        _source.insert(pos, keyworld);
    }

    if(_source.find("VERTEX") != std::string::npos){
        std::string ss = _source;
        ss.insert(pos, "\n#define VERTEX\n");
        shaderSources[GL_VERTEX_SHADER] = ss;
    }  

    if(_source.find("FRAGMENT") != std::string::npos){
        std::string ss = _source;
        ss.insert(pos, "\n#define FRAGMENT\n");
        shaderSources[GL_FRAGMENT_SHADER] = ss;
    }   

    if(_source.find("GEOMETRY") != std::string::npos){
        std::string ss = _source;
        ss.insert(pos, "\n#define GEOMETRY\n");
        shaderSources[GL_GEOMETRY_SHADER] = ss;
    }

    //LogInfo("%s", _source.c_str());

    return shaderSources;
}

void Shader::Compile(const std::unordered_map<GLenum, std::string>& shaderSources){
    Assert(shaderSources.size() <= 3 && "We only support 3 shaders for now");

    GLuint program = glCreateProgram();
    glCheckError();
    
    GLenum glShaderIDs[3] = {0, 0, 0};
    int glShaderIDIndex = 0;
    for(auto& kv : shaderSources){
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
            glCheckError();

            std::vector<GLchar> infoLog(maxLength);
            glGetShaderInfoLog(shader, maxLength, &maxLength, &infoLog[0]);
            glCheckError();

            glDeleteShader(shader);
            glCheckError();

            printf("%s", infoLog.data());
            //Assert(false && "Shader compilation failure!");
            LogError("Shader compilation failure!");
            break;
        }

        glAttachShader(program, shader);
        glCheckError();
        glShaderIDs[glShaderIDIndex++] = shader;
    }
    
    rendererId = program;

    // Link our program
    glLinkProgram(program);
    glCheckError();

    // Note the different functions here: glGetProgram* instead of glGetShader*.
    GLint isLinked = 0;
    glGetProgramiv(program, GL_LINK_STATUS, (int*)&isLinked);
    glCheckError();

    if (isLinked == GL_FALSE){
        GLint maxLength = 0;
        glGetProgramiv(program, GL_INFO_LOG_LENGTH, &maxLength);
        glCheckError();

        // The maxLength includes the NULL character
        std::vector<GLchar> infoLog(maxLength);
        glGetProgramInfoLog(program, maxLength, &maxLength, &infoLog[0]);
        glCheckError();

        // We don't need the program anymore.
        glDeleteProgram(program);
        glCheckError();
        
        for(auto id : glShaderIDs){
            glDeleteShader(id);
            glCheckError();
        }

        printf("%s", infoLog.data());
        LogError("Shader link failure! %s", path.c_str());
        Assert(false && "Shader link failure!");
        return;
    }

    for(auto id: glShaderIDs){
        if(id == 0) continue;
        glDetachShader(program, id);
        glCheckError();
    }

    glCheckError();

    GLint count;
    GLint size; // size of the variable
    GLenum type; // type of the variable (float, vec3 or mat4, etc)
    const GLsizei bufSize = 64; // maximum name length
    GLchar name[bufSize]; // variable name in GLSL
    GLsizei length; // name length
    glGetProgramiv(rendererId, GL_ACTIVE_UNIFORMS, &count);
    //printf("Active Uniforms: %d\n", count);
    for(int i = 0; i < count; i++){
        glGetActiveUniform(rendererId, (GLuint)i, bufSize, &length, &size, &type, name);
        
        std::string t = std::string(name);
        std::string s = "[0]";
        std::string::size_type _i = t.find(s);
        if (_i != std::string::npos)
        t.erase(_i, s.length());

        _uniforms.push_back(std::string(t));
        //printf("Uniform #%d Type: %u Name: %s\n", i, type, name);
    }
}

bool Shader::IsValid(){
    return rendererId != 0;
}

void Shader::Reload(){
    Create(Path(), enabledKeyworlds);
}

GLint Shader::GetLocation(const char* name){
    if(uniforms.count(name)) return uniforms[name];
    
    GLint location = glGetUniformLocation(rendererId, name);
    glCheckError();
    
    uniforms[name] = location;
    return location;
}

void Shader::SetFloat(const char* name, float value){
    //if(curBindShaderRenderId != rendererId) Bind(*this);

    glUniform1f(GetLocation(name), value);
    glCheckError2([&](){ 
        LogError("UniformName: %s ShaderPath: %s", name, path.c_str()); 
    });
}

void Shader::SetFloat(const char* name, float* value, int count){
    //if(curBindShaderRenderId != rendererId) Bind(*this);

    //glUniform1f(GetLocation(name), value);
    glUniform1fv(GetLocation(name), (GLsizei)count, (GLfloat*)value);
    glCheckError();
}

void Shader::SetInt(const char* name, int value){
    //if(curBindShaderRenderId != rendererId) Bind(*this);

    glUniform1i(GetLocation(name), value);
    //glCheckError();
    glCheckError2([&](){ 
        LogError("UniformName: %s ShaderPath: %s", name, path.c_str()); 
    });
}

void Shader::SetVector2(const char* name, Vector2 value){
    //if(curBindShaderRenderId != rendererId) Bind(*this);

    glUniform2f(GetLocation(name), value.x, value.y);
    glCheckError();
}

void Shader::SetVector3(const char* name, Vector3 value){
    //if(curBindShaderRenderId != rendererId) Bind(*this);

    glUniform3f(GetLocation(name), value.x, value.y, value.z);
    //glCheckError();
    glCheckError2([&](){ 
        LogError("UniformName: %s ShaderPath: %s", name, path.c_str()); 
    });
}

void Shader::SetVector4(const char* name, Vector4 value){
    //if(curBindShaderRenderId != rendererId) Bind(*this);

    glUniform4f(GetLocation(name), value.x, value.y, value.z, value.w);
    glCheckError();
}

void Shader::SetVector4(const char* name, Vector4* value, int count){
    //if(curBindShaderRenderId != rendererId) Bind(*this);

    glUniform4fv(GetLocation(name), (GLsizei)count, (GLfloat*)value);
    glCheckError();
}

void Shader::SetMatrix4(const char* name, Matrix4 value){
    //if(curBindShaderRenderId != rendererId) Bind(*this);

    glUniformMatrix4fv(GetLocation(name), 1, GL_FALSE, glm::value_ptr(static_cast<glm::mat4>(value)));
    //glCheckError();
    glCheckError2([&](){ 
        LogError("UniformName: %s ShaderPath: %s", name, path.c_str()); 
    });
}

void Set(unsigned int slot, Matrix4* inputArray, unsigned int arrayLength) {
	glUniformMatrix4fv(slot, (GLsizei)arrayLength, false, (float*)&inputArray[0]);
}

void Shader::SetMatrix4(const char* name, std::vector<Matrix4>& value){
    //if(curBindShaderRenderId != rendererId) Bind(*this);

    glUniformMatrix4fv(GetLocation(name), (GLsizei)value.size(), GL_FALSE, glm::value_ptr(value[0]));
    //Set(GetLocation(name), &value[0], (unsigned int)value.size());
    //glCheckError();
    glCheckError2([&](){ 
        LogError("UniformName: %s ShaderPath: %s Cout: %zd", name, path.c_str(), value.size()); 
    });
}

void Shader::SetMatrix4(const char* name, Matrix4* value, int count){
    //if(curBindShaderRenderId != rendererId) Bind(*this);

    glUniformMatrix4fv(GetLocation(name), (GLsizei)count, GL_FALSE, (GLfloat*)value);
    glCheckError();
}

void Shader::SetTexture2D(const char* name, Texture2D& value, int index){
    //if(curBindShaderRenderId != rendererId) Bind(*this);

    Texture2D::Bind(value, index);
    SetInt(name, index);
}

void Shader::SetCubemap(const char* name, Cubemap& value, int index){
    //if(curBindShaderRenderId != rendererId) Bind(*this);

    Cubemap::Bind(value, index);
    SetInt(name, index);
}

void Shader::SetUniforBuffer(const char* name, UniformBuffer& buffer, int index){
    //if(curBindShaderRenderId != rendererId) Bind(*this);

    UniformBuffer::Bind(buffer, index);
    unsigned int bufferIndex = glGetUniformBlockIndex(rendererId, name);   
    glUniformBlockBinding(rendererId, bufferIndex, index);
    glCheckError();
}

void Shader::SetFramebuffer(const char* name, Framebuffer& framebuffer, int index, int colorAttachmentIndex){
    //if(curBindShaderRenderId != rendererId) Bind(*this);
    
    Assert(framebuffer.Specification().type != FramebufferAttachmentType::TEXTURE_2D_MULTISAMPLE);
    //Assert(framebuffer.specification().sample <= 1);

    glActiveTexture(GL_TEXTURE0 + index);
    glCheckError();

    unsigned int target = GL_TEXTURE_2D;
    if(framebuffer.Specification().type == FramebufferAttachmentType::TEXTURE_2D_ARRAY)
        target = GL_TEXTURE_2D_ARRAY;

    if(colorAttachmentIndex == -1){
        glBindTexture(target, framebuffer.DepthAttachmentId());
        glCheckError();
    } else {
        glBindTexture(target, framebuffer.ColorAttachmentId(colorAttachmentIndex));
        glCheckError();
    }

    glCheckError();
    SetInt(name, index);
}

}