using SARCExt;
using SwitchThemes.Common;
using System.IO.Compression;
using System.Text;
using System.Text.Json;
using static System.Runtime.InteropServices.JavaScript.JSType;

namespace NxThemeTool
{
    public interface IContentProvider : IDisposable
    {
        List<string> GetFiles();

        bool HasFile(string name);
        
        byte[] GetFile(string name);

        string GetString(string name) => Encoding.UTF8.GetString(GetFile(name));
        T GetJson<T>(string name) => JsonSerializer.Deserialize<T>(GetString(name)) ?? throw new Exception($"Failed to deserialize JSON file '{name}'.");
    }

    public static class ProviderHelper
    {
        public static IContentProvider OpenFor(byte[] data)
        {
            if (IsSzsFormat(data))
            {
                var decompressed = ManagedYaz0.Decompress(data);
                return new SarcContentProvider(SARC.Unpack(decompressed));
            }
            else if (IsZipFormat(data))
            {
                return new ZipContentProvider(new MemoryStream(data));
            }

            throw new InvalidDataException("Data is not in a recognized format (SARC or ZIP).");
        }

        public static IContentProvider OpenFor(string path)
        {
            if (Directory.Exists(path))
                return new DirectoryContentProvider(path);

            if (File.Exists(path))
            {
                var file = File.OpenRead(path);
                var format = IdentifyFormat(file);
                
                if (format == Format.Sarc)
                {
                    file.Dispose();

                    var decompressed = ManagedYaz0.Decompress(File.ReadAllBytes(path));
                    return new SarcContentProvider(SARC.Unpack(decompressed));
                }
                else if (format == Format.Zip)
                {
                    return new ZipContentProvider(file);
                }
            }

            throw new FileNotFoundException($"Path '{path}' does not exist as a directory or file.");
        }

        public static bool IsSzsFormat(ReadOnlySpan<byte> fileHeader) => ManagedYaz0.IsYaz0(fileHeader.ToArray());
        public static bool IsSzsFormat(byte[] fileHeader) => ManagedYaz0.IsYaz0(fileHeader);

        public static bool IsZipFormat(ReadOnlySpan<byte> fileHeader) => fileHeader.Length > 4 && fileHeader[0] == 'P' && fileHeader[1] == 'K';
        public static bool IsZipFormat(byte[] fileHeader) => IsZipFormat(fileHeader.AsSpan());

        public enum Format
        {
            Sarc,
            Zip,
            Unknown
        }

        // This will rewind the stream
        public static Format IdentifyFormat(Stream stream)
        {
            var bytes = new byte[10];
            stream.Read(bytes, 0, bytes.Length);
            stream.Seek(0, SeekOrigin.Begin);

            if (IsSzsFormat(bytes)) return Format.Sarc;
            if (IsZipFormat(bytes)) return Format.Zip;
            return Format.Unknown;
        }
    }

    public class DirectoryContentProvider : IContentProvider
    {
        private readonly string directoryPath;
        public DirectoryContentProvider(string directoryPath)
        {
            this.directoryPath = directoryPath;
        }

        public List<string> GetFiles()
        {
            return Directory.GetFiles(directoryPath, "*.*", SearchOption.AllDirectories)
                // Paths are relative to the directory path
                .Select(path => Path.GetRelativePath(directoryPath, path).Replace('\\', '/'))
                .ToList();
        }

        public byte[] GetFile(string name)
        {
            var filePath = Path.Combine(directoryPath, name);
            if (!File.Exists(filePath))
                throw new FileNotFoundException($"File '{name}' not found in directory '{directoryPath}'.");
            return File.ReadAllBytes(filePath);
        }

        public void Dispose()
        {
            // No resources to dispose in this implementation
        }

        public bool HasFile(string name)
        {
            var filePath = Path.Combine(directoryPath, name);
            return File.Exists(filePath);
        }
    }

    public class SarcContentProvider : IContentProvider
    {
        private readonly SarcData sarc;
        public SarcContentProvider(SarcData sarc)
        {
            this.sarc = sarc;
        }

        public List<string> GetFiles()
        {
            return sarc.Files.Keys.ToList();
        }

        public byte[] GetFile(string name)
        {
            if (!sarc.Files.ContainsKey(name))
                throw new FileNotFoundException($"File '{name}' not found in SARC archive.");
            return sarc.Files[name];
        }

        public void Dispose()
        {
            // No resources to dispose in this implementation
        }

        public bool HasFile(string name)
        {
            return sarc.Files.ContainsKey(name);
        }
    }

    public class ZipContentProvider : IContentProvider
    {
        private readonly ZipArchive zip;

        public ZipContentProvider(Stream zipStream)
        {
            zip = new ZipArchive(zipStream, ZipArchiveMode.Read, leaveOpen: false);
        }

        public List<string> GetFiles()
        {
            return zip.Entries.Select(entry => entry.FullName).ToList();
        }

        public byte[] GetFile(string name)
        {
            var entry = zip.GetEntry(name);
            if (entry == null)
                throw new FileNotFoundException($"File '{name}' not found in ZIP archive.");
            using var entryStream = entry.Open();
            using var ms = new MemoryStream();
            entryStream.CopyTo(ms);
            return ms.ToArray();
        }

        public void Dispose()
        {
            zip.Dispose();
        }

        public bool HasFile(string name)
        {
            return zip.GetEntry(name) != null;
        }
    }
}
