#include "Ultis.h"
#include <chrono>

namespace Standard{

Random2::Random2(unsigned int seed){
   srand(seed);
}

int Random2::Range(int min, int max){
   return min + rand() % (( max + 1 ) - min);
}

}