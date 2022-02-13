#pragma once

#include <robin_hood_hashing/include/robin_hood.h>

#include "../HRZ/Core/RTTI.h"

namespace RTTIScanner
{

const robin_hood::unordered_set<const HRZ::RTTI *>& GetAllTypes();
const HRZ::RTTI *GetTypeByName(const std::string_view Name);
void ExportAll(const std::string_view Directory, const std::string_view GameTypePrefix);
void ScanForRTTIStructures();
void RegisterRTTIStructures();

}