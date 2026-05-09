#pragma once

#include "../SwitchThemesCommon/NXTheme.hpp"

#include <vector>
#include <string>
#include <unordered_map>

namespace RomfsCache 
{
	using File = std::vector<u8>;
	using Dir = std::unordered_map<std::string, File>;

	const File& GetFile(const ThemeTargetInfo& info);
	const File& GetFile(u64 contentId, const std::string& filePath);
	const File& GetFile(u64 contentId, const std::string& filePath);

	const Dir& GetContent(u64 contentId);
}