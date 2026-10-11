#include "filesystem/basefilesystem.h"
#include "tier1/interface.h"
#include "modsystem/modfilesystem.h"
#include "vpklib/packedstore.h"

#include <cstdint>
#include <sstream>

DECLARE_MODULE(FilesystemHooks)

IFileSystem* g_pFilesystem;

static void* g_GetStudioHdrCacheReturnAddress = nullptr;

std::string ReadGameFile(const char* path, const char* pathID)
{
    if (!g_pFilesystem || !path)
        return {};

    IBaseFileSystem* const baseFileSystem = static_cast<IBaseFileSystem*>(g_pFilesystem);
    FileHandle_t fileHandle = baseFileSystem->Open(path, "rb", pathID);
    if (!fileHandle)
        return {};

    std::stringstream fileStream;
    char data[4096];
    std::int64_t bytesRead = 0;
    while ((bytesRead = baseFileSystem->Read(data, std::size(data), fileHandle)) > 0)
    {
        fileStream.write(data, bytesRead);
    }

    baseFileSystem->Close(fileHandle);
    if (bytesRead < 0)
        return {};

    return fileStream.str();
}

DECLARE_HOOK(AddSearchPath, filesystem_stdio.dll + 0xB510,
             [](auto& hook, IFileSystem* fileSystem, const char* pPath, const char* pathID, SearchPathAdd_t addType)
{
    hook.Original(fileSystem, pPath, pathID, addType);
    PrioritiseModSearchPaths(fileSystem, pPath, pathID, addType);
})

DECLARE_HOOK(CBaseFileSystem_Size, filesystem_stdio.dll + 0x1BD60,
             [](auto& hook, IBaseFileSystem* fileSystem, const char* pPath, const char* pPathID) -> std::int64_t
{
    if (pPath)
        TryReplaceFile(pPath, true, pPathID);

    return hook.Original(fileSystem, pPath, pPathID);
})

DECLARE_HOOK(ReadFromCache, filesystem_stdio.dll + 0xFE50, [](auto& hook, IFileSystem* filesystem, const char* pPath, void* result) -> bool
{
    if (ShouldBypassModFileCache(pPath))
        return false;

    if (hook.ReturnAddress() == g_GetStudioHdrCacheReturnAddress)
        return false;

    return hook.Original(filesystem, pPath, result);
})

DECLARE_HOOK(CBaseFileSystem_OpenEx, filesystem_stdio.dll + 0x15F50,
             [](auto& hook, IFileSystem* filesystem, const char* pPath, const char* pOptions, uint32_t flags, const char* pPathID,
                char** ppszResolvedFilename) -> FileHandle_t
{
    TryReplaceFile(pPath, true, pPathID);
    return hook.Original(filesystem, pPath, pOptions, flags, pPathID, ppszResolvedFilename);
})

DECLARE_HOOK(UnmountVPK, filesystem_stdio.dll + 0x1A120, [](auto& hook, IFileSystem* fileSystem, const char* pVpkPath)
{
    hook.Original(fileSystem, pVpkPath);
    ForgetMountedModVPK(pVpkPath);
})

DECLARE_HOOK(MountVPK, filesystem_stdio.dll + 0xBEA0, [](auto& hook, IFileSystem* fileSystem, const char* pVpkPath) -> CPackedStore*
{
    CPackedStore* ret = hook.Original(fileSystem, pVpkPath);
    return MountModVPKs(fileSystem, pVpkPath, ret);
})

ON_DLL_LOAD("datacache.dll", FilesystemStudioCache, [](CModule module) { g_GetStudioHdrCacheReturnAddress = module.Offset(0x5B51C).RCast<void*>(); })

ON_DLL_LOAD("filesystem_stdio.dll", Filesystem, [](CModule)
{
    g_pFilesystem = Sys_GetFactoryPtr("filesystem_stdio.dll", "VFileSystem017").RCast<IFileSystem*>();

    DISPATCH_MODULE(FilesystemHooks)
    InitialisePackedStoreFilesystemHooks();

    InitialiseModFilesystem();
})
