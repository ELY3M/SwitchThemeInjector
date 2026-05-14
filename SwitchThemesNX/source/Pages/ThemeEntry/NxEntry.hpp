#include "ThemeEntry.hpp"
#include "../../fs.hpp"
#include "../../SwitchTools/PatchMng.hpp"
#include "../../SwitchThemesCommon/NXTheme.hpp"
#include "../../SwitchThemesCommon/Common.hpp"
#include "../../SwitchTools/RomfsCache.hpp"
#include "../../SwitchThemesCommon/MyTypes.h"
#include "../SettingsPage.hpp"
#include <string>
#include <vector>
#include <utility>
#include <variant>
#include <optional>

class NxEntry : public ThemeEntry
{
public:
	NxEntry(const std::string& fileName, std::vector<u8>&& RawData) :
		theme(NxTheme::TryLoad(std::move(RawData)))
	{
		FileName = fileName;
		Initialize();
	}

	NxEntry(const std::string& fileName, FileContainer&& container) :
		theme(NxTheme(std::move(container)))
	{
		FileName = fileName;
		Initialize();
	}

	bool IsFolder() override { return false; }
	bool CanInstall() override { return _CanInstall; }
	bool HasPreview() override { return _HasPreview; }
protected:

	bool DoInstall(bool ShowDialogs = true) override
	{
		if (!TargetInfo || !_CanInstall)
			return false;

		if (!PatchMng::ExefsCompatAsk(fs::GetFileName(TargetInfo->SzsFile)))
			return false;

		auto targetPart = theme.manifest->Target;
		const std::vector<u8>& baseSzs = RomfsCache::GetFile(*TargetInfo);

		if (ShowDialogs)
			ThemeEntry::DisplayInstallDialog(FileName);

		const std::string CommonDestPath = fs::path::RomfsFolder("0100000000001000") + "lyt/common.szs";
		// If we're installing a resident menu theme delete the common.szs file that could be a leftover from a previous theme
		if (targetPart == "home" && fs::Exists(CommonDestPath))
			fs::Delete(CommonDestPath);

		// common.szs is patched in the following cases: 
		//	On <= 5.0 apply the background image for the applets
		bool ShouldPatchBGInCommon = hos::Version.major <= 5 && (targetPart == "news" || targetPart == "apps" || targetPart == "set");
		//	If we have a common layout and it's enabled in the settings 
		bool HasCommonLayout = targetPart == "home" && theme.HasCommonLayout() && Settings::UseCommon;

		if (HasCommonLayout || ShouldPatchBGInCommon)
		{
			const std::vector<u8>& commonSzs = RomfsCache::GetFile(ThemeTargetInfo::QlaunchCommon);

			SARC::SarcData sarc;
			if (!SarcOpen(commonSzs, &sarc))
				return false;

			SwitchThemesCommon::SzsPatcher Patcher(sarc);

			if (ShouldPatchBGInCommon)
			{
				if (auto image = GetBackgroundImage())
					if (!PatchBG(Patcher, *image, "common.szs"))
						return false;
			}

			if (HasCommonLayout)
			{
				if (!PatchLayout(Patcher, theme.GetCommonLayout(), "common.szs"))
					return false;
			}

			fs::theme::CreateStructure("0100000000001000");

			auto pack = SarcPack(Patcher.GetFinalSarc());
			fs::WriteFile(CommonDestPath, pack);

			fs::theme::WriteSystemVersionFile();
		}

		//Actual file patching code 
		bool FileHasBeenPatched = false;
		SARC::SarcData sarc;

		if (!SarcOpen(baseSzs, &sarc))
			return false;

		SwitchThemesCommon::SzsPatcher Patcher(sarc);

		std::string ContentID = TargetInfo->StringContentId();
		std::string SzsName = fs::GetFileName(TargetInfo->SzsFile);

		auto patch = Patcher.DetectedSarc();

		if (!ShouldPatchBGInCommon)
		{
			if (!patch)
			{
				DialogBlocking("Couldn't find any patch for " + SzsName + "\nThe theme was not installed");
				return false;
			}

			if (auto image = GetBackgroundImage()) 
			{
				if (!PatchBG(Patcher, *image, SzsName))
					return false;

				FileHasBeenPatched = true;
			}
		}

		/*
			The layout patching step has been moved after the custom user icons (and future home menu components)
			to let layouts edit the built-in patches that are applied to the panes. To avoid breaking old layouts
			patches from pre 9 nxthemes will still be applied first
		*/
		const auto applyLayoutPatch = [&]() {
			if (theme.HasMainLayout())
			{
				if (!PatchLayout(Patcher, theme.GetMainLayout(), targetPart))
					return false;

				FileHasBeenPatched = true;
			}
			else if (
				Settings::HomeMenuCompat == SwitchThemesCommon::LayoutCompatibilityOption::Firmware10 ||
				Settings::HomeMenuCompat == SwitchThemesCommon::LayoutCompatibilityOption::Firmware11
				)
			{
				// Special case: this theme has no layout but the user requested to force compatibility fixes
				if (!PatchLayout(Patcher, "", targetPart))
					return false;

				FileHasBeenPatched = true;
			}

			return true;
			};

		if (theme.manifest->Version <= 8)
		{
			if (!applyLayoutPatch())
				return false;
		}

		//Applet icons patching
		if (Settings::UseIcons)
		{
			if (theme.manifest->Version >= 8)
			{
				//New applet texture patching method
				if (Settings::UseIcons && Patches::textureReplacement::NxNameToList.count(targetPart))
				{
					for (const TextureReplacement& p : Patches::textureReplacement::NxNameToList[targetPart])
					{
						if (!theme.HasImagePart(p.NxThemeName))
							continue;

						auto data = theme.GetImagePart(p.NxThemeName, p.W, p.H);
						if (std::holds_alternative<std::string>(data))
						{
							DialogBlocking("Failed to convert the image for: " + p.NxThemeName + "\n" + std::get<std::string>(data));
						}
						else
						{
							auto patched = Patcher.PatchAppletIcon(std::get<FileData>(data), p.NxThemeName);

							if (!patched)
								DialogBlocking(p.NxThemeName + " icon patch failed for " + SzsName + "\nThe theme will be installed anyway but may crash.");
							else
								FileHasBeenPatched = true;
						}
					}
				}
			}
			else
			{
				//Old album.szs patching to avoid breaking old themes
				if (targetPart == "home" && theme.files.count("album.dds"))
				{
					FileHasBeenPatched = true;
					if (!Patcher.PatchBntxTexture(theme.files.at("album.dds"), { "RdtIcoPvr_00^s" }, 0x02000000))
						DialogBlocking("Album icon patch failed for " + SzsName + "\nThe theme will be installed anyway but may crash.");
				}
			}
		}

		if (theme.manifest->Version >= 9)
		{
			if (!applyLayoutPatch())
				return false;
		}

		if (FileHasBeenPatched)
		{
			fs::theme::CreateStructure(ContentID);

			auto sarc = SarcPack(Patcher.GetFinalSarc());
			fs::WriteFile(fs::path::RomfsFolder(ContentID) + "lyt/" + SzsName, sarc);
		}

		if (TargetInfo->TitleId == ThemeTargetInfo::QlaunchID)
			fs::theme::WriteSystemVersionFile();

		return true;
	}

