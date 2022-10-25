#pragma once

#include "ankerl/unordered_dense.h"

#include "../HRZ/Core/RTTI.h"

namespace RTTIScanner
{

const ankerl::unordered_dense::set<const HRZ::RTTI *>& GetAllTypes();
const HRZ::RTTI *GetTypeByName(const std::string_view Name);
void ExportAll(const std::string_view Directory, const std::string_view GameTypePrefix);
void ScanForRTTIStructures();
void RegisterRTTIStructures();

}