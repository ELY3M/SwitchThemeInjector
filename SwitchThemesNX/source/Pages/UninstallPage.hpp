#pragma once
#include "../UI/UI.hpp"

class UninstallPage : public IPage
{
	private:
		ImGuiID firstBtn = 0;
	public:
		UninstallPage();	
		
		void Render(int X, int Y) override;
		void Update() override;
};