#pragma once

#include "appframework/IAppSystem.h"

#include <cstddef>
#include <cstdint>
#include <type_traits>

using FileHandle_t = void*;
using FileNameHandle_t = void*;
using FileFindHandle_t = std::uint64_t;
using FSAllocFunc_t = void* (*)(const char* fileName, std::uint64_t bytes);
using PathTypeQuery_t = std::uint32_t;
using FSAsyncControl_t = void*;
using FSAsyncFile_t = void*;
using WaitForResourcesHandle_t = std::int32_t;
using FileSystemWarningFunc_t = void (*)(const char* format, ...);
using FileSystemLoggingFunc_t = void (*)(const char* fileName, const char* accessType);
using BlockingFileAccessLoggingFunc_t = void (*)(const char* fileName, std::int32_t flags, const char* accessType);
using FSAsyncDirectoryScanFunc_t = void (*)(void* context, const char* directory, const char* fileName);
using FSAsyncDirectoryScanCompleteFunc_t = void (*)(void* context, std::int32_t status);
inline constexpr FileFindHandle_t FILESYSTEM_INVALID_FIND_HANDLE = UINT64_MAX;

class CPackedStore;
class CSysModule;
class CUtlBuffer;
class FileSystemCache;
class KeyValues;
class CUtlString;
class VPKMasterCache;
struct VPKFileEntry_t;
struct FileAsyncRequest_t;
struct FileSystemStatistics;
struct PackedFileOpenData;
class IAsyncFileRequest;
class IAsyncGroupRequest;
class IAsyncRequest;
class IAsyncSearchRequest;
template <class T, class I> class CUtlMemory;
template <class T, class A, class I> class CUtlVector;

enum SearchPathAdd_t
{
    PATH_ADD_TO_HEAD,
    PATH_ADD_TO_TAIL,
};

enum FilesystemMountRetval_t
{
    FILESYSTEM_MOUNT_OK,
    FILESYSTEM_MOUNT_FAILED,
};

enum PathTypeFilter_t
{
    FILTER_NONE,
    FILTER_CULLPACK,
    FILTER_CULLNONPACK,
    FILTER_CULLLOCALIZED,
    FILTER_CULLLOCALIZED_ANY,
};

enum FileSystemSeek_t : std::uint32_t
{
    FILESYSTEM_SEEK_HEAD,
    FILESYSTEM_SEEK_CURRENT,
    FILESYSTEM_SEEK_TAIL,
};

enum FileSourceType_t : int
{
    FileSourceType_Original = 1 << 0,
    FileSourceType_ModOverride = 1 << 1,
    FileSourceType_Compiled = 1 << 2,

    FileSourceType_Modded = FileSourceType_ModOverride | FileSourceType_Compiled,
    FileSourceType_Any = FileSourceType_Original | FileSourceType_ModOverride | FileSourceType_Compiled,
};

class IBaseFileSystem
{
  public:
    virtual std::int64_t Read(void* output, std::int64_t size, FileHandle_t file) = 0;       // 0
    virtual std::int64_t Write(const void* input, std::int64_t size, FileHandle_t file) = 0; // 1
    virtual FileHandle_t Open(const char* fileName, const char* options, const char* pathID = nullptr,
                              std::uint32_t flags = 0) = 0;                                              // 2
    virtual void Close(FileHandle_t file) = 0;                                                           // 3
    virtual void Seek(FileHandle_t file, std::int64_t offset, FileSystemSeek_t seekType) = 0;            // 4
    virtual std::int64_t Tell(FileHandle_t file) = 0;                                                    // 5
    // MSVC emits virtual overload groups in reverse declaration order.
    virtual std::int64_t Size(FileHandle_t file) = 0;                                                    // 7
    virtual std::int64_t Size(const char* fileName, const char* pathID = nullptr) = 0;                   // 6
    virtual void Flush(FileHandle_t file) = 0;                                                           // 8
    virtual bool Precache(const char* fileName, const char* pathID = nullptr) = 0;                       // 9
    virtual bool FileExists(const char* fileName, const char* pathID = nullptr) = 0;                     // 10
    virtual bool IsFileWritable(const char* fileName, const char* pathID = nullptr) = 0;                 // 11
    virtual bool SetFileWritable(const char* fileName, bool writable, const char* pathID = nullptr) = 0; // 12
    virtual std::int32_t GetFileTime(const char* fileName, const char* pathID = nullptr) = 0;            // 13
    virtual bool ReadFile(const char* fileName, const char* pathID, CUtlBuffer& buffer, std::int64_t maxBytes = 0, std::int64_t startingByte = 0,
                          FSAllocFunc_t alloc = nullptr) = 0;                                      // 14
    virtual bool WriteFile(const char* fileName, const char* pathID, CUtlBuffer& buffer) = 0;      // 15
    virtual bool UnzipFile(const char* fileName, const char* pathID, const char* destination) = 0; // 16
};

