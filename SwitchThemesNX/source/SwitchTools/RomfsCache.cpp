#include "RomfsCache.hpp"
#include "../SwitchThemesCommon/MyTypes.h"
#include "hactool.hpp"

#include <unordered_map>
#include <vector>
#include <string>
#include <format>
#include <stdexcept>

namespace {
	std::unordered_map<u64, std::unordered_map<std::string, std::vector<u8>>> Cache;
}

const std::vector<u8>& RomfsCache::GetFile(u64 contentId, const std::string& filePath)
{
	if (!Cache.count(contentId))
	{
		auto targets = ThemeTargetInfo::GetTargetsForTitleId(contentId);
		if (!targets.size())
			throw std::runtime_error("No theme targets found for content id " + std::format("{:x}", contentId));

		auto files = hactool::ExtractFiles(contentId, targets);
		Cache[contentId] = std::move(files);
	}

	const auto& cache = Cache[contentId];

	if (!cache.count(filePath))
		throw std::runtime_error("File " + filePath + " not found in the extracted content of " + std::format("{:x}", contentId));

	return cache.at(filePath);
}

const std::vector<u8>& RomfsCache::GetFile(const ThemeTargetInfo& info)
{
	return GetFile(info.TitleId, info.SzsFile);
}