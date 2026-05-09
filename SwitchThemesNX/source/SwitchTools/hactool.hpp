#pragma once
#include <unordered_map>
#include <vector>
#include <string>
#include <array>
#include <cstdint>

namespace hactool 
{
	std::unordered_map<std::string, std::vector<uint8_t>> ExtractFiles(uint64_t contentId, std::vector<std::string> files);

	std::array<uint8_t, 32> GetTitleBuildID(uint64_t contentID);
	std::string BuildIDToString(std::array<uint8_t, 32> data);

	std::string QlaunchBuildID();
}