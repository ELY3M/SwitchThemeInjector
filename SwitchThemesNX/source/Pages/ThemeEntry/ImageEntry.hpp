#pragma once
#include "../../UI/UI.hpp"
#include <string>
#include <vector>
#include <tuple>
#include <span>

class InstallImageDialog : public IUIControlObj
{
public:
	InstallImageDialog(ImageRef preview, const std::vector<u8>& ddsImage, bool resizeWarning, bool* outSuccess);

	void Render(int X, int Y) override;
	void Update() override {};
private:
	std::vector<std::tuple<std::string, std::string>> targetParts;

	bool resizeWarning;
	ImageRef previewImage;
	std::span<const u8> ddsImage;
	bool* outSuccess;

	void ApplyToPart(const std::string& part);
};
