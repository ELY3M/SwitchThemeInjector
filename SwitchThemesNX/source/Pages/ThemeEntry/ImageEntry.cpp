#include <string>
#include <vector>
#include <memory>
#include <utility>
#include "ThemeEntry.hpp"
#include "ImageEntry.hpp"
#include "../../SwitchThemesCommon/MyTypes.h"
#include "../../SwitchThemesCommon/Bntx/DDS_conversion.hpp"
#include "../../SwitchThemesCommon/Common.hpp"
#include "../../SwitchThemesCommon/NxTheme.hpp"
#include "../../UI/UI.hpp"
#include "../../fs.hpp"
#include "../../ViewFunctions.hpp"

ImageEntry::ImageEntry(const std::string& fileName, std::vector<u8>&& RawData)
{
	lblFname = fs::GetFileName(fileName);
	lblLine1 = fileName;
	lblLine2 = "Image file";
	_originalData = std::move(RawData);
}

void ImageEntry::PerformConversion()
{
	if (_previewImage)
		return;

	auto dds = DDSConv::ConvertImage(_originalData, false, 1280, 720, true);
	_originalData.clear();

	if (dds.ErrorMessage.size())
	{
		CannotInstallReason = dds.ErrorMessage;
		_previewImage = std::make_shared<RenderImage>();
		lblLine1 = "Error loading file";
	}
	else
	{
		_convertedDds = std::move(dds.Data);
		_resizeWarning = dds.resized;
		_previewImage = std::make_shared<RenderImage>(_convertedDds);
	}
}

ImageRef ImageEntry::GetConvertedImage()
{
	PerformConversion();
	return _previewImage;
}

bool ImageEntry::DoInstall(bool ShowDialogs)
{
	PerformConversion();

	if (!CanInstall() || !_previewImage || _convertedDds.empty())
		return false;

	bool result;
	PushPageBlocking(new InstallImageDialog(_previewImage, _convertedDds, _resizeWarning, &result));

	return result;
}

InstallImageDialog::InstallImageDialog(ImageRef previer, const std::vector<u8>& data, bool resizeWarning, bool* outSuccess) :
	img(previer), data(data), resizeWarning(resizeWarning), outSuccess(outSuccess)
{
	*outSuccess = false;
	targetParts = 
	{
		{ "Home menu",		"home"},
		{ "Lock screen",	"lock"},
		{ "All apps menu",	"apps"},
		{ "Settings applet","set"},
		{ "News applet",	"news"},
		{ "User page",		"user"},
		{ "Player selection", "psl"},
	};
}

void InstallImageDialog::Render(int X, int Y)
{
	Utils::ImGuiNextFullScreen();
	ImGui::Begin("ThemeInstall", nullptr, DefaultWinFlags);

	ImGui::PushFont(font40);
	Utils::ImGuiCenterString("Set theme wallpaper");
	ImGui::PopFont();

	if (resizeWarning)
	{
		ImGui::PushStyleColor(ImGuiCol_Text, Colors::Red);
		Utils::ImGuiCenterString("This image was automatically resized.");
		Utils::ImGuiCenterString("For optimal resuls only use images with 720p resulution (1280x720 pixels).");
		ImGui::PopStyleColor();
	}

	Utils::ImGuiCenterString("Select where you want to apply this image");

	auto startY = ImGui::GetCursorPosY();
	auto padding = ImGui::GetStyle().ItemSpacing.x * 2;

	// Three paddings: left, image to list, list to right
	auto previewWidth = 2 * (SCR_W - padding * 3) / 3.0f;
	auto previewRatio = (float)img->Height / img->Width;
	auto previewHeight = previewWidth * previewRatio;

	ImGui::SetCursorPosX(padding);
	ImGui::Image(img->TextureId, { previewWidth, previewHeight });

	ImGui::SetCursorPosY(startY);

	bool first = true;
	for (const auto& [label, part] : targetParts)
	{
		ImGui::SetCursorPosX(previewWidth + padding * 2);
		ImGui::Selectable(label.c_str(), false, ImGuiSelectableFlags_DontClosePopups);
	}

	if (ImGui::GetFocusID() == 0)
		ImGui::SetFocusID(ImGui::GetID("Home menu"), ImGui::GetCurrentWindow());

	ImGui::NewLine();
	ImGui::NewLine();
	
	ImGui::SetCursorPosX(previewWidth + padding * 2);
	if (ImGui::Selectable("Cancel", false, ImGuiSelectableFlags_DontClosePopups))
		PopPage(this);

	ImGui::End();
}