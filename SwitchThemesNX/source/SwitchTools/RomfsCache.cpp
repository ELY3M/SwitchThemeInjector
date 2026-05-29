#include "RomfsCache.hpp"
#include "../SwitchThemesCommon/MyTypes.h"
#include "hactool.hpp"

#include <unordered_map>
#include <vector>
#include <string>
#include <format>
#include <stdexcept>

namespace {
	std::unordered_map<u64, FileContainer> Cache;
}

const FileContainer& RomfsCache::GetContent(u64 contentId)
{
	if (!Cache.count(contentId))
	{
		auto targets = ThemeTargetInfo::GetTargetsForTitleId(contentId);
		if (!targets.size())
			throw std::runtime_error("No theme targets found for content id " + ThemeTargetInfo::TitleIdToString(contentId));

		auto files = hactool::ExtractFiles(contentId, targets);
		Cache[contentId] = std::move(files);
	}

	return Cache[contentId];
}

const FileData& RomfsCache::GetFile(u64 contentId, const std::string& filePath)
{
	const auto& cache = GetContent(contentId);

	if (!cache.count(filePath))
		throw std::runtime_error("File " + filePath + " not found in the extracted content of " + std::format("{:x}", contentId));

	return cache.at(filePath);
}

const FileData& RomfsCache::GetFile(const ThemeTargetInfo& info)
{
	return GetFile(info.TitleId, info.SzsFile);
}
