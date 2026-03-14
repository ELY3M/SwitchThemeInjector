using SARCExt;
using SwitchThemes.Common;
using SwitchThemes.Common.Images;
using SwitchThemes.Common.Patching;

namespace NxThemeTool
{
    public class ThemeApply(NxTheme Theme, IContentProvider Szs) : IDisposable
    {
        readonly NxTheme Theme = Theme;
        readonly IContentProvider Szs = Szs;

        public LayoutCompatibilityOption Compatibility = LayoutCompatibilityOption.Default;

        public static ThemeApply FromFiles(string nxtheme, string szs, ProcessResult result)
        {
            using var source = ProviderHelper.OpenFor(nxtheme);
            return new ThemeApply(new NxTheme(source, result), ProviderHelper.OpenFor(szs));
        }

        public void Apply(IContentWriter writer, ProcessResult result)
        {
            var info = CommonInfo.GetPart(Theme.Manifest.Target);
            if (info == null)
            {
                result.Err("ApplyPart", $"Part {Theme.Manifest.Target} is not recognized and will be skipped.");
                return;
            }

            var path = $"{info.TitleId}/{info.SzsName}";
            try
            {
                var szs = SARC.Unpack(ManagedYaz0.Decompress(Szs.GetFile(path)));
                ApplyPart(info, szs, result);
                writer.WriteFile(path, ManagedYaz0.Compress(SARC.Pack(szs).Item2));
            }
            catch (Exception ex)
            {
                result.Err(Theme.Manifest.Target, $"An error occurred while applying part: {ex.Message}");
            }

            if (info.AllowCommon && Theme.CommonLayoutFile is not null)
            {
                path = $"{info.TitleId}/common.szs";
                try
                {
                    var szs = SARC.Unpack(ManagedYaz0.Decompress(Szs.GetFile(path)));
                    ApplyCommonJson(szs, result);
                    writer.WriteFile(path, ManagedYaz0.Compress(SARC.Pack(szs).Item2));
                }
                catch (Exception ex)
                {
                    result.Err(Theme.Manifest.Target, $"An error occurred while applying part: {ex.Message}");
                }
            }
        }

        void ApplyCommonJson(SarcData szs, ProcessResult result)
        {
            var patcher = new SzsPatcher(szs)
            {
                // Currently we don't provide any fixes for common layouts
                CompatFixes = LayoutCompatibilityOption.DisableFixes
            };

            patcher.PatchLayouts(Theme.CommonLayoutFile!, "");
        }

        void ApplyPart(PatchPartInfo info, SarcData szs, ProcessResult result)
        {
            var patcher = new SzsPatcher(szs)
            {
                CompatFixes = Compatibility
            };

            bool hasImage = false;

            if (Theme.MainImageFile is not null)
            {
                var imageFormat = ImageUtil.DetectFormat(Theme.MainImageFile);

                if (imageFormat != ImageFormat.Dds)
                {
                    result.Err("image", "This tool can only apply DDS images");
                }
                else
                {
                    if (!patcher.PatchMainBG(Theme.MainImageFile))
                        result.Err("image", "Failed to patch main background image.");
                    else
                        hasImage = true;
                }
            }

            if (TextureReplacement.NxNameToList.TryGetValue(Theme.Manifest.Target, out var textures))
            {
                foreach (var texture in textures)
                {
                    var texName = texture.NxThemeName;

                    if (Theme.Extra.ContainsKey(texName + ".dds"))
                    {
                        if (!patcher.PatchAppletIcon2(Theme.Extra[texName + ".dds"], texture))
                        {                           
                            result.Err(texName, "Failed to patch texture.");
                        }
                        else
                        {
                            hasImage = true;
                        }
                    }
                    else if (Theme.Extra.ContainsKey(texName + ".png"))
                    {
                        result.Err(texName, "This tool can apply only DDS images");
                    }
                }
            }

            if (Theme.MainLayoutFile is not null)
            {
                patcher.PatchLayouts(Theme.MainLayoutFile);
            }

            if (hasImage)
                patcher.FinalizeBntx();
        }

        public void Dispose()
        {
            Szs.Dispose();
        }
    }
}
