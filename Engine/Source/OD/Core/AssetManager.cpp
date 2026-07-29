#include "OD/pch.h"
#include "AssetManager.h"
#include <execution>
#include <filesystem>

namespace OD{

AssetManager globalAssetManager;

bool AssetTypesDB::HasAssetByExtension(std::string fileExtension){
    return assetFuncs.find(fileExtension) != assetFuncs.end();
}


AssetTypesDB& AssetTypesDB::Get(){
    static AssetTypesDB global;
    return global;
}

#ifdef USE_WEAK_PTR
std::unordered_map<std::string, WeakRef<Asset>>& AssetManager::GetDB(Type id){
    return data[id];
}
#else
std::unordered_map<std::string, Ref<Asset>>& AssetManager::GetDB(Type id){
    return data[id];
}
#endif

void AssetManager::Mount(Package* p){
    packages.push_back(p);
}

void AssetManager::UnMount(Package* p){
    if(!p) return;
    packages.erase(std::remove(packages.begin(), packages.end(), p), packages.end());
}

void AssetManager::UnloadAll(){
    //for(auto& [type, db]: data){
        /*for(auto& [path, asset]: db){
            LogWarning("Unload Asset: %s %d", asset->Path().c_str(), asset.use_count());
            Assert(asset.use_count() == 1);
            asset.reset();  // Explicitly release shared_ptr
        }*/
    //    db.clear();  // Clear inner map
    //}
    //data.clear();  // Then clear the outer map
    
    data.clear(); //Fixme: Crach in Debug Mode and using Engine.dll
}

AssetManager& AssetManager::Get(){
    return globalAssetManager;
}

class AssetManagerFileUpdateListener : public efsw::FileWatchListener {
public:
    AssetManager* assetManager = nullptr;

    void handleFileAction(
        efsw::WatchID watchid, const std::string& dir,
        const std::string& filename, efsw::Action action,
        std::string oldFilename) override 
    {
        Assert(false && "Outdate becose weak_ptr update");
        /*switch ( action ) {
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
        }*/

        auto removeBasePath = [](const std::string& fullPath, const std::string& basePath) -> std::string {
            if (fullPath.rfind(basePath, 0) == 0) { // basePath is a prefix
                return fullPath.substr(basePath.length());
            }
            return fullPath; // basePath not found at start
        };

        auto convertToForwardSlashes = [](std::string path) -> std::string {
            std::replace(path.begin(), path.end(), '\\', '/');
            return path;
        };

        std::string fullPath = dir + filename;
        fullPath = removeBasePath(fullPath, std::filesystem::current_path().string() + "\\");
        fullPath = convertToForwardSlashes(fullPath);
        LogInfo("FileWatching: {}", fullPath);

        if(action == efsw::Actions::Modified){
            for(auto& i: assetManager->data){
                if(i.second.count(fullPath) > 0){
                    std::lock_guard<std::mutex> lock(assetManager->toApplyHotReloadMutex);
                    //assetManager->toApplyHotReload.push_back(i.second[fullPath]);
                    #ifndef USE_WEAK_PTR
                    assetManager->toApplyHotReload.insert(i.second[fullPath]); //current code comented becose weak_ptr update
                    #endif
                }
            }
        }
    }
};

void AssetManager::StartHotReload(){
    efsw::FileWatcher* fileWatcher = new efsw::FileWatcher();
    AssetManagerFileUpdateListener* listener = new AssetManagerFileUpdateListener();
    listener->assetManager = this;
    LogInfo("Start Filewatch on: {}", std::filesystem::current_path().string());
    efsw::WatchID watchID = fileWatcher->addWatch(std::filesystem::current_path().string(), listener, true);
    fileWatcher->watch();
}

void AssetManager::StopHotReload(){
    delete listener;
    delete fileWatcher;
}

void AssetManager::ApplyHotReload(){
    std::lock_guard<std::mutex> lock(toApplyHotReloadMutex);
    for(auto& i: toApplyHotReload){
        LogInfo("Apply HotReload: {}", i->Path());
        i->Reload();
    }
    toApplyHotReload.clear();
}

}