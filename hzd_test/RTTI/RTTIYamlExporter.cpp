#include <algorithm>
#include <string_view>

#include "RTTIYamlExporter.h"

RTTIYamlExporter::RTTIYamlExporter(const ankerl::unordered_dense::set<const HRZ::RTTI *>& Types, const std::string_view GameTypePrefix) : m_Types(Types.begin(), Types.end()), m_GameTypePrefix(GameTypePrefix)
{
	// Always sort by name during export
	std::sort(m_Types.begin(), m_Types.end(), [](const HRZ::RTTI *A, const HRZ::RTTI *B)
	{
		return A->GetSymbolName() < B->GetSymbolName();
	});
}

void RTTIYamlExporter::ExportRTTITypes(const std::string_view Directory)
{
	CreateDirectoryA(Directory.data(), nullptr);

	auto outputPath = std::format("{0:}\\Decima.{1:}.yaml", Directory, m_GameTypePrefix);

	if (fopen_s(&m_FileHandle, outputPath.c_str(), "w") == 0)
	{
		Print("---");
		ExportGGRTTI();
		Print("...");
		fclose(m_FileHandle);
	}
}

void RTTIYamlExporter::ExportGGRTTI()
{
	// Enum and class members. The rest are unused.
	for (auto type : m_Types)
	{
		if (auto asEnum = type->AsEnum(); asEnum)
		{
			Print("{0:}:", asEnum->GetSymbolName());

			IncreaseIndent();
			if (asEnum->m_InfoType == HRZ::RTTI::InfoType::EnumFlags)
				Print("type: \"enum flags\"");
			else
				Print("type: \"enum\"");
			Print("typeid: {0:}", asEnum->GetCoreBinaryTypeId());
			Print("size: {0:}", asEnum->m_EnumUnderlyingTypeSize);
			Print("alignment: {0:}", 0);

			Print("values:");
			IncreaseIndent();
			for (auto& member : asEnum->EnumMembers())
				Print("- [\"{0:}\", {1:}]", EscapeString(member.m_Name), member.m_Value);
			DecreaseIndent();

			DecreaseIndent();
		}
		else if (auto asClass = type->AsClass(); asClass)
		{
			Print("{0:}:", asClass->GetSymbolName());

			IncreaseIndent();
			Print("type: \"class\"");
			Print("typeid: {0:}", asClass->GetCoreBinaryTypeId());
			Print("size: {0:}", asClass->m_Size);
			Print("alignment: {0:}", asClass->m_Alignment);
			Print("flags: {0:}", asClass->m_Flags);
			Print("unknownC: {0:}", asClass->m_UnknownC);

			Print("messages:");
			IncreaseIndent();
			for (auto& message : asClass->ClassMessageHandlers())
				Print("- \"{0:}\"", message.m_Type->GetSymbolName());
			DecreaseIndent();

			Print("bases:");
			IncreaseIndent();
			for (auto& base : asClass->ClassInheritance())
				Print("- {{name: \"{0:}\", offset: {1:}}}", base.m_Type->GetSymbolName(), base.m_Offset);
			DecreaseIndent();

			auto members = asClass->GetCategorizedClassMembers();

			Print("members:");
			IncreaseIndent();
			for (auto& [member, category, _] : members)
			{
				Print("- {{name: \"{0:}\", type: \"{1:}\", category: \"{2:}\", offset: {3:}, flags: {4:}, is_property: {5:s}, is_savestate: {6:s}}}",
					member->m_Name,
					member->m_Type->GetSymbolName(),
					category,
					member->m_Offset,
					static_cast<uint32_t>(member->m_Flags),
					member->IsProperty(),
					member->IsSaveStateOnly());
			}
			DecreaseIndent();

			DecreaseIndent();
		}
	}
}

void RTTIYamlExporter::IncreaseIndent()
{
	m_IndentSpaces += 2;
}

void RTTIYamlExporter::DecreaseIndent()
{
	m_IndentSpaces -= 2;
}

std::string RTTIYamlExporter::EscapeString(std::string Value)
{
	using namespace std::literals::string_view_literals;

	auto replaceAll = [&](const std::string_view From, const std::string_view To)
	{
		for (size_t startPos = 0; (startPos = Value.find(From, startPos)) != std::string::npos;)
		{
			Value.replace(startPos, From.length(), To);
			startPos += To.length();
		}
	};

	replaceAll("\\"sv, "\\\\"sv);
	replaceAll("\""sv, "\\\""sv);
	return Value;
}