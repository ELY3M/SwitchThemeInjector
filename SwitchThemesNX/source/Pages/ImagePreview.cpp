#include "ImagePreview.hpp"
#include "../ViewFunctions.hpp"

ImagePreview::ImagePreview(ImageRef img, std::string label) : img(img), label(label)
{
	if (!label.size())
		label = "Image preview";
}

void ImagePreview::Render(int X, int Y)
{
	Utils::ImGuiNextFullScreen();
	ImGui::Begin("ImagePreview", nullptr, DefaultWinFlags);

	auto sz = ImGui::CalcTextSize("HelloWorld");
	Utils::ImGuiCenterString(label);

	auto height = SCR_H - sz.y * 3;
	auto ratio = (float)img->Width / img->Height;
	auto width = height * ratio;

	ImGui::SetCursorPosX(SCR_W / 2.0f - width / 2.0f);
	ImGui::Image(img->TextureId, { width, height });

	if (ImGui::IsItemClicked() || Utils::PageLeaveFocusInput())
		PopPage(this);

	Utils::ImGuiCenterString("Tap or press B to go back");

	ImGui::End();
}
