#pragma once
#include <vector>
#include <string>
#include "../UI/UI.hpp"

class CfwSelectPage : public IUIControlObj
{
	public:
		CfwSelectPage(const std::vector<std::string> &folders);	
		~CfwSelectPage();
		
		void Render(int X, int Y) override;
		void Update() override;
	private:		
		std::vector<std::string> Folders;
};