#pragma once

#include <OD/OD.h>
#include <stdlib.h>
#include <time.h>
#include <string>

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

struct RotateScript: public Script{
   float speed = 40;

   inline void OnUpdate() override {
      TransformComponent& transform = GetEntity().GetComponent<TransformComponent>();
      transform.LocalEulerAngles(Vector3(transform.LocalEulerAngles().x, Platform::GetTime() * speed, transform.LocalEulerAngles().z));
   }

   template <class Archive>
   void serialize(Archive & ar){
      ar(
         CEREAL_NVP(speed)
      );
   }
};
