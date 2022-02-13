#include <algorithm>

#include "../XUtil.h"

#include "RTTIScanner.h"
#include "RTTIIDAExporter.h"
#include "RTTICSharpExporter.h"
#include "RTTIYamlExporter.h"

namespace RTTIScanner
{

robin_hood::unordered_set<const HRZ::RTTI *> ScannedRTTITypes;
robin_hood::unordered_set<const HRZ::RTTI *> RegisteredRTTITypes;

const robin_hood::unordered_set<const HRZ::RTTI *>& GetAllTypes()
{
	// If no types are present, try to gather them now
	if (!ScannedRTTITypes.empty() && RegisteredRTTITypes.empty())
		RegisterRTTIStructures();

	return RegisteredRTTITypes;
}

const HRZ::RTTI *GetTypeByName(const std::string_view Name)
{
	for (auto rtti : GetAllTypes())
	{
		if (rtti->GetSymbolName() == Name)
			return rtti;
	}

	return nullptr;
}

void ExportAll(const std::string_view Directory, const std::string_view GameTypePrefix)
{
	RTTIIDAExporter idaExporter(GetAllTypes(), GameTypePrefix);
	idaExporter.ExportRTTITypes(Directory);

	RTTIYamlExporter yamlExporter(GetAllTypes(), GameTypePrefix);
	yamlExporter.ExportRTTITypes(Directory);

	RTTICSharpExporter csExporter(GetAllTypes(), GameTypePrefix);
	csExporter.ExportAll(Directory);
}

void ScanForRTTIStructures()
{
	//
	// Manually scan for embedded RTTI structures before they're initialized. Most, but not all, types
	// are registered through RTTIFactory. This should catch the remainder.
	//
	// NOTE: Some types will be created at runtime. We don't care about them.
	//
	auto [rdataBase, rdataEnd] = Offsets::GetRdataSection();
	auto [dataBase, dataEnd] = Offsets::GetDataSection();

	auto results = XUtil::FindPatterns(dataBase, dataEnd - dataBase, "FF FF FF FF ? ? ? ?");

	auto isDataSegment = [&]<typename T>(T *Pointer)
	{
		return reinterpret_cast<uintptr_t>(Pointer) >= dataBase && reinterpret_cast<uintptr_t>(Pointer) < dataEnd;
	};

	auto isRdataSegment = [&]<typename T>(T *Pointer)
	{
		return reinterpret_cast<uintptr_t>(Pointer) >= rdataBase && reinterpret_cast<uintptr_t>(Pointer) < rdataEnd;
	};

	for (uintptr_t result : results)
	{
		auto rtti = reinterpret_cast<const HRZ::RTTI *>(result);

		if (rtti->m_InfoType < HRZ::RTTI::InfoType::Primitive || rtti->m_InfoType > HRZ::RTTI::InfoType::POD)
			continue;

		// Validate pointers before blindly adding them to the collection. RTTI entries typically have
		// multiple fields for accessing the .data section, so check those.
		if (auto asContainer = rtti->AsContainer(); asContainer)
		{
			if (!isDataSegment(asContainer->m_Type) || !isDataSegment(asContainer->m_Data))
				continue;
		}
		else if (auto asEnum = rtti->AsEnum(); asEnum)
		{
			if (!isDataSegment(asEnum->m_Name) || !isDataSegment(asEnum->m_Values))
				continue;
		}
		else if (auto asClass = rtti->AsClass(); asClass)
		{
			if (!isRdataSegment(asClass->m_Name) || asClass->m_Alignment <= 0)
				continue;
		}
		else
		{
			// Discard the rest for now
			continue;
		}

		ScannedRTTITypes.emplace(rtti);
	}
}

void RegisterTypeInfoRecursively(const HRZ::RTTI *Info)
{
	if (!Info)
		return;

	if (RegisteredRTTITypes.contains(Info))
		return;

	RegisteredRTTITypes.emplace(Info);

	if (auto asClass = Info->AsClass(); asClass)
	{
		// Register base classes
		for (auto& base : asClass->ClassInheritance())
			RegisterTypeInfoRecursively(base.m_Type);

		// Then field types
		for (auto& member : asClass->ClassMembers())
			RegisterTypeInfoRecursively(member.m_Type);

		// Then message types
		for (auto& message : asClass->ClassMessageHandlers())
			RegisterTypeInfoRecursively(message.m_Type);

		// Then exported script symbols
		if (asClass->m_GetExportedSymbols)
			RegisterTypeInfoRecursively(asClass->m_GetExportedSymbols());
	}
	else if (auto asContainer = Info->AsContainer(); asContainer)
	{
		RegisterTypeInfoRecursively(asContainer->GetContainedType());
	}
}

void RegisterRTTIStructures()
{
	RegisteredRTTITypes.clear();

	for (auto rtti : ScannedRTTITypes)
		RegisterTypeInfoRecursively(rtti);

	ScannedRTTITypes.clear();
}

}