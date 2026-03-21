#include "BuildsPanel.h"
#include "OD/Scene/SceneManager.h"
#include "OD/Graphics/Model.h"
#include "OD/Graphics/Texture.h"
#include <filesystem>
#include <iostream>
#include <vector>
#include <string>
#include <functional>

namespace OD{

namespace fs = std::filesystem;

// Callback type: return true to copy, false to skip
using FileCallback = std::function<bool(const fs::path& sourcePath, const fs::path& destPath)>;

void CopyDirectoryRecursive(
    const fs::path& sourceDir,
    const fs::path& destDir,
    const std::vector<std::string>& allowedExtensions = {}, // empty = all files
    FileCallback callback = nullptr
){
    if(!fs::exists(sourceDir) || !fs::is_directory(sourceDir)){
        //LogError("Source directory does not exist or is not a directory");
        std::cerr << "Source directory does not exist or is not a directory\n";
        return;
    }

    fs::create_directories(destDir);

    for(const auto& entry : fs::recursive_directory_iterator(sourceDir)){
        const auto& path = entry.path();
        auto relativePath = fs::relative(path, sourceDir);
        auto destPath = destDir / relativePath;

        if(entry.is_directory()){
            fs::create_directories(destPath);
        } else if(entry.is_regular_file()){
            // Check file extension
            if(!allowedExtensions.empty()){
                std::string ext = path.extension().string();
                bool match = false;
                for(const auto& allowed : allowedExtensions){
                    if (ext == allowed) {
                        match = true;
                        break;
                    }
                }
                if(!match) continue; // skip this file
            }

            // Call user callback
            if(callback){
                if(!callback(path, destPath)) continue; // skip if callback returns false
            }

            try {
                fs::copy_file(path, destPath, fs::copy_options::overwrite_existing);
            } catch(const std::exception& e){
                //LogError("Failed to copy {} : {}", path.c_str(), e.what());
                std::cerr << "Failed to copy " << path << " : " << e.what() << "\n";
            }
        }
    }
}

bool IsPathOutsideCurrentDir(const fs::path& buildPath){
    try {
        fs::path absBuildPath = fs::absolute(buildPath);
        fs::path currentPath = fs::absolute(fs::current_path());

        // Canonicalize to resolve symlinks, "..", "."
        absBuildPath = fs::weakly_canonical(absBuildPath);
        currentPath = fs::weakly_canonical(currentPath);

        // Check if absBuildPath starts with currentPath
        return absBuildPath.string().find(currentPath.string()) != 0;
    }
    catch (const fs::filesystem_error& e) {
        std::cerr << "Filesystem error: " << e.what() << "\n";
        return true; // if any error, treat as outside
    }
}

std::string GetNormalizedRelativePathFromCurrent(const fs::path& src){
    // Get absolute canonical paths
    fs::path current = fs::weakly_canonical(fs::current_path());
    fs::path absoluteSrc = fs::weakly_canonical(fs::absolute(src));

    // Make relative
    fs::path relative = fs::relative(absoluteSrc, current);

    // Convert to generic format (always '/')
    return relative.generic_string();
}

void ReplaceAll(
    std::string& text,
    const std::string& from,
    const std::string& to
){
    if(from.empty()) return;

    size_t pos = 0;
    while((pos = text.find(from, pos)) != std::string::npos){
        text.replace(pos, from.length(), to);
        pos += to.length();
    }
};

bool HasExtension(const std::string& ext, const std::vector<std::string>& list){
    return std::find(list.begin(), list.end(), ext) != list.end();
}

void ProcessTextAssets(
    const fs::path& buildPath,
    const std::vector<std::string>& textExtensions,
    const std::vector<std::string>& oldPath,
    const std::vector<std::string>& newPath
){
    for(const auto& entry : fs::recursive_directory_iterator(buildPath)){
        if(!entry.is_regular_file()) continue;

        std::string ext = entry.path().extension().string();

        if(!HasExtension(ext, textExtensions)) continue;

        // Read file
        std::ifstream in(entry.path(), std::ios::binary);
        if(!in.is_open()) continue;

        std::string content((std::istreambuf_iterator<char>(in)), std::istreambuf_iterator<char>());

        in.close();

        ReplaceAll(content, "\\", "/");
        ReplaceAll(content, "//", "/");

        // Replace paths
        for(size_t i = 0; i < oldPath.size(); ++i){
            ReplaceAll(content, oldPath[i], newPath[i]);
        }

        // Write back
        std::ofstream out(entry.path(), std::ios::binary | std::ios::trunc);
        out << content;
        out.close();

        LogInfo("Processed text asset: {}", entry.path().generic_string());
    }
}

template <typename T> 
bool BuildAsset(
    const fs::path& src,
    const fs::path& buildPath,
    std::vector<std::string>& oldPath,
    std::vector<std::string>& newPath,
    std::string finalExt
){
    // 1️⃣ Extension check
    std::string ext = src.extension().string();

    // 2️⃣ Get normalized relative source path
    std::string relativeSrc = GetNormalizedRelativePathFromCurrent(src);
    LogInfo("Building model: {}", relativeSrc);

    fs::path relativePath(relativeSrc);

    // 3️⃣ Replace extension with .modelbin
    fs::path relativeModelPath = relativePath;
    relativeModelPath.replace_extension(finalExt);

    std::string relativeModelStr = relativeModelPath.generic_string();

    // 4️⃣ Store relative paths (manifest mapping)
    oldPath.push_back(relativeSrc);
    newPath.push_back(relativeModelStr);

    // 5️⃣ Build absolute destination path
    fs::path absoluteSavePath = fs::absolute(buildPath) / relativeModelPath;

    // Ensure directory exists
    fs::create_directories(absoluteSavePath.parent_path());

    // 6️⃣ Load and save
    Ref<T> asset = AssetManager::Get().LoadAsset<T>(relativeSrc);
    if(asset == nullptr){
        LogError("Erro to load {}", relativeSrc);
        return false;
    }

    asset->Save(absoluteSavePath.string(), Asset::SaveType::FinalBinary);

    return false; // prevent original file copy
};

std::string NormalizePath(const fs::path& p){
    std::string s = p.lexically_normal().generic_string();
    if(!s.empty() && s.back() != '/'){
        s += '/';
    }
    return s;
}

bool IsSubPathOf(const fs::path& file, const std::string& folder){
    std::string filePath = file.lexically_normal().generic_string();
    std::string folderNorm = folder;

    // ensure folder ends with /
    if(!folderNorm.empty() && folderNorm.back() != '/'){
        folderNorm += '/';
    }

    return filePath.rfind(folderNorm, 0) == 0; // starts_with
}

bool ShouldSkip(
    const fs::path& src,
    const std::vector<std::string>& skipFolders
){
    for(const auto& folder : skipFolders){
        if(IsSubPathOf(src, folder)) return true;
    }
    return false;
}

void RemoveConvertedAssetMeta(
    const fs::path& buildPath,
    const std::vector<std::string>& oldPath
){
    for(const auto& rel : oldPath){
        fs::path p = buildPath / rel;

        fs::path meta = p;
        meta += ".meta";   // blending.png.meta

        if(fs::exists(meta)){
            fs::remove(meta);
            LogInfo("Removed old meta: {}", meta.string());
        }
    }
}

BuildsPanel::BuildsPanel(){
    name = "BuildsPanel";
    show = false;
}

void BuildsPanel::OnGui(){
    if(ImGui::Begin("BuildsPanel", &show)){
        ImGui::DrawString("Build Path", buildPath);

        if(ImGui::Button("Build")) Build();

        ImGui::End();
    }
}

void BuildsPanel::Build(){
    auto& sceneManager = SceneManager::Get();

    if(sceneManager.GetActiveScene()->Running()){
        LogError("Can not build on running scene!!!");
        return;
    }

    if(IsPathOutsideCurrentDir(buildPath) == false){
        LogError("Build Path can not to be inside current path!!!");
        return;
    };

    Ref<Scene> cur = sceneManager.GetActiveScene();
    sceneManager.NewScene();

    std::vector<std::string> oldPath;
    std::vector<std::string> newPath;
    
    CopyDirectoryRecursive(
        "./",
        buildPath,
        {".meta", ".glb", ".glft", ".fbx", ".obj", ".mtl", ".dae", ".scene", ".prefab", ".png", ".jpg", ".jpeg", ".material", ".glsl", ".shader", ".compute", ".wav", ".mp3", ".ttf", ".otf", ".ini"},
        [&](const fs::path& src, const fs::path& dst) -> bool {
            if(ShouldSkip(src, dontBuildAssetFolders)) return true;

            std::string ext = src.extension().string();

            // Models
            if(HasExtension(ext, {".glb", ".glft", ".fbx", ".dae"})) return BuildAsset<Model>(src, buildPath, oldPath, newPath, ".modelbin"); // return BuildModel(src, buildPath, oldPath, newPath);
            if(HasExtension(ext, {".png", ".jpg", ".jpeg"})) return BuildAsset<Texture2D>(src, buildPath, oldPath, newPath, ".texturebin"); // return BuildModel(src, buildPath, oldPath, newPath);
            if(HasExtension(ext, {".glsl", ".shader"})) return BuildAsset<Shader>(src, buildPath, oldPath, newPath, ".shaderbin"); // return BuildModel(src, buildPath, oldPath, newPath);
            //TODO: Add A Simple audio pack/save to AudioClip

            return true; // normal copy
        }
    );

    ProcessTextAssets(
        buildPath,
        {".scene", ".prefab", ".material"},
        oldPath,
        newPath
    );

    RemoveConvertedAssetMeta(buildPath, oldPath);

    sceneManager.SetActiveScene(cur);
}

}