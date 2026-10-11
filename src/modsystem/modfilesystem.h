#pragma once

#include "filesystem/ifilesystem.h"

#include <string>

std::string ReadVPKFile(const char* path);
std::string ReadVPKFile(const char* path, int fileSourceFilter);

// Mounts an already-resolved VPK base path without invoking the mod VPK hook.
CPackedStore* MountVPKDirect(const char* vpkPath);
bool UnmountVPKDirect(const char* vpkPath);
bool IsVPKMounted(const char* vpkPath);

// Removes every loose mod directory added through SetNewModSearchPaths.
bool RemoveModSearchPaths();

// Native filesystem hooks delegate only the mod policy to these helpers.
void InitialiseModFilesystem();
void PrioritiseModSearchPaths(IFileSystem* fileSystem, const char* path, const char* pathID, SearchPathAdd_t addType);
bool TryReplaceFile(const char* path, bool shouldCompile, const char* pathID = "GAME");
bool ShouldBypassModFileCache(const char* path);
CPackedStore* GetPreferredModVPK(const char* filename);
void ForgetMountedModVPK(const char* path);
CPackedStore* MountModVPKs(IFileSystem* fileSystem, const char* path, CPackedStore* mounted);
