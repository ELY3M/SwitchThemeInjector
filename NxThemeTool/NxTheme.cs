using SARCExt;
using SwitchThemes.Common;
using SwitchThemes.Common.Images;
using System.Text;

namespace NxThemeTool
{
    public class NxTheme
    {
        public readonly ThemeFileManifest Manifest;

        public byte[]? MainImageFile;
        public LayoutPatch? MainLayoutFile;
        public LayoutPatch? CommonLayoutFile;
        public byte[]? PreviewFile;

        // Indexed by NxThemeName without file extension
        public readonly Dictionary<string, byte[]> Extra = new();

        public NxTheme(byte[] data, ProcessResult? validation) : this(ProviderHelper.OpenFor(data), validation, false) { }

        public NxTheme(ThemeFileManifest manifest)
        {
            Manifest = manifest;

            if (CommonInfo.GetPart(manifest.Target) == null)
                throw new InvalidOperationException($"The specified theme part is not valid: {manifest.Target}");
        }

        public static NxTheme CreateNew(string target) => new(new ThemeFileManifest
        {
            Target = target,
            ThemeName = "New Theme",
            Author = "",
            Id = Guid.NewGuid().ToString("N").Substring(0, 8),
            Version = CommonInfo.NxThemeFormatVersion
        });

        public NxTheme(IContentProvider sarc, ProcessResult? validation, bool leaveOpen = true)
        {
            using var _ = leaveOpen ? null : sarc;

            if (!sarc.HasFile("info.json"))
                throw new ArgumentException("Content provider does not contain a manifest.json file.");

            Manifest = ThemeFileManifest.Deserialize(sarc.GetString("info.json"));

            if (sarc.HasFile("image.dds"))
                MainImageFile = sarc.GetFile("image.dds");
            else if (sarc.HasFile("image.jpg"))
                MainImageFile = sarc.GetFile("image.jpg");

            if (sarc.HasFile("layout.json"))
                MainLayoutFile = LayoutPatch.Load(sarc.GetString("layout.json"));

            if (sarc.HasFile("common.json"))
                CommonLayoutFile = LayoutPatch.Load(sarc.GetString("common.json"));

            // The casing was never properly standardized so check for both
            if (sarc.HasFile("Preview.png") || sarc.HasFile("preview.png"))
                PreviewFile = sarc.GetFile(sarc.HasFile("Preview.png") ? "Preview.png" : "preview.png");

            if (TextureReplacement.NxNameToList.TryGetValue(Manifest.Target, out var replacement))
            {
                var allowed = replacement.Select(x => x.NxThemeName).ToArray();

                foreach (var name in allowed)
                {
                    if (sarc.HasFile(name + ".dds"))
                        Extra.Add(name, sarc.GetFile(name + ".dds"));
                    else if (sarc.HasFile(name + ".png"))
                        Extra.Add(name, sarc.GetFile(name + ".png"));
                }
            }

            if (validation is not null)
                Validate(validation);
        }

        public void Pack(IContentWriter writer)
        {
            writer.WriteFile("info.json", Encoding.UTF8.GetBytes(Manifest.Serialize(true)));

            if (MainImageFile != null)
            {
                var extension = ImageUtil.DetectImageExtension(MainImageFile);
                writer.WriteFile("image." + extension, MainImageFile);
            }

            if (MainLayoutFile != null)
                writer.WriteFile("layout.json", MainLayoutFile.AsByteArray());

            if (CommonLayoutFile != null)
                writer.WriteFile("common.json", CommonLayoutFile.AsByteArray());

            if (PreviewFile != null)
                writer.WriteFile("preview.png", PreviewFile);

            foreach (var icon in Extra)
            {
                var extension = ImageUtil.DetectImageExtension(icon.Value);
                writer.WriteFile(icon.Key + "." + extension, icon.Value);
            }
        }