	LoadedImage GetPreview() override
	{
		if (!_HasPreview) return 0;
		auto image = GetBackgroundImage();
		if (!image) return 0;

		auto Preview = ImageCache::LoadDDS(*image, FileName);
		if (!Preview)
		{
			_HasPreview = false;
			DialogBlocking("Failed to load the preview image");
		}

		return Preview;
	}

private:
	bool _CanInstall = true;
	bool _HasPreview = false;
	NxTheme theme;
	const ThemeTargetInfo* TargetInfo = nullptr;

	std::optional<FileData> GetBackgroundImage()
	{
		if (!theme.HasMainImage()) 
			return std::nullopt;

		auto image = theme.GetMainImage();
		if (std::holds_alternative<std::string>(image))
		{
			_HasPreview = false;
			_CanInstall = false;

			lblLine2 = std::get<std::string>(image);
			CannotInstallReason = "Failed to load the background image: " + lblLine2;
			DialogBlocking(CannotInstallReason);

			return std::nullopt;
		}

		return std::get<FileData>(image);
	}

	void Initialize()
	{
		// In case of errors, initialize the first line with the file name.
		// If not needed this will be overwritten later
		lblLine1 = FileName;

		if (!theme.IsValid())
		{
			lblLine2 = "Invalid theme";
			CannotInstallReason = theme.error.value();
			_CanInstall = false;
			return;
		}

		if (theme.manifest->Version > SwitchThemesCommon::NXThemeVer)
		{
			lblLine2 = "New version, update the installer !";
			CannotInstallReason = "This theme requres a newer version of the theme installer. Download latest version from GitHub.";
			_CanInstall = false;
			return;
		}

		_HasPreview = theme.HasMainImage();
		TargetInfo = ThemeTargetInfo::Find(theme.manifest->Target);

		if (!TargetInfo)
		{
			lblLine2 = "Error: invalid target";
			CannotInstallReason = "The target home menu part is not valid";
			_CanInstall = false;
			return;
		}

		lblFname = theme.manifest->ThemeName;
		lblLine2 = TargetInfo->PartName;
		if (_HasPreview)
			lblLine2 += " - press X for preview";

		lblLine1 = "";
		if (theme.manifest->Author != "")
			lblLine1 += "by " + theme.manifest->Author;

		if (theme.manifest->LayoutInfo != "")
			lblLine1 += " - " + theme.manifest->LayoutInfo;

		// In case all metadata is missing
		if (lblLine1 == "")
			lblLine1 = FileName;
	}

