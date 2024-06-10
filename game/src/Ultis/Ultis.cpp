#include "Ultis.h"
#include <iostream>

int random(int min, int max){
   static bool first = true;
   if(first) {  
      srand(time(NULL));
      first = false;
   }
   return min + rand() % (( max + 1 ) - min);
}

bool FileExist(const std::string& path){
   std::ifstream file;
   file.open(path);

   if(file) return true;
   return false;
}

Ref<Material> _mat = nullptr;

Ref<Material> LoadFloorMaterial(){
   if(_mat != nullptr) return _mat;

   //std::string path = "res/Game/Textures/floor.material";

   //if(FileExist(path) == false){
      Ref<Material> m = CreateRef<Material>();

      //m->SetEnableInstancing(true);
      m->SetShader(AssetManager::Get().LoadAsset<Shader>("res/Engine/Shaders/Lit.glsl"));
      m->SetTexture("mainTex", AssetManager::Get().LoadAsset<Texture2D>("res/Game/Textures/floor.jpg"));
      m->SetVector4("color", Vector4(1, 1, 1, 1));
      _mat = m;
      return m;

      //m->Save(path);
   //}

   //return AssetManager::Get().LoadMaterial(path);
}

Ref<Material> LoadRockMaterial(){
   //std::string path = "res/Game/Textures/rock.material";

   //if(FileExist(path) == false){
      Ref<Material> m = CreateRef<Material>();

      m->SetShader(AssetManager::Get().LoadAsset<Shader>("res/Engine/Shaders/StandDiffuse.glsl"));
      m->SetTexture("mainTex", AssetManager::Get().LoadAsset<Texture2D>("res/Game/Textures/rock.jpg"));
      m->SetVector4("color", Vector4(1, 1, 1, 1));
      return m;

      //m->Save(path);
   //}

   //return AssetManager::Get().LoadMaterial(path);
}

Ref<Material> LoadMaterial1(){
   //std::string path = "res/Game/Materials/mat1.material";

   //if(FileExist(path) == false){
      Ref<Material> m = CreateRef<Material>();

      m->SetShader(AssetManager::Get().LoadAsset<Shader>("res/Engine/Shaders/StandDiffuse.glsl"));
      m->SetTexture("mainTex", AssetManager::Get().LoadAsset<Texture2D>("res/Game/Textures/image.jpg"));
      m->SetVector4("color", Vector4(1, 1, 1, 1));
      return m;
   //   m->Save(path);
   //}

   //return AssetManager::Get().LoadMaterial(path);
}