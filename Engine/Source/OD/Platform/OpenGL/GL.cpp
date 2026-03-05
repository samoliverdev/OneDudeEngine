#ifdef OPENGL_SUPPORT
#include "OD/pch.h"
#include "GL.h"
#include "OD/Defines.h"
#include "OD/Base.h"
#include "OD/Core/Log.h"

#ifdef __EMSCRIPTEN__
int glCheckError_(const char *file, int line, std::function<void()> callback){
    return 1;
}
#else

bool IsExtensionSupported(const char* ext)
{
    GLint n;
    glGetIntegerv(GL_NUM_EXTENSIONS, &n);
    for (GLint i = 0; i < n; i++)
    {
        const char* e = (const char*)glGetStringi(GL_EXTENSIONS, i);
        if (strcmp(e, ext) == 0) return true;
    }
    return false;
}

#define GL_TEXTURE_FREE_MEMORY_ATI 0x87FC

void CheckVRAM_AMD()
{
    if (!IsExtensionSupported("GL_ATI_meminfo"))
        return; // Extension not available

    GLint memInfo[4] = { 0 };
    glGetIntegerv(GL_TEXTURE_FREE_MEMORY_ATI, memInfo);

    GLint freeKB = memInfo[0];
    static GLint previousFreeKB = 0;

    if (previousFreeKB != 0)
    {
        GLint usedIncrease = std::max(0, previousFreeKB - freeKB);
        if (usedIncrease > 4000 * 1024) // 1000 MB
        {
            //LogFatal("VRAM increased by >1000MB! {} MB", usedIncrease / 1024);
            //Assert(false);
        }
    }

    previousFreeKB = freeKB;
}

int glCheckError_(const char *file, int line, std::function<void()> callback){
    //CheckVRAM_AMD();

    GLenum errorCode;
    while((errorCode = glGetError()) != GL_NO_ERROR){
        std::string error = "OTHER";
        switch (errorCode){
            case GL_INVALID_ENUM:                  error = "INVALID_ENUM"; break;
            case GL_INVALID_VALUE:                 error = "INVALID_VALUE"; break;
            //case GL_INVALID_OPERATION:             error = "INVALID_OPERATION"; break;
            //case GL_STACK_OVERFLOW:                error = "STACK_OVERFLOW"; break;
            //case GL_STACK_UNDERFLOW:               error = "STACK_UNDERFLOW"; break;
            case GL_OUT_OF_MEMORY:                 error = "OUT_OF_MEMORY"; break;
            case GL_INVALID_FRAMEBUFFER_OPERATION: error = "INVALID_FRAMEBUFFER_OPERATION"; break;
        }

        //std::cout << error << " | " << file << " (" << line << ")" << std::endl;
        LogFatal("OpenGL:ERROR: {}({}) | {} ({})\n", error.c_str(), errorCode, file, line);
        //Assert(false);
        if(callback != nullptr) callback();

        Assert(false);
    }

    return errorCode;
}
#endif

#if defined(OpenGL46)
    //#include <glad46Core/glad.c>
    #include <glad.c>
#elif defined(OpenGL33)
    #include <glad33Core/glad.c>
#endif



#endif