class IFileSystem : public IAppSystem, public IBaseFileSystem
{
  public:
    virtual bool IsSteam() const = 0;                                                                                 // 8
    virtual FilesystemMountRetval_t MountSteamContent(int extraAppID = -1) = 0;                                       // 9
    virtual void AddSearchPath(const char* path, const char* pathID, SearchPathAdd_t addType = PATH_ADD_TO_TAIL) = 0; // 10
    virtual bool RemoveSearchPath(const char* path, const char* pathID = nullptr) = 0;                                // 11
    virtual void RemoveAllSearchPaths() = 0;                                                                          // 12
    virtual void RemoveSearchPaths(const char* pathID) = 0;                                                           // 13
    virtual void MarkPathIDByRequestOnly(const char* pathID, bool requestOnly) = 0;                                   // 14
    virtual const char* RelativePathToFullPath(const char* fileName, const char* pathID, char* localPath, std::int64_t localPathBufferSize,
                                               PathTypeFilter_t pathFilter = FILTER_NONE,
                                               PathTypeQuery_t* pathType = nullptr) = 0; // 15
    virtual std::int64_t GetSearchPath(const char* pathID, bool getPackFiles, char* path,
                                       std::int64_t maxLength) = 0;                                          // 16
    virtual bool AddPackFile(const char* fullPath, const char* pathID) = 0;                                  // 17
    virtual void RemoveFile(const char* relativePath, const char* pathID = nullptr) = 0;                     // 18
    virtual bool RenameFile(const char* oldPath, const char* newPath, const char* pathID = nullptr) = 0;     // 19
    virtual int CreateDirHierarchy(const char* path, const char* pathID = nullptr) = 0;                      // 20
    virtual bool IsDirectory(const char* fileName, const char* pathID = nullptr) = 0;                        // 21
    virtual std::int64_t FileTimeToString(char* output, std::int64_t outputSize, std::int32_t fileTime) = 0; // 22
    virtual void SetBufferSize(FileHandle_t file) = 0;                                                       // 23
    virtual bool IsOk(FileHandle_t file) = 0;                                                                // 24
    virtual bool EndOfFile(FileHandle_t file) = 0;                                                           // 25
    virtual char* ReadLine(char* output, std::size_t maxChars, FileHandle_t file) = 0;                       // 26
    virtual std::int64_t FPrintf(FileHandle_t file, const char* format, ...) = 0;                            // 27
    virtual CSysModule* LoadModule(const char* fileName, const char* pathID = nullptr,
                                   bool validatedOnly = true) = 0;                                           // 28
    virtual void UnloadModule(CSysModule* module) = 0;                                                       // 29
    virtual const char* FindFirst(const char* wildcard, FileFindHandle_t* handle) = 0;                       // 30
    virtual const char* FindNext(FileFindHandle_t handle) = 0;                                               // 31
    virtual bool FindIsDirectory(FileFindHandle_t handle) = 0;                                               // 32
    virtual void FindClose(FileFindHandle_t handle) = 0;                                                     // 33
    virtual const char* FindFirstEx(const char* wildcard, const char* pathID, FileFindHandle_t* handle) = 0; // 34
    virtual void FindFileAbsoluteList(CUtlVector<CUtlString, CUtlMemory<CUtlString, std::int64_t>, int>& absolutePathNames, const char* wildcard,
                                      const char* pathID) = 0;                                                     // 35
    virtual const char* GetLocalPath(const char* fileName, char* localPath, std::int64_t localPathBufferSize) = 0; // 36
    virtual bool FullPathToRelativePath(const char* fullPath, char* relativePath, std::int64_t maxLength) = 0;     // 37
    virtual bool GetCurrentDirectory(char* directory, std::uint32_t maxLength) = 0;                                // 38
    virtual FileNameHandle_t FindOrAddFileName(const char* fileName) = 0;                                          // 39
    virtual bool String(const FileNameHandle_t& handle, char* buffer, std::int64_t bufferLength) = 0;              // 40
    virtual std::int32_t AsyncReadMultiple(const FileAsyncRequest_t* requests, int requestCount,
                                           FSAsyncControl_t* controls = nullptr) = 0; // 41
    virtual std::int32_t AsyncAppend(const char* fileName, const void* source, std::int64_t sourceBytes, bool freeMemory,
                                     FSAsyncControl_t* control = nullptr) = 0; // 42
    virtual std::int32_t AsyncAppendFile(const char* appendToFileName, const char* appendFromFileName,
                                         FSAsyncControl_t* control = nullptr) = 0;                                  // 43
    virtual void AsyncFinishAll(int priority = 0) = 0;                                                              // 44
    virtual void AsyncFinishAllWrites() = 0;                                                                        // 45
    virtual std::int32_t AsyncFlush() = 0;                                                                          // 46
    virtual bool AsyncSuspend() = 0;                                                                                // 47
    virtual bool AsyncResume() = 0;                                                                                 // 48
    virtual std::int32_t AsyncBeginRead(const char* fileName, FSAsyncFile_t* file) = 0;                             // 49
    virtual std::int32_t AsyncEndRead(FSAsyncFile_t file) = 0;                                                      // 50
    virtual std::int32_t AsyncFinish(FSAsyncControl_t control, bool wait = true) = 0;                               // 51
    virtual std::int32_t AsyncGetResult(FSAsyncControl_t control, void** data, std::int64_t* size) = 0;             // 52
    virtual std::int32_t AsyncAbort(FSAsyncControl_t control) = 0;                                                  // 53
    virtual std::int32_t AsyncStatus(FSAsyncControl_t control) = 0;                                                 // 54
    virtual std::int32_t AsyncSetPriority(FSAsyncControl_t control, int priority) = 0;                              // 55
    virtual void AsyncAddRef(FSAsyncControl_t control) = 0;                                                         // 56
    virtual void AsyncRelease(FSAsyncControl_t control) = 0;                                                        // 57
    virtual WaitForResourcesHandle_t WaitForResources(const char* resourceList) = 0;                                // 58
    virtual bool GetWaitForResourcesProgress(WaitForResourcesHandle_t handle, float* progress, bool* complete) = 0; // 59
    virtual void CancelWaitForResources(WaitForResourcesHandle_t handle) = 0;                                      // 60
    virtual int HintResourceNeed(const char* hintList, int forgetEverything) = 0;                                 // 61
    virtual bool IsFileImmediatelyAvailable(const char* fileName) = 0;                                           // 62
    virtual void GetLocalCopy(const char* fileName) = 0;                                                          // 63
    virtual void PrintOpenedFiles() = 0;                                                                            // 64
    virtual void PrintSearchPaths() = 0;                                                                            // 65
    virtual void SetWarningFunc(FileSystemWarningFunc_t warningFunc) = 0;                                           // 66
    virtual void SetWarningLevel(int level) = 0;                                                                    // 67
    virtual void AddLoggingFunc(FileSystemLoggingFunc_t loggingFunc) = 0;                                           // 68
    virtual void RemoveLoggingFunc(FileSystemLoggingFunc_t loggingFunc) = 0;                                        // 69
    virtual void AddBlockingFileAccessLoggingFunc(BlockingFileAccessLoggingFunc_t loggingFunc) = 0;                 // 70
    virtual void RemoveBlockingFileAccessLoggingFunc(BlockingFileAccessLoggingFunc_t loggingFunc) = 0;              // 71
    virtual void LogFileAccess(const char* accessType, const char* fileName) = 0;                                   // 72
    virtual const FileSystemStatistics* GetFilesystemStatistics() = 0;                                              // 73
    virtual FileHandle_t OpenEx(const char* fileName, const char* options, std::uint32_t flags = 0, const char* pathID = nullptr,
                                char** resolvedFileName = nullptr) = 0; // 74
    virtual std::int64_t ReadEx(void* output, std::int64_t destinationSize, std::int64_t size,
                                FileHandle_t file) = 0; // 75
    virtual std::int64_t ReadFileEx(const char* fileName, const char* pathID, void** buffer, bool nullTerminate = false,
                                    bool optimalAllocation = false, std::int64_t maxBytes = 0, std::int64_t startingByte = 0,
                                    FSAllocFunc_t allocator = nullptr) = 0;                             // 76
    virtual FileNameHandle_t FindFileName(const char* fileName) = 0;                                    // 77
    virtual void SetupPreloadData() = 0;                                                                // 78
    virtual void DiscardPreloadData() = 0;                                                              // 79
    // The overloads occupy retail slots 80/81; declaration order follows MSVC's grouping.
    virtual KeyValues* LoadKeyValues(int type, const char* fileName, const char* pathID = nullptr) = 0; // 81
    virtual bool LoadKeyValues(KeyValues& root, int type, const char* fileName,
                               const char* pathID = nullptr) = 0; // 80
    virtual std::int32_t AsyncWrite(const char* fileName, const void* source, std::int64_t sourceBytes, bool freeMemory, bool append = false,
                                    FSAsyncControl_t* control = nullptr) = 0; // 82
    virtual std::int32_t AsyncWriteFile(const char* fileName, const CUtlBuffer* source, std::int64_t sourceBytes, bool freeMemory,
                                        bool append = false, FSAsyncControl_t* control = nullptr) = 0; // 83
    virtual std::int32_t AsyncReadMultipleCreditAlloc(const FileAsyncRequest_t* requests, int requestCount, const char* fileName, int line,
                                                      FSAsyncControl_t* controls = nullptr) = 0; // 84
    virtual std::int32_t AsyncDirectoryScan(const char* wildcard, bool recursive, void* context, FSAsyncDirectoryScanFunc_t scan,
                                            FSAsyncDirectoryScanCompleteFunc_t complete, FSAsyncControl_t* control = nullptr) = 0; // 85
    virtual bool GetFileTypeForFullPath(const char* fullPath, wchar_t* output, std::size_t outputSizeBytes) = 0; // 86
    virtual bool ReadToBuffer(FileHandle_t file, CUtlBuffer& buffer, std::int64_t maxBytes = 0,
                              FSAllocFunc_t allocator = nullptr) = 0; // 87
    virtual bool GetOptimalIOConstraints(FileHandle_t file, std::uint64_t* offsetAlignment, std::uint64_t* sizeAlignment,
                                         std::uint64_t* bufferAlignment) = 0;                                 // 88
    virtual void* AllocOptimalReadBuffer(FileHandle_t file, std::uint64_t size, std::uint64_t offset) = 0;    // 89
    virtual void FreeOptimalReadBuffer(void* buffer) = 0;                                                     // 90
    virtual bool IsVPKFileHandle(FileHandle_t file) = 0;                                                      // 91
    virtual bool IsOutsideVPKRead() = 0;                                                                      // 92
    virtual void UnmountVPKByIndex(int index) = 0;                                                            // 93
    virtual CPackedStore* GetMountedVPK(int index) = 0;                                                       // 94
    virtual bool ReadFromCache(const char* path, FileSystemCache* result) = 0;                                // 95
    virtual bool GetVPKFileEntry(CPackedStore* store, std::uint32_t entryIndex, FileSystemCache* result) = 0; // 96
    virtual void SetVPKCacheModeClient() = 0;                                                                 // 97
    virtual void SetVPKCacheModeServer() = 0;                                                                 // 98
    virtual bool IsVPKCacheEnabled() = 0;                                                                     // 99
    virtual std::int64_t PrecacheTaskItem(CPackedStore* store) = 0;                                           // 100
    virtual void ResetItemCacheSize(std::uint32_t size) = 0;                                                  // 101
    virtual void FinishVPKPrecache(CPackedStore* store) = 0;                                                  // 102
    virtual void SetVPKPrecacheEnabled(bool enabled) = 0;                                                     // 103
    virtual void* SetVPKItemCacheEntries(const void* entries, std::uint32_t count) = 0;                       // 104
    virtual void SetVPKPrecacheCallback(std::int64_t (*callback)(std::uint64_t)) = 0;                         // 105
    virtual bool ResetItemCache() = 0;                                                                        // 106
    virtual bool GetMountedVPKName(int index, char* output, std::uint32_t outputSize) = 0;                    // 107
    virtual std::uint32_t GetVPKFileInfo(std::uint32_t vpkIndex, std::uint64_t entryIndex, char* outputPath,
                                         std::uint32_t outputSize) = 0;                                        // 108
    virtual VPKFileEntry_t* GetMountedVPKFileEntry(std::uint32_t index) = 0;                                 // 109
    virtual int GetMountedVPKFileCount(const char* vpkPath) = 0;          // 110
    virtual CPackedStore* MountVPK(const char* vpkPath) = 0;              // 111
    virtual void UnmountVPK(const char* vpkPath) = 0;                     // 112
    virtual void BeginVPKPreloadAccess() = 0;                             // 113
    virtual void EndVPKPreloadAccess() = 0;                               // 114
    virtual std::int64_t ReadVPKFileData(CPackedStore* store, VPKMasterCache* cache, PackedFileOpenData* openData, std::uint64_t bytesToRead,
                                         void* context, void* (*allocateDestination)(void*)) = 0; // 115
    virtual VPKMasterCache* GetVPKPreloadState() = 0;                                                 // 116
    virtual void BuildVPKOpenData(CPackedStore* store, std::uint32_t entryIndex, PackedFileOpenData* output) = 0; // 117
    virtual bool LoadVPKForMap(const char* mapName) = 0;                                                                   // 118
    virtual std::uint32_t GetVPKPrecacheJobID() = 0;                                                                       // 119
    virtual bool FullPathToRelativePathEx(const char* fullPath, const char* pathID, char* relativePath,
                                          std::int64_t relativePathSize) = 0; // 120
    virtual int GetPathIndex(const FileNameHandle_t& handle) = 0;             // 121
    virtual std::int32_t GetPathTime(const char* path, const char* pathID) = 0; // 122
    virtual std::int64_t GetSearchPathID(char* output, std::int64_t outputSize) = 0; // 123
    virtual void RemoveAllMapSearchPaths() = 0;                                 // 124
    virtual bool GetStringFromKVPool(std::uint32_t poolKey, std::uint32_t key, char* output,
                                     std::int64_t outputSize) = 0;        // 125
    virtual void SetIODelayAlarm(float threshold) = 0;                    // 126

