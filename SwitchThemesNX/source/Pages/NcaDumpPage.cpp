#include "NcaDumpPage.hpp"
#include "../ViewFunctions.hpp"

using namespace std;

NcaDumpPage::NcaDumpPage()
{
	Name = "Extract home menu";
}

void NcaDumpPage::Render(int X, int Y)
{
	Utils::ImGuiSetupPage(this, X, Y);
	ImGui::PushFont(font30);

	ImGui::TextWrapped(
		"Older versions of the theme installer required you to manually extract the home menu RomFS data.\n"
		"This is now automatic and not needed anymore, any guide asking for this is outdated.");
		
	ImGui::PopFont();
	Utils::ImGuiCloseWin();
}

void NcaDumpPage::Update()
{	
	if (Utils::PageLeaveFocusInput()){
		Parent->PageLeaveFocus(this);
	}
}