	static bool PatchBG(SwitchThemesCommon::SzsPatcher& Patcher, const std::vector<u8>& data, const std::string& SzsName)
	{
		if (!Patcher.PatchMainBG(data))
		{
			DialogBlocking("PatchBntx failed for " + SzsName + "\nThe theme was not installed");
			return false;
		}
		return true;
	}

	bool PatchLayout(SwitchThemesCommon::SzsPatcher& Patcher, const std::string_view JSON, const std::string& PartName)
	{
		Patcher.CompatFixes = Settings::HomeMenuCompat;
		bool success = false;

		// Special case, apply compat fixes to old themes that have no layout if manually requested by the user
		if (JSON.empty())
		{
			success = Patcher.PatchLayouts();
		}
		else
		{
			auto patch = Patches::LoadLayout(JSON);
			success = Patcher.PatchLayouts(patch, PartName);
		}

		if (!success)
		{
			DialogBlocking("PatchLayouts failed for " + PartName + "\nThe theme was not installed");
			return false;
		}

		if (Patcher.TotalNonCompatibleFixes > 0)
		{
			AppendInstallMessage("The theme \"" + lblFname + "\" for " + PartName + " contained a custom layout that was not compatible with your current firmware, the problematic parts were automatically removed and might change the look of the theme.");
		}

		return true;
	}

	static inline bool SarcOpen(const std::string& path, SARC::SarcData* out)
	{
		auto f = fs::OpenFile(path);
		if (f.size() == 0) return false;
		f = Yaz0::Decompress(f);
		*out = SARC::Unpack(f);
		return true;
	}

	static inline bool SarcOpen(const std::vector<u8>& data, SARC::SarcData* out)
	{
		if (data.size() == 0) return false;
		auto f = Yaz0::Decompress(data);
		*out = SARC::Unpack(f);
		return true;
	}

	static inline std::vector<u8> SarcPack(SARC::SarcData& data)
	{
		auto packed = SARC::Pack(data);
		return Yaz0::Compress(packed.data, 3, packed.align);
	}
};