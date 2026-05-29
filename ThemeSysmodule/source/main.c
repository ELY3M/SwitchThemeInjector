#include <switch.h>
#include <string.h>

// We try to have the smallest impact on memory as possible.
// By default, we don't use the heap at all.
#define USE_HEAP 0

#if !USE_HEAP
void* __libnx_aligned_alloc(size_t alignment, size_t size)
{
	return NULL;
}

void __libnx_free(void* p)
{

}

void __libnx_initheap(void)
{

}
#else

#define INNER_HEAP_SIZE (1 * 1024 * 1024)

size_t nx_inner_heap_size = INNER_HEAP_SIZE;
char nx_inner_heap[INNER_HEAP_SIZE];

void __libnx_initheap(void)
{
	void* addr = nx_inner_heap;
	size_t size = nx_inner_heap_size;

	// Newlib
	extern char* fake_heap_start;
	extern char* fake_heap_end;

	fake_heap_start = (char*)addr;
	fake_heap_end = (char*)addr + size;
}
#endif

u32 __nx_applet_type = AppletType_None;
u32 __nx_fs_num_sessions = 1;
u32 __nx_fsdev_direntry_cache_size = 1;

#define QLAUNCH_ID 0x0100000000001000

static FsFileSystem sdCard;
static Event homeMenuLaunched;
static SetSysFirmwareVersion fw;

void __attribute__((weak)) __appInit(void)
{
	Result rc;

	rc = smInitialize();
	if (R_FAILED(rc))
		fatalThrow(MAKERESULT(Module_Libnx, LibnxError_InitFail_SM));
	
	rc = setsysInitialize();
	if (R_SUCCEEDED(rc)) {
		rc = setsysGetFirmwareVersion(&fw);
		if (R_FAILED(rc)) fatalThrow(rc);
			
		hosversionSet(MAKEHOSVERSION(fw.major, fw.minor, fw.micro));
		setsysExit();
	}

	rc = pmdmntInitialize();
	if (R_FAILED(rc))
		fatalThrow(MAKERESULT(Module_Libnx, LibnxError_ShouldNotHappen));

	// Try to do this as fast as possible
	rc = pmdmntHookToCreateProcess(&homeMenuLaunched, QLAUNCH_ID);
	if (R_FAILED(rc))
		fatalThrow(rc);

	rc = fsInitialize();
	if (R_FAILED(rc))
		fatalThrow(MAKERESULT(Module_Libnx, LibnxError_InitFail_FS));

	// We use this directly because we don't want to depend on fsdev which requires malloc
	rc = fsOpenSdCardFileSystem(&sdCard);
	if (R_FAILED(rc))
		fatalThrow(rc);
}

void __attribute__((weak)) __appExit(void)
{
	fsFsClose(&sdCard);
	fsExit();
	eventClose(&homeMenuLaunched);
	pmdmntExit();
	smExit();
}

#define SECONDS(x) ((u64)(x)*1000000000ULL)

static bool should_delete_themes() 
{
	FsFile versionFile;

	if (R_FAILED(fsFsOpenFile(&sdCard, "/atmosphere/contents/0100000000001000/version_hash.bin", FsOpenMode_Read, &versionFile)))
	{
		FsDir dir;
		if (R_FAILED(fsFsOpenDirectory(&sdCard, "/atmosphere/contents/0100000000001000/romfs", FsDirOpenMode_ReadFiles | FsDirOpenMode_NoFileSize, &dir)))
		{
			// The themes directory doesn't exist at all. no need to delete it.
			return false;
		}

		// The directory exists but we don't know the version, better delete it to be safe.
		fsDirClose(&dir);
		return true;
	}

	// The file exists, read the content and check the version.	
	char disk_version[sizeof(fw.version_hash)] = {0};
	u64 bytes_read = 0;
	Result rc = fsFileRead(&versionFile, 0, disk_version, sizeof(disk_version), 0, &bytes_read);

	fsFileClose(&versionFile);

	if (R_FAILED(rc))
		return true;

	if (bytes_read != sizeof(disk_version))
		return true;

	if (!memcmp(disk_version, fw.version_hash, sizeof(disk_version)))
		return false;

	return true;
}

int main(int argc, char* argv[])
{
	Result rc = eventWait(&homeMenuLaunched, 20E+9);
	u64 pid = 0;

	if (R_SUCCEEDED(rc))
	{
		// We prevented the home menu from launching!
		// Check the system version, clean up if needed, then resume boot.
		if (should_delete_themes())
			fsFsDeleteDirectoryRecursively(&sdCard, "/atmosphere/contents/0100000000001000");
		
		// Resume the home menu
		rc = pmdmntGetProcessId(&pid, QLAUNCH_ID);
		if (R_FAILED(rc)) fatalThrow(rc);

		rc = pmdmntStartProcess(pid);
		if (R_FAILED(rc)) fatalThrow(rc);
	}
	else if (rc == KERNELRESULT(TimedOut))
	{
		if (R_SUCCEEDED(pmdmntGetProcessId(&pid, QLAUNCH_ID)) && pid)
		{
			// We were too slow?
			fatalThrow(MAKERESULT(Module_Libnx, LibnxError_AlreadyInitialized));
		}
		else
		{
			// Qlaunch is not running at all?
			fatalThrow(rc);
		}
	}
	else
	{
		fatalThrow(rc);
	}
	
	// Cleanly exit to free sysmodule memory
	return 0;
}
