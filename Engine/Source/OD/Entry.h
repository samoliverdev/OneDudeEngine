#ifdef _WIN32
    //#define _CRTDBG_MAP_ALLOC //#define _CRTDBG_MAP_ALLOC_NEW
    //#include <crtdbg.h>
#endif

#include "OD/Core/Log.h"
#include "OD/Core/Module.h"
#include "OD/Core/Application.h"
#include "OD/Core/Instrumentor.h"
#include "OD/Core/Asset.h"
#include <stdio.h>
#include <stdlib.h>
#include <filesystem>
#include <iostream>
#include "CoreModulesStartup.h"

/*void* operator new(size_t size){
    LogInfo("Alloc: %zd", size);
    return malloc(size);
}
void operator delete(void* data){
    LogInfo("Dealloc");
    free(data);
}*/

extern OD::ApplicationConfig GetStartAppConfig();
extern OD::Module* CreateMainModule();

#ifdef DEFINE_CUSTOM_PROJECT
extern OD::Ref<OD::Project> CreateCustomProject();
#else 
OD::Ref<OD::Project> CreateCustomProject(){
    return nullptr;
}
#endif

/*#include <FileWatch.hpp>
#include <efsw/efsw.hpp>*/

#define ENABLE_MEMORT_TRACKER 0

#if ENABLE_MEMORT_TRACKER
#include <crtdbg.h>

constexpr size_t operator"" _MB(unsigned long long v) { return v * 1024ULL * 1024ULL; }

static std::atomic<size_t> g_CurrentMemory = 0;
static thread_local bool g_InHook = false;

int AllocHook(
    int allocType,
    void* userData,
    size_t size,
    int blockType,
    long requestNumber,
    const unsigned char* filename,
    int lineNumber)
{
    //if (g_InHook) return 1;
    //g_InHook = true;

    if (allocType == _HOOK_ALLOC)
    {
        g_CurrentMemory.fetch_add(size);

        if (size > 500_MB) {
            __debugbreak();
        }
    }
    else if (allocType == _HOOK_FREE)
    {
        if (userData)
        {
            size_t realSize = _msize(userData);
            g_CurrentMemory.fetch_sub(realSize);
        }
    }
    else if (allocType == _HOOK_REALLOC)
    {
        if (userData)
        {
            size_t oldSize = _msize(userData);
            g_CurrentMemory.fetch_sub(oldSize);
        }

        g_CurrentMemory.fetch_add(size);
    }

    size_t current = g_CurrentMemory.load();
    if (current > 1250_MB)
        __debugbreak();

    //g_InHook = false;
    return 1;
}
#endif

int main(int argc, char *argv[]){
    //int* a = new int();

    #ifdef _WIN32
        //_CrtSetDbgFlag(_CRTDBG_ALLOC_MEM_DF | _CRTDBG_LEAK_CHECK_DF);
    #endif

    #if ENABLE_MEMORT_TRACKER
    _CrtSetAllocHook(AllocHook);
    #endif

    for(int i = 0; i < argc; i++){
        OD::Application::GetArgs().push_back(std::string(argv[i]));
    }

    OD::ApplicationCallbacks callbacks = {
        [](){ OD::CoreModulesInit(); },
        [](){ OD::CoreModulesShutdown(); }
    };

    if(!OD::Application::Create(CreateMainModule(), GetStartAppConfig(), argc > 1 ? argv[1] : RESOURCES_PATH "", callbacks, CreateCustomProject())){
        printf("Application failed to create!");
        return 1;
    }

    /*filewatch::FileWatch<std::string> watch {
        "C:/Users/sam/Desktop/cpp/OneDudeEngine/Sandbox/Content",
        [&](const std::string& path, const filewatch::Event event){
            std::cout << path << ' ' << filewatch::event_to_string(event) << '\n';
            //LogInfo("dsdss fdff dsdsdsdd");
        }
    };*/

    /*class UpdateListener : public efsw::FileWatchListener {
    public:
        void handleFileAction( efsw::WatchID watchid, const std::string& dir,
                                const std::string& filename, efsw::Action action,
                                std::string oldFilename ) override {
            switch ( action ) {
                case efsw::Actions::Add:
                    std::cout << "DIR (" << dir << ") FILE (" << filename << ") has event Added"
                            << std::endl;
                    break;
                case efsw::Actions::Delete:
                    std::cout << "DIR (" << dir << ") FILE (" << filename << ") has event Delete"
                            << std::endl;
                    break;
                case efsw::Actions::Modified:
                    std::cout << "DIR (" << dir << ") FILE (" << filename << ") has event Modified"
                            << std::endl;
                    break;
                case efsw::Actions::Moved:
                    std::cout << "DIR (" << dir << ") FILE (" << filename << ") has event Moved from ("
                            << oldFilename << ")" << std::endl;
                    break;
                default:
                    std::cout << "Should never happen!" << std::endl;
            }
        }
    };

    efsw::FileWatcher* fileWatcher = new efsw::FileWatcher();
    UpdateListener* listener = new UpdateListener();
    efsw::WatchID watchID = fileWatcher->addWatch("C:/Users/sam/Desktop/cpp/OneDudeEngine/Sandbox/Content", listener, true );
    fileWatcher->watch();*/
    
    OD::Application::Run();

    /*delete listener;
    delete fileWatcher;*/

    #ifdef _WIN32 
        //_CrtDumpMemoryLeaks();
    #endif

    return 0;
}
