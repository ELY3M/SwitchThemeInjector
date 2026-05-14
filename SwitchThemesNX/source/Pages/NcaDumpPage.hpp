#pragma once
#include "../UI/UI.hpp"

class NcaDumpPage : public IPage
{
	public:
		NcaDumpPage();	
		
		void Render(int X, int Y) override;
		void Update() override;
};