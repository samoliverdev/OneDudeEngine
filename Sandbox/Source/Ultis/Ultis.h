#pragma once
#include <OD/Scene/Scripts.h>
#include <OD/Platform/Platform.h>
#include <stdlib.h>
#include <time.h>
#include <string>

namespace OD{
   class Material;
}

using namespace OD;

class Random{
public:
   Random(unsigned int seed);
   int Range(int min, int max);
};

int random(int min, int max);
bool FileExist(const std::string& path);

Ref<Material> LoadFloorMaterial();
Ref<Material> LoadRockMaterial();
Ref<Material> LoadMaterial1();

struct RotateScript: public ScriptBase<RotateScript>{
   float speed = 40;

   inline void OnUpdate() override {
      TransformComponent& transform = scene->GetComponent<TransformComponent>(entity);
      transform.LocalEulerAngles(Vector3(transform.LocalEulerAngles().x, Platform::GetTime() * speed, transform.LocalEulerAngles().z));
   }

   template <class Archive>
   void serialize(Archive & ar){
      ar(
         CEREAL_NVP(speed)
      );
   }
};
