#include <unordered_map>
#include <vector>
#include "../fs.hpp"
#include "NXTheme.hpp"
#include "json.hpp"

SystemVersion HOSVer = { 0,0,0 };

// Info for fw 6.0+
const std::unordered_map<std::string, ThemeTargetInfo> ThemeTargetList6
{
	{"home", { ThemeTargetInfo::QlaunchID  , "Home menu"        , "/lyt/ResidentMenu.szs" } },
	{"lock", { ThemeTargetInfo::QlaunchID  , "Lock screen"      , "/lyt/Entrance.szs" } },
	{"apps", { ThemeTargetInfo::QlaunchID  , "All apps menu"    , "/lyt/Flaunch.szs" } },
	{"set",	 { ThemeTargetInfo::QlaunchID  , "Settings applet"  , "/lyt/Set.szs" } },
	{"news", { ThemeTargetInfo::QlaunchID  , "News applet"      , "/lyt/Notification.szs" } },
	{"user", { ThemeTargetInfo::UserPageID , "User page"        , "/lyt/MyPage.szs" } },
	{"psl",	 { ThemeTargetInfo::PslID      , "Player selection" , "/lyt/Psl.szs" } },
};

// Info for fw <=6.x
const std::unordered_map<std::string, ThemeTargetInfo> ThemeTargetList5
{
	{"home", { ThemeTargetInfo::QlaunchID  , "Home menu"        , "/lyt/ResidentMenu.szs" } },
	{"lock", { ThemeTargetInfo::QlaunchID  , "Lock screen"      , "/lyt/Entrance.szs" } },
	// These behaved differently before 6.0 due to common.szs being used instead of resident menu
	{"apps", { ThemeTargetInfo::QlaunchID  , "All applets"      , "/lyt/Flaunch.szs" } },
	{"set",	 { ThemeTargetInfo::QlaunchID  , "All applets"      , "/lyt/Set.szs" } },
	{"news", { ThemeTargetInfo::QlaunchID  , "All applets"      , "/lyt/Notification.szs" } },
	{"user", { ThemeTargetInfo::UserPageID , "User page"        , "/lyt/MyPage.szs" } },
	{"psl",	 { ThemeTargetInfo::PslID      , "Player selection" , "/lyt/Psl.szs" } },
};

const ThemeTargetInfo ThemeTargetInfo::QlaunchCommon =
{
	QlaunchID, "Home menu common layout", "/lyt/common.szs"
};

const ThemeTargetInfo* ThemeTargetInfo::Find(std::string nxThemeName)
{
	// TODO: Do we even need to support older firmware anymore?
	if (HOSVer.major <= 5)
	{
		if (ThemeTargetList5.count(nxThemeName))
			return &ThemeTargetList5.at(nxThemeName);
	}
	else
	{
		if (ThemeTargetList6.count(nxThemeName))
			return &ThemeTargetList6.at(nxThemeName);
	}

	return nullptr;
}

std::vector<std::string> ThemeTargetInfo::GetTargetsForTitleId(u64 tid)
{
	std::vector<std::string> res = {};

	if (tid == QlaunchID)
		res.push_back(QlaunchCommon.SzsFile);

	// Only care about ThemeTargetList6 since the szs file names are the same for both
	for (const auto& [name, info] : ThemeTargetList6)
	{
		if (info.TitleId == tid)
			res.push_back(info.SzsFile);
	}

	return res;
}

const ThemeTargetInfo* ThemeTargetInfo::FindBySzsName(std::string szsName, std::string& outNxPartName)
{
	// Only care about ThemeTargetList6 since the szs file names are the same for both
	for (const auto& [name, info] : ThemeTargetList6)
	{
		if (fs::GetFileName(info.SzsFile) == szsName) {
			outNxPartName = name;
			return &info;
		}
	}
	
	outNxPartName = "";
	return nullptr;
}

using namespace std;
using json = nlohmann::json;

ThemeFileManifest ParseNXThemeFile(SARC::SarcData &Archive)
{
	if (!Archive.files.count("info.json"))
	{
		return {-1,"","",""};
	}
	string jsn(reinterpret_cast<char*>((Archive.files["info.json"]).data()),(Archive.files["info.json"]).size());
	auto j = json::parse(jsn);
	
	ThemeFileManifest res = {0};
	if (j.count("Version") && j.count("Target"))
	{
		res.Version = j["Version"].get<int>();
		res.Target = j["Target"].get<string>();
	}
	else 
	{
		res.Version = -1;
		return res;
	}
	if (j.count("Author"))
		res.Author = j["Author"].get<string>();
	if (j.count("ThemeName"))
		res.ThemeName = j["ThemeName"].get<string>();
	if (j.count("LayoutInfo"))
		res.LayoutInfo = j["LayoutInfo"].get<string>();	
	
	return res;
}