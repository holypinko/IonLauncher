#include "modsystem/modfilesystem.h"
#include "filesystem/basefilesystem.h"
#include "modsystem/modmanager.h"

#include <algorithm>
#include <cctype>
#include <mutex>
#include <unordered_map>
#include <vector>

// the currently accepted sources for files
static int iFileSourceType = FileSourceType_Any;

struct AddedModSearchPath_s
{
    std::string m_Path;
    std::string m_PathID;
    bool m_bHasPathID;
};

struct ModFilesystemState_s
{
    std::mutex m_SearchPathMutex;
    std::string m_CurrentModPath;
    std::string m_CurrentModPathID;
    bool m_bCurrentModHasPathID = false;
    std::vector<AddedModSearchPath_s> m_AddedSearchPaths;
    std::mutex m_MountedVPKsMutex;
    std::unordered_map<std::string, CPackedStore*> m_MountedVPKs;
    AddSearchPath_t m_AddSearchPath = nullptr;
    MountVPK_t m_MountVPK = nullptr;
    UnmountVPK_t m_UnmountVPK = nullptr;
};

static ModFilesystemState_s g_ModFilesystem;

std::string ReadVPKFile(const char* path)
{
    return ReadGameFile(path, "GAME");
}

std::string ReadVPKFile(const char* path, int fileSourceType)
{
    int oldType = iFileSourceType;
    iFileSourceType = fileSourceType;

    std::string ret = ReadVPKFile(path);

    iFileSourceType = oldType;

    return ret;
}

static void AddTrackedModSearchPathLocked(IFileSystem* const fileSystem, const std::string& path, const char* const pathID,
                                          const SearchPathAdd_t addType)
{
    if (!fileSystem || path.empty())
        return;

    g_ModFilesystem.m_AddSearchPath(fileSystem, path.c_str(), pathID, addType);

    const std::string trackedPathID = pathID ? pathID : "";
    const auto existing =
        std::find_if(g_ModFilesystem.m_AddedSearchPaths.begin(), g_ModFilesystem.m_AddedSearchPaths.end(), [&](const AddedModSearchPath_s& added)
    { return added.m_Path == path && added.m_PathID == trackedPathID && added.m_bHasPathID == (pathID != nullptr); });
    if (existing == g_ModFilesystem.m_AddedSearchPaths.end())
        g_ModFilesystem.m_AddedSearchPaths.push_back({path, trackedPathID, pathID != nullptr});
}

void PrioritiseModSearchPaths(IFileSystem* fileSystem, const char* pPath, const char* pathID, SearchPathAdd_t addType)
{
    std::scoped_lock lock(g_ModFilesystem.m_SearchPathMutex);

    // make sure current mod paths are at head
    if (g_ModFilesystem.m_CurrentModPath.compare(pPath) && addType == PATH_ADD_TO_HEAD)
    {
        AddTrackedModSearchPathLocked(fileSystem, g_ModFilesystem.m_CurrentModPath, pathID, PATH_ADD_TO_HEAD);
        g_ModFilesystem.m_AddSearchPath(fileSystem, GetCompiledAssetsPath().string().c_str(), pathID, PATH_ADD_TO_HEAD);
    }
}

static void SetNewCompiledSearchPaths(const char* pPathID)
{
    std::scoped_lock lock(g_ModFilesystem.m_SearchPathMutex);

    // push compiled to head
    g_ModFilesystem.m_AddSearchPath(g_pFilesystem, fs::absolute(GetCompiledAssetsPath()).string().c_str(), pPathID, PATH_ADD_TO_HEAD);
    g_ModFilesystem.m_CurrentModPath.clear();
    g_ModFilesystem.m_CurrentModPathID.clear();
    g_ModFilesystem.m_bCurrentModHasPathID = false;
}

static void SetNewModSearchPaths(Mod* mod, const char* pPathID)
{
    if (mod == nullptr)
        return;
    const std::string modPath = (fs::absolute(mod->m_ModDirectory) / MOD_OVERRIDE_DIR).string();
    std::scoped_lock lock(g_ModFilesystem.m_SearchPathMutex);
    if (modPath != g_ModFilesystem.m_CurrentModPath || (pPathID != nullptr) != g_ModFilesystem.m_bCurrentModHasPathID ||
        (pPathID && g_ModFilesystem.m_CurrentModPathID != pPathID))
    {
        AddTrackedModSearchPathLocked(g_pFilesystem, modPath, pPathID, PATH_ADD_TO_HEAD);
        g_ModFilesystem.m_CurrentModPath = modPath;
        g_ModFilesystem.m_CurrentModPathID = pPathID ? pPathID : "";
        g_ModFilesystem.m_bCurrentModHasPathID = pPathID != nullptr;
    }
}

