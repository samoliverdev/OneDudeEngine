#include "Ultis.h"
#include <OD/Graphics/Material.h>
#include <iostream>

Random::Random(unsigned int seed){
   srand(seed);
}

int Random::Range(int min, int max){
   return min + rand() % (( max + 1 ) - min);
}

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

Ref<Material> LoadFloorMaterial(){
   //std::string path = "res/Game/Textures/floor.material";

   //if(FileExist(path) == false){
      Ref<Material> m = ResourceManager::Get().Create<Material>();

      //m->SetEnableInstancing(true);
      m->SetShader(ResourceManager::Get().LoadByPath<Shader>("Engine/Shaders/Lit.glsl"));
      m->SetTexture("mainTex", ResourceManager::Get().LoadByPath<Texture2D>("Sandbox/Textures/floor"));
      m->SetVector4("color", Vector4(1, 1, 1, 1));
      return m;

      //m->Save(path);
   //}

   //return AssetManager::Get().LoadMaterial(path);
}

Ref<Material> LoadRockMaterial(){
   //std::string path = "res/Game/Textures/rock.material";

   //if(FileExist(path) == false){
      Ref<Material> m = ResourceManager::Get().Create<Material>();
      //m->SetShader(AssetManager::Get().LoadAsset<Shader>("Engine/Shaders/StandDiffuse.glsl"));
      m->SetShader(ResourceManager::Get().LoadByPath<Shader>("Engine/Shaders/Lit.glsl"));
      m->SetTexture("mainTex", ResourceManager::Get().LoadByPath<Texture2D>("Sandbox/Textures/Rock"));
      m->SetVector4("color", Vector4(1, 1, 1, 1));
      return m;

      //m->Save(path);
   //}

   //return AssetManager::Get().LoadMaterial(path);
}

Ref<Material> LoadMaterial1(){
   //std::string path = "res/Game/Materials/mat1.material";

   //if(FileExist(path) == false){
      Ref<Material> m = ResourceManager::Get().Create<Material>();
      m->SetShader(ResourceManager::Get().LoadByPath<Shader>("Engine/Shaders/Lit.glsl"));
      m->SetTexture("mainTex", ResourceManager::Get().LoadByPath<Texture2D>("Sandbox/Textures/image"));
      m->SetVector4("color", Vector4(1, 1, 1, 1));                 
      return m;
   //   m->Save(path);
   //}

   //return AssetManager::Get().LoadMaterial(path);
}