  protected:
    // The final retail slot returns false and has no observable input contract.
    // Its original semantic name is unavailable; keep it out of the callable API.
    virtual bool Reserved127() = 0; // 127
};

class IAsyncFileSystem : public IAppSystem
{
  public:
    virtual std::uint32_t SubmitRequest(IAsyncRequest* request) = 0;           // 8
    virtual std::uint32_t SubmitRequestAndWait(IAsyncRequest* request) = 0;    // 9
    virtual std::uint32_t GetRequestStatus(IAsyncRequest* request) = 0;        // 10
    virtual std::uint32_t AbortRequest(IAsyncRequest* request) = 0;            // 11
    virtual void SuspendProcessing(bool wait) = 0;                             // 12
    virtual void ResumeProcessing() = 0;                                       // 13
    virtual void DeleteAllRequests(bool wait) = 0;                             // 14
    virtual IAsyncFileRequest* CreateFileRequest() = 0;                        // 15
    virtual IAsyncSearchRequest* CreateSearchRequest() = 0;                    // 16
    virtual IAsyncGroupRequest* CreateGroupRequest() = 0;                      // 17
    virtual void ReleaseRequest(IAsyncRequest* request) = 0;                   // 18
    virtual bool WaitForRequest(IAsyncRequest* request) = 0;                   // 19
    virtual void SetRequestPriority(IAsyncRequest* request, int priority) = 0; // 20
    virtual void FreeResultBuffer(void* buffer) = 0;                           // 21
};

static_assert(sizeof(FileNameHandle_t) == 0x8);
static_assert(sizeof(FileFindHandle_t) == 0x8);
static_assert(sizeof(IBaseFileSystem) == 0x8);
static_assert(sizeof(IFileSystem) == 0x10);
static_assert(sizeof(IAsyncFileSystem) == 0x8);
static_assert(std::is_base_of_v<IAppSystem, IFileSystem>);
static_assert(std::is_base_of_v<IBaseFileSystem, IFileSystem>);
