#pragma once

#include "../SwitchThemesCommon/NXTheme.hpp"
#include <vector>

namespace RomfsCache 
{
	const std::vector<u8>& GetFile(const ThemeTargetInfo& info);
	const std::vector<u8>& GetFile(u64 contentId, const std::string& filePath);
}