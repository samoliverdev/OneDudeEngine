#pragma once
#include "OD/Core/Math.h"
#include <cmath>
#include <cstdlib> // For rand()
#include <ctime>   // For seeding

using namespace OD;

namespace Standard{

class Random2{
public:
   Random2(unsigned int seed);
   int Range(int min, int max);
};

namespace Ultis{

inline float RandomFloat() {
    return (static_cast<float>(rand()) / RAND_MAX) * 2.0f - 1.0f;
}

// Returns a random float in the range [min, max)
inline float RandomRange(float min, float max) {
    return min + static_cast<float>(rand()) / RAND_MAX * (max - min);
}

inline int RandomRange(int min, int max) {
    if(max <= min) return min;
    // Ensure max is never returned
    return min + rand() % (max - min);
}

// Function to return a random point inside a unit sphere
inline Vector3 RandomInsideUnitSphere() {
    while(true){
        // Generate random x, y, z in the range [-1, 1]
        float x = RandomFloat();
        float y = RandomFloat();
        float z = RandomFloat();

        // Compute squared length
        float lengthSquared = x * x + y * y + z * z;

        // If inside the unit sphere, return the point
        if(lengthSquared <= 1.0f) return {x, y, z};
    }

    return Vector3Zero;
}

};

}