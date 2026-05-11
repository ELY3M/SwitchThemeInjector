#pragma once
#include <iostream>
#include <vector>
#include "../SwitchThemesCommon/SwitchThemesCommon.hpp"
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
	private:
		bool sysmoduleInstalled;

		void InstallSysmodule();
		void RemoveSysmodule();
};