        public void Validate(ProcessResult validation)
        {
            // Manifest checks
            if (string.IsNullOrWhiteSpace(Manifest.ThemeName))
                validation.Err("manifest.json", "Theme name cannot be empty.");

            if (string.IsNullOrWhiteSpace(Manifest.Author))
                validation.Warn("manifest.json", "Consider setting the theme author.");

            if (string.IsNullOrWhiteSpace(Manifest.Id) || Manifest.Id.Length < 8)
                validation.Warn("manifest.json", "Consider setting setting a unique ID for your theme.");

            if (Manifest.Version > CommonInfo.NxThemeFormatVersion)
                validation.Err("manifest.json", $"The theme format version is newer than the one this program supports. The output might be wrong. This build supports up to {CommonInfo.NxThemeFormatVersion}");

            var target = CommonInfo.GetPart(Manifest.Target);
            if (target is null)
            {
                validation.Err("manifest.json", "Unknown target name. Make sure the part name is correct and matches one of the parts defined in the documentation.");
                return;
            }

            if (PreviewFile != null)
            {
                try
                { 
                    var image = ImageUtil.ParsePng(PreviewFile);
                    if (image.Size.Width != 1280 || image.Size.Height != 720)
                        validation.Err("preview.png", $"Invalid preview image size {image.Size}. The preview image must be 1280x720 pixels.");
                }
                catch (Exception ex)
                {
                    validation.Err("preview.png", "Invalid preview image format. Supported format is: png. Error:" + ex.Message);
                }
            }

            if (CommonLayoutFile != null && !target.AllowCommon)
            {
                validation.Err("common.json", "The common layout patch is not supported for the " + target.Name + " theme part.");
            }

            if (MainImageFile != null)
            {
                try
                {
                    var image = ImageUtil.ParseImage(MainImageFile);

                    if (image is JpgInfo && !image.IsValidForBg)
                        validation.Err("image", $"The provided JPG image uses progressive encoding. This is not support and will fail to install.");

                    if (image is DDS dds && !dds.IsValidForBg)
                        validation.Err("image", $"The provided DDS image can't be used for wallpapers. Only DXT1 encoded images are valid.");

                    if (image is PngInfo)
                        validation.Err("image", $"Png images are not supported for wallpapers.");

                    if (image.Size.Width != 1280 || image.Size.Height != 720)
                        validation.Err("image", $"Invalid main image size {image.Size}. The main image must be 1280x720 pixels.");
                }
                catch (Exception ex)
                {
                    validation.Err("image", "Invalid main image format. Supported formats are: jpg, dds. Error:" + ex.Message);
                    return;
                }
            }

            if (TextureReplacement.NxNameToList.TryGetValue(Manifest.Target, out var validTextures))
            {
                foreach (var image in Extra)
                {
                    var textureTarget = validTextures.FirstOrDefault(t => t.NxThemeName == image.Key);

                    if (textureTarget is null)
                    {
                        validation.Err(image.Key, $"This texture is not supported in the {Manifest.Target} theme part.");
                        continue;
                    }

                    try
                    {
                        var imageInfo = ImageUtil.ParseImage(image.Value);

                        if (imageInfo is JpgInfo)
                            validation.Err(image.Key, $"JPG images are not allowed for icons.");

                        if (imageInfo is DDS dds && !dds.IsValidForIcons)
                            validation.Err(image.Key, $"The provided DDS image can't be used for icons. Try a different encoding.");

                        if (imageInfo.Size.Width != textureTarget.W || imageInfo.Size.Height != textureTarget.H)
                            validation.Err(image.Key, $"Invalid extra image size {imageInfo.Size}. The image must be {textureTarget.W}x{textureTarget.H} pixels.");
                    }
                    catch (Exception ex)
                    {
                        validation.Err(image.Key, "Invalid extra image format. Supported formats are: jpg, png, dds. Error:" + ex.Message);
                    }
                }
            }
            else if (Extra.Count != 0)
            {
                foreach (var image in Extra)
                    validation.Err(image.Key, $"This texture is not supported in the {Manifest.Target} theme part.");
            }
        }
    }
}
