#pragma once

#include "../SwitchThemesCommon/Common.hpp"

#include <string>

namespace RomfsCache 
{
	const FileData& GetFile(const ThemeTargetInfo& info);
	const FileData& GetFile(u64 contentId, const std::string& filePath);
	const FileData& GetFile(u64 contentId, const std::string& filePath);

	const FileContainer& GetContent(u64 contentId);
}