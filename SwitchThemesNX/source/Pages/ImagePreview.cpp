#include "ImagePreview.hpp"
#include "../ViewFunctions.hpp"
#include <format>

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

	/*ImGui::Text("%d, %d %dx%d", 
		img ? 0 : 1,
		img->IsValid() ? 0 : 1,
		img ? img->Width : 0, 
		img ? img->Height : 0);*/

	if (!img || !img->IsValid())
	{
		ImGui::NewLine();
		Utils::ImGuiCenterString("Failed to load image");
		if (UseLowMemory)
			Utils::ImGuiCenterString("You are running in applet mode, try launching with title takeover.");
	}
	else 
	{
		auto height = SCR_H - sz.y * 3;
		auto ratio = (float)img->Width / img->Height;
		auto width = height * ratio;

		if (width > SCR_W)
		{
			ratio = (float)img->Height / img->Width;
			width = SCR_W - 100;
			height = width * ratio;
		}

		ImGui::SetCursorPosX(SCR_W / 2.0f - (int)width / 2.0f);
		ImGui::Image(img->TextureId, ImVec2((int)width, (int)height));
	}

	if (ImGui::IsItemClicked() || Utils::PageLeaveFocusInput())
		PopPage(this);

	Utils::ImGuiCenterString("Tap or press B to go back");

	ImGui::End();
}
