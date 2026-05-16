#include <string>
#include <vector>
#include <memory>
#include <utility>
#include <format>
#include "ThemeEntry.hpp"
#include "ImageEntry.hpp"
#include "../../SwitchThemesCommon/MyTypes.h"
#include "../../SwitchThemesCommon/Bntx/DDS_conversion.hpp"
#include "../../SwitchThemesCommon/Common.hpp"
#include "../../SwitchThemesCommon/NXTheme.hpp"
#include "../../UI/UI.hpp"
#include "../../fs.hpp"
#include "../../ViewFunctions.hpp"
#include <string_view>

namespace 
{
	FileData BuildManifest(const std::string& target) 
	{
		std::string msg = std::format(
			R"({{
				"Target": "{}",
				"ThemeName": "New Theme",
				"Version": {}
			}})",
			target, SwitchThemesCommon::NXThemeVer);

		return FileData(msg.begin(), msg.end());
	}
}

ImageEntry::ImageEntry(const std::string& fileName, std::vector<u8>&& RawData)
{
	FileName = fileName;
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
	PushPageBlocking(new InstallImageDialog(_previewImage, _convertedDds, _resizeWarning, ShowDialogs, &result));

	return result;
}

InstallImageDialog::InstallImageDialog(ImageRef preview, const std::vector<u8>& ddsImage, bool resizeWarning, bool showInstallDialogs, bool* outSuccess) :
	previewImage(preview), ddsImage(ddsImage), resizeWarning(resizeWarning), showInstallDialogs(showInstallDialogs), outSuccess(outSuccess)
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

ImageRef InstallImageDialog::LoadOverlayPart(const std::string& part)
{
	if (partOverlay.count(part))
		return partOverlay.at(part);

	ImageRef res = nullptr;

	auto path = ASSET("preview/") + part + ".png";
	try 
	{
		if (fs::Exists(path))
		{
			auto image = fs::OpenFile(path);
			res = std::make_shared<RenderImage>(image);
		}	
	}
	catch(const std::exception& ex)
	{
		LOGf("%s", ex.what());
	}

	partOverlay[part] = res;
	return res;
}

void InstallImageDialog::ApplyToPart(const std::string& part)
{
	DisplayLoading("Installing...");

	// Hacky impl: build an nxtheme in memory and start the installation process
	FileContainer files =
	{
		{"info.json", BuildManifest(part) },
		{"image.dds", FileData(ddsImage.begin(), ddsImage.end()) },
	};

	auto entry = NxEntry("theme", std::move(files));
	if (!entry.CanInstall())
	{
		Dialog("Failed to build the theme file. Open an issue on github.\n" + entry.CannotInstallReason);
		return;
	}

	*outSuccess = entry.Install(showInstallDialogs);
	PopPage(this);
}

void InstallImageDialog::Render(int X, int Y)
{
	Utils::ImGuiNextFullScreen();
	ImGui::Begin("ThemeInstall", nullptr, DefaultWinFlags);

	ImGui::NewLine();
	ImGui::PushFont(font40);
	Utils::ImGuiCenterString("Set theme wallpaper");
	ImGui::PopFont();

	if (resizeWarning)
	{
		ImGui::PushStyleColor(ImGuiCol_Text, Colors::Red);
		ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(0, 0));
		Utils::ImGuiCenterString("This image was automatically resized.");
		Utils::ImGuiCenterString("For optimal resuls only use images with 720p resulution (1280x720 pixels).");
		ImGui::NewLine();
		ImGui::PopStyleVar();
		ImGui::PopStyleColor();
	}

	Utils::ImGuiCenterString("Select where you want to apply this image");
	ImGui::NewLine();

	auto startY = ImGui::GetCursorPosY();
	auto padding = ImGui::GetStyle().ItemSpacing.x * 4;

	// Three paddings: left, image to list, list to right
	auto previewWidth = 2 * (SCR_W - padding * 3) / 3.0f;
	auto previewRatio = (float)previewImage->Height / previewImage->Width;
	auto previewHeight = previewWidth * previewRatio;

	ImGui::SetCursorPosX(padding);
	ImGui::Image(previewImage->TextureId, { previewWidth, previewHeight });

	ImGui::SetCursorPosY(startY);

	auto itemStart = previewWidth + padding * 2;
	auto itemSize = ImVec2(SCR_W - itemStart - padding, 0);

	const std::string* currentPart = nullptr;
	for (const auto& [label, part] : targetParts)
	{
		ImGui::SetCursorPosX(itemStart);
		
		auto id = ImGui::GetID(label.c_str());
		if (ImGui::Selectable(label.c_str(), false, ImGuiSelectableFlags_DontClosePopups, itemSize))
		{
			PushFunction([this, part]()
			{
				ApplyToPart(part);
			});
		}

		if (ImGui::GetFocusID() == id)
			currentPart = &part;
	}

	if (ImGui::GetFocusID() == 0)
		ImGui::SetFocusID(ImGui::GetID("Home menu"), ImGui::GetCurrentWindow());

	if (currentPart)
	{
		auto overlay = LoadOverlayPart(*currentPart);
		if (overlay && overlay->IsValid())
		{
			ImGui::SetCursorPosY(startY);
			ImGui::SetCursorPosX(padding);
			ImGui::Image(overlay->TextureId, { previewWidth, previewHeight });
		}
	}
		
	auto textSize = ImGui::CalcTextSize("Cancel");

	ImGui::SetCursorPosY(startY + previewHeight - textSize.y);
	ImGui::SetCursorPosX(previewWidth + padding * 2);
	
	if (ImGui::Selectable("Cancel", false, ImGuiSelectableFlags_DontClosePopups, itemSize))
		PopPage(this);

	if (Utils::PageLeaveFocusInput(false))
		PopPage(this);

	ImGui::End();
}