#pragma once
#include "SerializationCore.h"

#include <cereal/archives/binary.hpp>
#include <cereal/archives/portable_binary.hpp>
#include <cereal/archives/json.hpp>
#define ODOutputArchive cereal::JSONOutputArchive
#define ODInputArchive cereal::JSONInputArchive
