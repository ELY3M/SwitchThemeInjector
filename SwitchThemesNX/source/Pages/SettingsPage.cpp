#include "SettingsPage.hpp"
#include "../ViewFunctions.hpp"
#include "../Platform/Platform.hpp"
#include <vector>
#include "../SwitchThemesCommon/MyTypes.h"

using namespace std;

namespace Settings {
	bool UseIcons = true;
	bool UseCommon = true;
	SwitchThemesCommon::LayoutCompatibilityOption HomeMenuCompat = SwitchThemesCommon::LayoutCompatibilityOption::Default;
};

SettingsPage::SettingsPage()
{
	Name = "Settings";
	sysmoduleInstalled = CheckSysmoduleInstalled();
}

#define SYSMODULE_ID "00FF007468656D65"

bool SettingsPage::CheckSysmoduleInstalled()
{
	return fs::Exists(fs::path::FsMitmFolder() + SYSMODULE_ID "/exefs.nsp");
}

void SettingsPage::InstallSysmodule()
{
	try {
		fs::CreateDirectory(fs::path::FsMitmFolder() + SYSMODULE_ID "/flags");
		fs::WriteFile(fs::path::FsMitmFolder() + SYSMODULE_ID "/exefs.nsp", fs::OpenFile(ASSET("sysmodule/ThemeSysmodule.nsp")));
		fs::WriteFile(fs::path::FsMitmFolder() + SYSMODULE_ID "/toolbox.json", fs::OpenFile(ASSET("sysmodule/toolbox.json")));
		fs::WriteFile(fs::path::FsMitmFolder() + SYSMODULE_ID "/flags/boot2.flag", std::vector<u8>{ 'a' });
		fs::WriteFile(fs::path::FsMitmFolder() + SYSMODULE_ID "/ver.txt", std::vector<u8>{ '1' });
		sysmoduleInstalled = true;

		Dialog("The sysmodule has been installed. Restart your console to apply the changes.");
	}
	catch (const std::exception& ex)
	{
		Dialog("Error installing sysmodule: "s + ex.what());
	}
}

void SettingsPage::RemoveSysmodule()
{
	auto path = fs::path::FsMitmFolder() + SYSMODULE_ID "/";

	try {
		if (fs::Exists(path))
			fs::DeleteDirectory(path);

		sysmoduleInstalled = false;

		Dialog("The sysmodule has been uninstalled. Restart your console to apply the changes.");
	}
	catch (const std::exception& ex)
	{
		Dialog("Error uninstalling sysmodule: "s + ex.what());
	}
}

void SettingsPage::Render(int X, int Y)
{
	Utils::ImGuiSetupWin(Name.c_str(), X, Y, DefaultWinFlags);
	ImGui::SetWindowSize(ImVec2(SCR_W - (float)X - 30, SCR_H - (float)Y - 70));
	ImGui::PushFont(font25);

	ImGui::PushFont(font30);
	ImGui::TextUnformatted("Update detection sysmodule");
	ImGui::PopFont();

	ImGui::TextWrapped("There is a new experimental sysmodule that automatically uninstalls themes when the system firmware is updated. This fixes the common crashes caused by incompatible versions of the home menu.");
	if (sysmoduleInstalled)
	{
		ImGui::PushStyleColor(ImGuiCol_Text, Colors::Highlight);
		ImGui::Text("The sysmodule is currently installed.     ");
		ImGui::PopStyleColor();
		ImGui::SameLine();
		if (ImGui::Button("Uninstall"))
			PushFunction([this]() { RemoveSysmodule(); });
	}
	else
	{
		ImGui::PushStyleColor(ImGuiCol_Text, Colors::Red);
		ImGui::Text("The sysmodule is currently not installed.    ");
		ImGui::PopStyleColor();
		ImGui::SameLine();
		if (ImGui::Button("Install now"))
			PushFunction([this]() { InstallSysmodule(); });
	}
	PAGE_RESET_FOCUS;

	ImGui::NewLine();

	ImGui::PushFont(font30);
	ImGui::TextUnformatted("NXTheme settings");
	ImGui::PopFont();

	ImGui::TextWrapped("These settings only apply for installing nxthemes and are not saved, you have to switch them back every time you launch this app");
	ImGui::Checkbox("Enable custom icons", &Settings::UseIcons);
	ImGui::Checkbox("Enable extra layouts (eg. common.szs)", &Settings::UseCommon);

	ImGui::NewLine();
	ImGui::Text("Home menu compatibility options.");
	ImGui::TextWrapped("Changing this could help solve install issues with old themes on latest firmware.");
	ImGui::RadioButton("Decide automatically (default)", (int*)&Settings::HomeMenuCompat, (int)SwitchThemesCommon::LayoutCompatibilityOption::Default);
	ImGui::RadioButton("Force original home menu applet icons (firmware <= 10.0)", (int*)&Settings::HomeMenuCompat, (int)SwitchThemesCommon::LayoutCompatibilityOption::Firmware10);
	ImGui::RadioButton("Force home menu layout with the NS online icon (firmware 11.0)", (int*)&Settings::HomeMenuCompat, (int)SwitchThemesCommon::LayoutCompatibilityOption::Firmware11);
	ImGui::RadioButton("Do not apply compatibility fixes", (int*)&Settings::HomeMenuCompat, (int)SwitchThemesCommon::LayoutCompatibilityOption::DisableFixes);

	ImGui::NewLine();

	ImGui::PopFont();
	Utils::ImGuiSetWindowScrollable();
	Utils::ImGuiCloseWin();
}

void SettingsPage::Update()
{
	if (Utils::PageLeaveFocusInput(true))
	{
		Parent->PageLeaveFocus(this);
		return;
	}
}









