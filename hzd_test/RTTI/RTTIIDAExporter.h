#pragma once

#include "ankerl/unordered_dense.h"
#include "../HRZ/Core/RTTI.h"
#include "../HRZ/Core/ExportedSymbol.h"
#include "RTTI/RTTICommon.h"

class RTTIIDAExporter
{
private:
	uintptr_t m_ModuleBase = 0;
	FILE* m_FileHandle = nullptr;
	std::vector<const HRZ::RTTI*> m_Types;
	const std::string m_GameTypePrefix;

public:
	RTTIIDAExporter() = delete;
	RTTIIDAExporter(const RTTIIDAExporter&) = delete;
	RTTIIDAExporter(const ankerl::unordered_dense::set<const HRZ::RTTI*>& Types, const std::string_view GameTypePrefix);
	RTTIIDAExporter& operator=(const RTTIIDAExporter&) = delete;

	void ExportRTTITypes(const std::string_view Directory);
	void ExportFullgameTypes(const std::string_view Directory);

private:
	void ExportMSRTTI();
	void ExportGGRTTI();
	void ExportGGRTTIStructures();
	void ExportGameSymbolRTTI();
	void ExportFullgameScriptSymbols();



	template<typename... TArgs>
	inline void Print(const std::string_view Format, TArgs&&... Args)
	{
		char buffer[2048];
		*RTTI_fmt::fmt(buffer, std::size(buffer) - 1, Format, std::forward<TArgs>(Args)...).out = '\0';

		fputs(buffer, m_FileHandle);
	}

	template<typename T>
	inline void WriteReflectedMemberT(const char* Name, size_t Offset, const char* StructType)
	{
		uint32_t typeFlags = 0x60000000;// FF_STRUCT

		if (!StructType)
		{
			switch (sizeof(T))
			{
			case 1: typeFlags = 0x0; break;// FF_BYTE
			case 2: typeFlags = 0x10000000; break;// FF_WORD
			case 4: typeFlags = 0x20000000; break;// FF_DWORD
			case 8: typeFlags = 0x30000000; break;// FF_QWORD
			}
		}

		if constexpr (std::is_pointer_v<T>)
			typeFlags |= 0x5500400;// FF_DATA | FF_0OFF | FF_1OFF (data offset)

		if (!StructType)
			Print("mid = add_struc_member(id, \"{0:}\", {1:#X}, {2:#X}, -1, {3:});\n", Name, Offset, typeFlags, sizeof(T));
		else
			Print("mid = add_struc_member(id, \"{0:}\", {1:#X}, {2:#X}, get_struc_id(\"{3:}\"), {4:});\n", Name, Offset, typeFlags, StructType, sizeof(T));
	}

	static std::string BuildReturnType(std::string_view BaseType, std::string_view Modifiers, bool IDAFormat);
	static std::string BuildArgType(size_t Index, std::string BaseType, std::string_view Modifiers, bool IDAFormat);
	static std::string BuildGameSymbolFunctionDecl(const HRZ::ExportedSymbolMember::LanguageInfo& Info, bool IDAFormat);
};