#pragma once
#include "../../UI/UI.hpp"
#include <string>
#include <vector>
#include <tuple>
#include <span>

class InstallImageDialog : public IUIControlObj
{
public:
	InstallImageDialog(ImageRef previer, const std::vector<u8>& data, bool resizeWarning, bool* outSuccess);

	void Render(int X, int Y) override;
	void Update() override {};
private:
	bool resizeWarning;
	ImageRef img;
	std::span<const u8> data;
	bool* outSuccess;

	std::vector<std::tuple<std::string, std::string>> targetParts;
};
