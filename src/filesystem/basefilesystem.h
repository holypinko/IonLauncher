#pragma once

#include "filesystem/ifilesystem.h"

#include <cstddef>
#include <cstdarg>
#include <cstdio>
#include <cstdint>
#include <string>


enum FileMode_t : int;
struct _stat64i32;
struct _WIN32_FIND_DATAA;

class CBaseFileSystem : public IFileSystem
{
  protected:
    virtual std::FILE* FS_fopen(const char* fileName, const char* options, std::uint32_t flags, std::int64_t* size) = 0; // 128
    virtual void FS_setbufsize(std::FILE* file, std::uint64_t bytes) = 0;                                              // 129
    virtual void FS_fclose(std::FILE* file) = 0;                                                                       // 130
    virtual void FS_fseek(std::FILE* file, std::int64_t position, int seekType) = 0;                                  // 131
    virtual std::int64_t FS_ftell(std::FILE* file) = 0;                                                               // 132
    virtual int FS_feof(std::FILE* file) = 0;                                                                          // 133
    virtual std::size_t FS_fread(void* destination, std::size_t destinationSize, std::size_t bytes, std::FILE* file) = 0; // 134
    virtual std::size_t FS_fwrite(const void* source, std::size_t bytes, std::FILE* file) = 0;                         // 135
    virtual bool FS_setmode(std::FILE* file, FileMode_t mode) = 0;                                                    // 136
    virtual std::size_t FS_vfprintf(std::FILE* file, const char* format, std::va_list arguments) = 0;                 // 137
    virtual int FS_ferror(std::FILE* file) = 0;                                                                        // 138
    virtual int FS_fflush(std::FILE* file) = 0;                                                                        // 139
    virtual char* FS_fgets(char* destination, std::int64_t destinationSize, std::FILE* file) = 0;                     // 140
    virtual int FS_stat(const char* path, struct _stat64i32* status) = 0;                                             // 141
    virtual int FS_chmod(const char* path, int mode) = 0;                                                              // 142
    virtual void* FS_FindFirstFile(const char* wildcard, _WIN32_FIND_DATAA* findData) = 0;                            // 143
    virtual bool FS_FindNextFile(void* handle, _WIN32_FIND_DATAA* findData) = 0;                                      // 144
    virtual bool FS_FindClose(void* handle) = 0;                                                                       // 145
    virtual int FS_GetSectorSize(std::FILE* file) = 0;                                                                // 146
};

class CSearchPath
{
  public:
    unsigned char unknown[0x18];
    const char* debugPath;
};

static_assert(sizeof(CSearchPath) == 0x20);
static_assert(offsetof(CSearchPath, debugPath) == 0x18);

using AddSearchPath_t = void(__fastcall*)(IFileSystem*, const char*, const char*, SearchPathAdd_t);
using MountVPK_t = CPackedStore* (*)(IFileSystem*, const char*);
using UnmountVPK_t = void (*)(IFileSystem*, const char*);

extern IFileSystem* g_pFilesystem;

std::string ReadGameFile(const char* path, const char* pathID);
