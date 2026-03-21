#include "OD/Base.h"
#define ENTT_ASSERT(condition, msg) Assert((condition) && (msg))
//#include <entt/entt.hpp>
#include <entt/entity/registry.hpp>

namespace OD {

using Entity = entt::entity;
using Registry = entt::registry;
#define EntityNull entt::null
//#define EntityNull (Entity)UINT32_MAX

}