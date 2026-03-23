#if 0
#pragma once
#include <cereal/cereal.hpp>
#include <cereal/types/array.hpp>

#define TYPE_TO_STRING(T) #T
#define ArchiveDump(archive, data) try{ archive(data); }catch(const cereal::Exception& e){ /*LogWarning("ErrorOnTrySerialize: {}", std::string(e.what()));*/ }
#define ArchiveDumpNVP(archive, data) try{ archive(CEREAL_NVP(data)); }catch(const cereal::Exception& e){ /*LogWarning("ErrorOnTrySerialize: {}", std::string(e.what()));*/ }
#define ArchiveDumpNamed(archive, name, data) try{ archive(cereal::make_nvp(name, data)); }catch(const cereal::Exception& e){ /*LogWarning("ErrorOnTrySerialize: {}", std::string(e.what()));*/ }
#endif