bool RemoveModSearchPaths()
{
    if (!g_pFilesystem)
        return false;

    std::scoped_lock lock(g_ModFilesystem.m_SearchPathMutex);

    for (const AddedModSearchPath_s& added : g_ModFilesystem.m_AddedSearchPaths)
        g_pFilesystem->RemoveSearchPath(added.m_Path.c_str(), added.m_bHasPathID ? added.m_PathID.c_str() : nullptr);

    g_ModFilesystem.m_AddedSearchPaths.clear();
    g_ModFilesystem.m_CurrentModPath.clear();
    g_ModFilesystem.m_CurrentModPathID.clear();
    g_ModFilesystem.m_bCurrentModHasPathID = false;
    return true;
}

bool TryReplaceFile(const char* pPath, bool shouldCompile, const char* pPathID)
{
    // idk how efficient the lexically normal check is
    // can't just set all /s in path to \, since some paths aren't in writeable memory
    std::string normalisedPath = g_pModManager->NormaliseModFilePath(fs::path(pPath));

    if ((iFileSourceType & FileSourceType_ModOverride) && normalisedPath.ends_with(".nm") &&
        (normalisedPath.starts_with("maps\\navmesh\\") || normalisedPath.starts_with("maps/navmesh/")))
    {
        const fs::path generatedRoot = fs::absolute(GetGeneratedAssetsPath());
        std::error_code error;
        if (fs::is_regular_file(generatedRoot / fs::u8path(normalisedPath), error))
        {
            const std::u8string utf8Root = generatedRoot.u8string();
            const std::string searchPath(reinterpret_cast<const char*>(utf8Root.data()), utf8Root.size());
            std::scoped_lock lock(g_ModFilesystem.m_SearchPathMutex);
            if (g_ModFilesystem.m_CurrentModPath != searchPath)
            {
                AddTrackedModSearchPathLocked(g_pFilesystem, searchPath, pPathID, PATH_ADD_TO_HEAD);
                g_ModFilesystem.m_CurrentModPath = searchPath;
            }
            return true;
        }
    }

    if (iFileSourceType & FileSourceType_Compiled)
    {
        // only compile assets if we would accept a compiled asset in the first place
        if (shouldCompile)
            g_pModManager->CompileAssetsForFile(pPath);

        if (g_pModManager->m_CompiledFiles.contains(normalisedPath))
        {
            SetNewCompiledSearchPaths(pPathID);
            return true;
        }
    }

    if (iFileSourceType & FileSourceType_ModOverride)
    {
        auto file = g_pModManager->m_ModFiles.find(normalisedPath);

        if (file != g_pModManager->m_ModFiles.end())
        {
            SetNewModSearchPaths(file->second.m_pOwningMod, pPathID);
            return true;
        }
    }

    return false;
}

bool ShouldBypassModFileCache(const char* pPath)
{
    const bool isReloadModel = g_pModManager->IsModModelFile(pPath);
    return TryReplaceFile(pPath, true, "GAME") || isReloadModel || g_pModManager->IsMapVPKCacheFile(pPath);
}

static std::string NormaliseVPKPath(const char* path)
{
    std::string normalised = fs::path(path).lexically_normal().generic_string();
    std::transform(normalised.begin(), normalised.end(), normalised.begin(), [](const unsigned char c) { return std::tolower(c); });
    return normalised;
}

void ForgetMountedModVPK(const char* path)
{
    if (!path)
        return;

    {
        std::scoped_lock lock(g_ModFilesystem.m_MountedVPKsMutex);
        g_ModFilesystem.m_MountedVPKs.erase(NormaliseVPKPath(path));
    }
    if (g_pModManager)
        g_pModManager->UnregisterMountedVPKModels(path);
}

