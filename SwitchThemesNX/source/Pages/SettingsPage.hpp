#pragma once
#include "../SwitchThemesCommon/Patcher.hpp"
#include "../UI/UI.hpp"

namespace Settings {
	extern bool UseIcons;
	extern bool UseCommon;
	extern SwitchThemesCommon::LayoutCompatibilityOption HomeMenuCompat;
};

class SettingsPage : public IPage
{
	public:
		SettingsPage();	
		
		void Render(int X, int Y) override;
		void Update() override;

		static bool CheckSysmoduleInstalled();
		static bool InstallSysmodule();
		static bool RemoveSysmodule(bool dialogs);
	private:
		static bool sysmoduleInstalled;
		static bool canInstallSysmodule;
};
