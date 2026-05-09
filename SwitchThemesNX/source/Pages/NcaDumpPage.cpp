#include "NcaDumpPage.hpp"
#include "../ViewFunctions.hpp"

#include "../SwitchTools/hactool.hpp"
#include "../SwitchTools/RomFsCache.hpp"
#include "../fs.hpp"
#include "../SwitchThemesCommon/NXTheme.hpp"

static void WriteExtracted(const std::string& basePath, const RomfsCache::Dir& files)
{
	for (const auto& [name, data] : files)
	{
		auto finalPath = fs::path::SystemDataFolder + fs::JoinPath(basePath, name);
		auto parent = fs::GetParentDir(finalPath);
		fs::CreateDirectory(parent);
		fs::WriteFile(finalPath, data);
	}
}

NcaDumpPage::NcaDumpPage()
{
	Name = "Extract home menu";
}

void NcaDumpPage::Render(int X, int Y)
{
	Utils::ImGuiSetupPage(this, X, Y);
	ImGui::PushFont(font30);

	ImGui::TextWrapped(
		"This page allows extracting the RomFS of the home menu and related titles.\n\n"
		"For regular usage of the theme installer you do not need this feature, it is done automaticaly when installing a theme."
	);

	ImGui::NewLine();

	if (ImGui::Button("Extract layout files"))
	{
		PushFunction([]() 
		{
			try 
			{
				if (fs::Exists(fs::path::SystemDataFolder + "extracted/"))
					fs::RecursiveDeleteFolder(fs::path::SystemDataFolder + "extracted/");

				DisplayLoading("Extracting qlaunch...");
				WriteExtracted("extracted/qlaunch", RomfsCache::GetContent(ThemeTargetInfo::QlaunchID));

				DisplayLoading("Extracting playerselect...");
				WriteExtracted("extracted/playerselect", RomfsCache::GetContent(ThemeTargetInfo::PslID));

				DisplayLoading("Extracting mypage...");
				WriteExtracted("extracted/mypage", RomfsCache::GetContent(ThemeTargetInfo::UserPageID));

				Dialog("The files have been extracted to the themes/systemData/extracted/ folder on your SD card.");
			}
			catch (const std::exception& ex)
			{
				DialogBlocking("Error while extracting the layout files: " + std::string(ex.what()));
			}
		});
	}

	if (ImGui::Button("Extract raw NCA files"))
	{
		PushFunction([]()
			{
				try
				{
					if (fs::Exists(fs::path::SystemDataFolder + "nca/"))
						fs::RecursiveDeleteFolder(fs::path::SystemDataFolder + "nca/");

					fs::CreateDirectory(fs::path::SystemDataFolder + "nca/");

					auto nca = hactool::GetNca(ThemeTargetInfo::QlaunchID);
					fs::WriteFile(fs::path::SystemDataFolder + "nca/" + ThemeTargetInfo::TitleIdToString(ThemeTargetInfo::QlaunchID) + ".nca", nca);
					
					nca = hactool::GetNca(ThemeTargetInfo::PslID);
					fs::WriteFile(fs::path::SystemDataFolder + "nca/" + ThemeTargetInfo::TitleIdToString(ThemeTargetInfo::PslID) + ".nca", nca);

					nca = hactool::GetNca(ThemeTargetInfo::UserPageID);
					fs::WriteFile(fs::path::SystemDataFolder + "nca/" + ThemeTargetInfo::TitleIdToString(ThemeTargetInfo::UserPageID) + ".nca", nca);

					Dialog("The files have been extracted to the themes/systemData/nca folder on your SD card.");
				}
				catch (const std::exception& ex)
				{
					DialogBlocking("Error while extracting the layout files: " + std::string(ex.what()));
				}
			});
	}
		
	ImGui::PopFont();
	Utils::ImGuiCloseWin();
}

void NcaDumpPage::Update()
{	
	if (Utils::PageLeaveFocusInput()){
		Parent->PageLeaveFocus(this);
	}
}



