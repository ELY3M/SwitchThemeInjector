using NxThemeTool;
using SwitchThemes.Common;

Console.WriteLine("NxThemeTool - https://github.com/exelix11/SwitchThemeInjector");
Console.WriteLine();

if (args.Length == 0 || args.Any(x => x == "help" || x == "-h" || x == "--help" || x == "-help"))
{
    PrintHelp();
    return 0;
}

if (args[0] == "list")
{
    Console.WriteLine("Supported theme parts:");
    foreach (var item in CommonInfo.Parts)
    {
        Console.WriteLine($"- Part name: {item.Name}");
        Console.WriteLine($"\t {item.Description} is {item.SzsName} in {item.TitleId}");
    }
}
if (args[0] == "new")
{
    if (args.Length < 2)
    {
        Console.WriteLine("Not enough arguments.");
        return 1;
    }

    var theme = NxTheme.CreateNew(args[1]);
    theme.MainImageFile = Util.CreateEmpty720PJPG();

    using var writer = new DirectoryContentWriter(args[2]);
    theme.Pack(writer);
}
else if (args[0] == "validate")
{
    var validation = new ProcessResult();

    using var provider = ProviderHelper.OpenFor(args[1]);
    new NxTheme(provider, validation);

    PrintValidation(validation);
}
else if (args[0] == "pack")
{
    if (args.Length < 3)
    {
        Console.WriteLine("Not enough arguments for packing.");
        return 1;
    }

    if (!Directory.Exists(args[1]))
    {
        Console.WriteLine("Target directory does not exist.");
        return 1;
    }

    using var provider = new DirectoryContentProvider(args[1]);
    using var writer = new ZipContentWriter(new FileStream(args[2], FileMode.Create, FileAccess.Write));
    var validation = new ProcessResult();

    var theme = new NxTheme(provider, validation);
    PrintValidation(validation);

    theme.Pack(writer);
}
else if (args[0] == "apply")
{
    if (args.Length < 4)
    {
        Console.WriteLine("Not enough arguments.");
        return 1;
    }

    var source = args[1];
    var szs = args[2];
    var output = args[3];

    var result = new ProcessResult();

    using var patcher = ThemeApply.FromFiles(source, szs, result);
    using var outputFolder = new DirectoryContentWriter(output);

    patcher.Apply(outputFolder, result);

    PrintValidation(result);
}
else if (args[0] == "unpack" || File.Exists(args[0]))
{
    if (args[0] == "unpack" && args.Length < 3)
    {
        Console.WriteLine("Not enough arguments for unpacking.");
        return 1;
    }

    var source = args[0] == "unpack" ? args[1] : args[0];
    var dest = args[0] == "unpack" ? args[2] : Path.GetFileNameWithoutExtension(args[0]) + "_unpacked";

    using var provider = ProviderHelper.OpenFor(source);
    using var destWriter = new DirectoryContentWriter(dest);

    var theme = new NxTheme(provider, null);
    theme.Pack(destWriter);
}
else if (args[0] == "install")
{
    if (args.Length < 3)
    {
        Console.WriteLine("Not enough arguments.");
        return 1;
    }

    var result = RemoteInstall.DoRemoteInstall(args[1], File.ReadAllBytes(args[2]));
    if (result != null)
    {
        Console.WriteLine(result);
        return 1;
    }
}
else if (args[0] == "cppgen")
{
    if (args.Length < 2)
    {
        Console.WriteLine("Not enough arguments.");
        return 1;
    }

    var gen = new CppGen(args[1]);
    gen.GeneratePatchTemplates();
    gen.GenerateTextureReplacementTable();
    gen.GenerateLayoutJsons();
}
else
{
    Console.WriteLine("Invalid commandline.");
    PrintHelp();
    return 1;
}

return 0;

void PrintValidation(ProcessResult result) 
{
    if (result.Warnings.Count == 0 && result.Errors.Count == 0)
    {
        Console.ForegroundColor = ConsoleColor.Green;
        Console.WriteLine("Completed. No warnings were generated.");
    }

    foreach (var warning in result.Warnings)
    {
        Console.ForegroundColor = ConsoleColor.Yellow;
        Console.WriteLine($"Warning [{warning.Source}]: {warning.Message}");
    }

    foreach (var warning in result.Errors)
    {
        Console.ForegroundColor = ConsoleColor.Red;
        Console.WriteLine($"Error [{warning.Source}]: {warning.Message}");
    }

    Console.ResetColor();
}

void PrintHelp()
{
    Console.WriteLine("Usage: NxThemeTool <command> [options]");
    Console.WriteLine("Commands:");
    Console.WriteLine("  new <theme part> <directory>       Creates a new theme structure in the specified folder for the provided theme part.");
    Console.WriteLine("  list                               Shows the list of valid theme parts");
    Console.WriteLine("  validate <target>                  Ensures the selected nxtheme or folder is valid");
    Console.WriteLine("  pack <target directory> <output>   Packs a folder to a nxtheme file");
    Console.WriteLine("  unpack <file> <output directory>   Extracts the content of a nxtheme file to the specified directory");
    Console.WriteLine("  install <file> <ip address>        Perform remote install to NXThemesInstaller running on a console");
    Console.WriteLine("  apply <nxtheme> <szs> <output>     Apply an nxthme file to one or more szs files. Szs must be the the path to the systemData folder of the theme installer.");
    Console.WriteLine("Extra:");
    Console.WriteLine("  <nxtheme file>                     If the only specified argument is a valid nxtheme file it will be unpacked. This is a convenience feature which allows dragging nxtheme files on this binary to unpack them automatically.");
    Console.WriteLine("Development:");
    Console.WriteLine("  cppgen <output foler>              Generate C++ data tables needed by the NxThemesInstaller codebase");
}