static CPackedStore* FindMountedModVPK(const char* path)
{
    if (!path)
        return nullptr;

    const std::string normalisedPath = NormaliseVPKPath(path);
    std::scoped_lock lock(g_ModFilesystem.m_MountedVPKsMutex);
    const auto mounted = g_ModFilesystem.m_MountedVPKs.find(normalisedPath);
    return mounted == g_ModFilesystem.m_MountedVPKs.end() ? nullptr : mounted->second;
}

CPackedStore* GetPreferredModVPK(const char* filename)
{
    std::string sourcePath;
    const bool hasModSource = g_pModManager->GetModVPKModelSource(filename, sourcePath) || g_pModManager->GetMapVPKFileSource(filename, sourcePath);
    return hasModSource ? FindMountedModVPK(sourcePath.c_str()) : nullptr;
}

static CPackedStore* MountModVPK(IFileSystem* fileSystem, const char* path)
{
    if (CPackedStore* const mounted = FindMountedModVPK(path))
        return mounted;
    const std::string normalisedPath = NormaliseVPKPath(path);

    CPackedStore* const loaded = g_ModFilesystem.m_MountVPK(fileSystem, path);
    {
        std::scoped_lock mountedLock(g_ModFilesystem.m_MountedVPKsMutex);
        if (loaded)
            g_ModFilesystem.m_MountedVPKs.insert_or_assign(normalisedPath, loaded);
        else
            g_ModFilesystem.m_MountedVPKs.erase(normalisedPath);
    }
    return loaded;
}

CPackedStore* MountVPKDirect(const char* pVpkPath)
{
    if (!g_pFilesystem || !g_ModFilesystem.m_MountVPK || !pVpkPath)
        return nullptr;

    return MountModVPK(g_pFilesystem, pVpkPath);
}

bool UnmountVPKDirect(const char* pVpkPath)
{
    if (!g_pFilesystem || !g_ModFilesystem.m_UnmountVPK || !pVpkPath)
        return false;

    g_ModFilesystem.m_UnmountVPK(g_pFilesystem, pVpkPath);
    ForgetMountedModVPK(pVpkPath);
    return true;
}

bool IsVPKMounted(const char* pVpkPath)
{
    if (!pVpkPath)
        return false;

    std::scoped_lock lock(g_ModFilesystem.m_MountedVPKsMutex);
    const std::string normalisedPath = NormaliseVPKPath(pVpkPath);
    return g_ModFilesystem.m_MountedVPKs.contains(normalisedPath);
}

CPackedStore* MountModVPKs(IFileSystem* fileSystem, const char* pVpkPath, CPackedStore* ret)
{
    for (const Mod& mod : g_pModManager->m_LoadedMods)
    {
        if (!mod.m_bEnabled)
            continue;

        for (const ModVPKEntry& vpkEntry : mod.Vpks)
        {
            if (!vpkEntry.m_MapName.empty())
            {
                if (!ret && fs::path(pVpkPath).filename() == fs::path(vpkEntry.m_sVpkPath).filename())
                    ret = FindMountedModVPK(vpkEntry.m_sVpkPath.c_str());
                continue;
            }
            // if we're autoloading, just load no matter what
            if (!vpkEntry.m_bAutoLoad)
            {
                // resolve vpk name and try to load one with the same name
                std::string mapName(fs::path(pVpkPath).filename().string());
                std::string modMapName(fs::path(vpkEntry.m_sVpkPath.c_str()).filename().string());
                if (mapName.compare(modMapName))
                    continue;
            }

            CPackedStore* loaded = MountModVPK(fileSystem, vpkEntry.m_sVpkPath.c_str());
            if (loaded)
                g_pModManager->RegisterMountedVPKModels(vpkEntry);
            if (!ret) // this is primarily for map vpks and stuff, so the map's vpk is what gets returned from here
                ret = loaded;
        }
    }

    return ret;
}

void InitialiseModFilesystem()
{
    g_ModFilesystem.m_AddSearchPath = HookSys::GetOriginalFunction<AddSearchPath_t>(HookSys::FindHook("AddSearchPath"));
    g_ModFilesystem.m_MountVPK = HookSys::GetOriginalFunction<MountVPK_t>(HookSys::FindHook("MountVPK"));
    g_ModFilesystem.m_UnmountVPK = HookSys::GetOriginalFunction<UnmountVPK_t>(HookSys::FindHook("UnmountVPK"));
}
