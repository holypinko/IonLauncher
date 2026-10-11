#include "game/shared/weapon_parse.h"
#include "game/shared/weapon_parse.h"
#include "game/client/c_baseentity.h"
#include "game/client/c_player.h"
#include "game/shared/r1/c_weapon_x.h"
#include "game/server/baseentity.h"
#include "game/shared/r1/weapon_x.h"
#include "engine/server/server.h"
#include "game/shared/weapon_mod_data.h"
#include "modsystem/modmanager.h"
#include "rtech/rstdlib.h"
#include "tier0/callbacks.h"
#include "util/utils.h"

#include <mutex>

DECLARE_MODULE(ClientWeaponParseHooks)

static FileWeaponInfo_Client* (*g_GetFileWeaponInfoFromHandleClient)(WEAPON_FILE_INFO_HANDLE);
static FileWeaponInfo_Client* (*g_GetFileWeaponInfoFromNameClient)(const char*);
static void (*FileWeaponInfo_Client__Parse)(FileWeaponInfo_Client*, KeyValues*, const char*);
static bool (*g_ReadWeaponDataFromFileForSlotClient)(IFileSystem*, const char*, WEAPON_FILE_INFO_HANDLE*, const unsigned char*);
static KeyValues* (*g_ReadEncryptedKVFileClient)(IFileSystem*, const char*, const unsigned char*);
static WeaponString_t (*g_AllocWeaponStringClient)(FileWeaponInfo_Client*, const char*);
static bool (*g_GetIndexForModNameClient)(const char*, const FileWeaponInfo_Client*, unsigned int*);
static bool (*g_CalcWeaponModsClient)(unsigned int, const FileWeaponInfo_Client*, WeaponModValues*, bool, unsigned int);
static KeyValues* (*g_ParseWeaponModClient)(KeyValues*, FileWeaponInfo_Client*, const char*, WeaponMod*);
static bool (*g_InsertWeaponModAssemblyItemClient)(WeaponModEntry_t*, const WeaponModParseTableEntry*, WeaponModAssemblyItem_t*);
static int (*g_PrecacheImpactEffectTableClient)(const char*);
static int (*g_PrecacheParticleSystemByNameClient)(const char*);
static int (*g_PrecacheModelClient)(const char*);
static void (*g_PrecacheScriptSoundClient)(C_BaseEntity*, const char*);

static RHashMapString* s_MPEDict_client;
static const WeaponModParseTableEntry* ModParseEntries_Client;

FileWeaponInfo_Client* GetFileWeaponInfoFromHandle_Client(WEAPON_FILE_INFO_HANDLE handle)
{
    return g_GetFileWeaponInfoFromHandleClient(handle);
}

FileWeaponInfo_Client* GetFileWeaponInfoFromName_Client(const char* name)
{
    return g_GetFileWeaponInfoFromNameClient(name);
}

WEAPON_FILE_INFO_HANDLE LookupWeaponInfoSlot_Client(const char* name)
{
    const FileWeaponInfo_Client* info = GetFileWeaponInfoFromName_Client(name);
    return info ? info->infoHandle : INVALID_WEAPON_INFO_HANDLE;
}

void FileWeaponInfo_Client::Parse(KeyValues* data, const char* weaponName)
{
    FileWeaponInfo_Client__Parse(this, data, weaponName);
}

bool ReadWeaponDataFromFileForSlot_Client(IFileSystem* filesystem, const char* weaponName, WEAPON_FILE_INFO_HANDLE* handle,
                                          const unsigned char* iceKey)
{
    return g_ReadWeaponDataFromFileForSlotClient(filesystem, weaponName, handle, iceKey);
}

KeyValues* ReadEncryptedKVFile_Client(IFileSystem* filesystem, const char* filenameWithoutExtension, const unsigned char* iceKey)
{
    return g_ReadEncryptedKVFileClient(filesystem, filenameWithoutExtension, iceKey);
}

WeaponString_t AllocWeaponString_Client(FileWeaponInfo_Client* info, const char* string)
{
    return g_AllocWeaponStringClient(info, string);
}

bool GetIndexForModName_Client(const char* modName, const FileWeaponInfo_Client* info, unsigned int* index)
{
    return g_GetIndexForModNameClient(modName, info, index);
}

bool CalcWeaponMods_Client(unsigned int bitfield, const FileWeaponInfo_Client* info, WeaponModValues* values, bool singlePlayer,
                           unsigned int overrideMods)
{
    return g_CalcWeaponModsClient(bitfield, info, values, singlePlayer, overrideMods);
}

const WeaponModParseTableEntry* FindWeaponModParseTableEntry_Client(const char* fieldName)
{
    const auto* entryType = static_cast<const std::uint32_t*>(s_MPEDict_client->Find(fieldName));
    return entryType && *entryType != WMET_INVALID && *entryType < MAX_WEAPON_MOD_PARSE_ENTRIES ? &ModParseEntries_Client[*entryType] : nullptr;
}

const WeaponModParseTableEntry* GetWeaponModParseTableEntry_Client(WeaponModEntryType entryType)
{
    const auto index = static_cast<std::uint16_t>(entryType);
    return index < MAX_WEAPON_MOD_PARSE_ENTRIES ? &ModParseEntries_Client[index] : nullptr;
}

KeyValues* ParseWeaponMod_Client(KeyValues* section, FileWeaponInfo_Client* info, const char* weaponName, WeaponMod* mod)
{
    return g_ParseWeaponModClient(section, info, weaponName, mod);
}

bool InsertWeaponModAssemblyItem_Client(WeaponModEntry_t* entry, const WeaponModParseTableEntry* parseEntry, WeaponModAssemblyItem_t* item)
{
    return g_InsertWeaponModAssemblyItemClient(entry, parseEntry, item);
}

void PrecacheWeaponModAsset_Client(const WeaponModParseTableEntry& parseEntry, const char* assetName)
{
    if (parseEntry.parseFlags & WMPF_IMPACT_EFFECT_TABLE)
        g_PrecacheImpactEffectTableClient(assetName);
    else if (parseEntry.parseFlags & WMPF_PARTICLE_SYSTEM)
        g_PrecacheParticleSystemByNameClient(assetName);
}

int PrecacheWeaponModModel_Client(const char* modelName)
{
    return g_PrecacheModelClient(modelName);
}

void PrecacheWeaponModSound_Client(C_BaseEntity* owner, const char* soundName)
{
    g_PrecacheScriptSoundClient(owner, soundName);
}

DECLARE_HOOK(UpdateWeaponUiValue, client.dll + 0x3D3710,
             [](auto& hook, float* output, const WeaponUiArg_s* source, C_WeaponX* weapon, C_Player* player, float spread)
{
    C_Player* const previousPlayer =
        C_Player::SetCrosshairQueryPlayer(source->source == 0x17 || source->source == 0x18 ? player : nullptr);
    const ScopeGuard restorePlayer([previousPlayer] { C_Player::SetCrosshairQueryPlayer(previousPlayer); });
    hook.Original(output, source, weapon, player, spread);
})
DECLARE_HOOK(InitializeWeaponInfo_Client, client.dll + 0x3CC990, [](auto& hook, FileWeaponInfo_Client* weaponInfo) -> std::uintptr_t
{
    g_ClientWeaponMods.InitializeWeaponInfo(weaponInfo);
    return hook.Original(weaponInfo);
});

DECLARE_HOOK(ParseWeaponInfoFile_Client, client.dll + 0x3CFAC0,
             [](auto& hook, FileWeaponInfo_Client* weaponInfo, KeyValues* root, const char* weaponName) -> std::uint32_t*
{ return g_ClientWeaponMods.ParseWeaponInfo(weaponInfo, root, weaponName, hook.s_original); });

DECLARE_HOOK(ParseWeaponModGroups_Client, client.dll + 0x3D1020,
             [](auto& hook, FileWeaponInfo_Client* weaponInfo, KeyValues* root, const char* weaponName, WeaponMod* outputGroups,
                std::uint32_t* outputGroupCount) -> std::uint32_t*
{ return g_ClientWeaponMods.ParseGroups(weaponInfo, root, weaponName, outputGroups, outputGroupCount, hook.s_original); });

DECLARE_HOOK(ParseWeaponModGroup_Client, client.dll + 0x3D15D0,
             [](auto& hook, KeyValues* section, FileWeaponInfo_Client* weaponInfo, const char* weaponName, WeaponMod* outputGroup) -> KeyValues*
{ return g_ClientWeaponMods.ParseGroup(section, weaponInfo, weaponName, outputGroup, hook.s_original); });

DECLARE_HOOK(AssembleWeaponMods_Client, client.dll + 0x3CA0B0,
             [](auto& hook, int enabledMods, FileWeaponInfo_Client* weaponInfo, WeaponModValues* output, bool singlePlayer,
                int removedMods) -> std::uint8_t
{
    const auto result = g_ClientWeaponMods.Assemble(enabledMods, weaponInfo, output, singlePlayer, removedMods, hook.s_original);
    g_ClientWeaponMods.ApplyFieldOverrides(output);
    return result;
});

DECLARE_HOOK(ApplyWeaponModEntry_Client, client.dll + 0x3C8EA0, [](auto& hook, WeaponModEntry_t* entry, bool setBaseValue, bool remove) -> bool
{
    if (!CWeaponModHandler<FileWeaponInfo_Client>::HasActiveAssembly())
        return hook.Original(entry, setBaseValue, remove);

    return CWeaponModHandler<FileWeaponInfo_Client>::ApplyActiveEntry(entry, setBaseValue, remove);
});

DECLARE_HOOK(PrecacheWeaponModAssets_Client, client.dll + 0x3D1ED0,
             [](auto& hook, FileWeaponInfo_Client* weaponInfo, const WeaponMod* group, float precacheValue) -> void
{ g_ClientWeaponMods.PrecacheAssets(weaponInfo, *group, precacheValue, hook.s_original); });

DECLARE_HOOK(PrecacheWeaponModStrings_Client, client.dll + 0x3D23E0, [](auto& hook, FileWeaponInfo_Client* weaponInfo, const WeaponMod* group) -> int
{ return g_ClientWeaponMods.PrecacheStrings(weaponInfo, *group, hook.s_original); });

DECLARE_HOOK(PrecacheAllWeaponModStrings_Client, client.dll + 0x3D2480, [](auto& hook, FileWeaponInfo_Client* weaponInfo) -> int
{ return g_ClientWeaponMods.PrecacheAllStrings(weaponInfo, hook.s_original); });

DECLARE_HOOK(NotifyWeaponModStringField_Client, client.dll + 0x3D41E0, [](auto& hook, C_WeaponX* weapon, std::uint16_t fieldIndex) -> void
{ g_ClientWeaponMods.NotifyStringField(weapon, static_cast<WeaponModEntryType>(fieldIndex), hook.s_original); });

DECLARE_HOOK(ConCommand_weapon_reparse, client.dll + 0x3D4930, [](auto& hook, const CCommand& args) -> void
{
    g_pModManager->InvalidateKeyValuesPatches("scripts/weapons/");
    WeaponReparseServer(args);
    hook.Original(args);
});

ON_DLL_LOAD("client.dll", WeaponMods_Client, [](CModule)
{
    g_ClientWeaponMods.Reset();
    DISPATCH_HOOK(ClientWeaponParseHooks, InitializeWeaponInfo_Client)
    DISPATCH_HOOK(ClientWeaponParseHooks, ParseWeaponInfoFile_Client)
    DISPATCH_HOOK(ClientWeaponParseHooks, ParseWeaponModGroups_Client)
    DISPATCH_HOOK(ClientWeaponParseHooks, ParseWeaponModGroup_Client)
    DISPATCH_HOOK(ClientWeaponParseHooks, AssembleWeaponMods_Client)
    DISPATCH_HOOK(ClientWeaponParseHooks, ApplyWeaponModEntry_Client)
    DISPATCH_HOOK(ClientWeaponParseHooks, PrecacheWeaponModAssets_Client)
    DISPATCH_HOOK(ClientWeaponParseHooks, PrecacheWeaponModStrings_Client)
    DISPATCH_HOOK(ClientWeaponParseHooks, PrecacheAllWeaponModStrings_Client)
    DISPATCH_HOOK(ClientWeaponParseHooks, NotifyWeaponModStringField_Client)
});

ON_DLL_LOAD("client.dll", WeaponReparse_Client, [](CModule)
{
    DISPATCH_HOOK(ClientWeaponParseHooks, ConCommand_weapon_reparse)
});

ON_DLL_LOAD_CLIENT("client.dll", ClientWeaponParseSdk, [](CModule module)
{
    g_GetFileWeaponInfoFromHandleClient = module.Offset(0x3CB030).RCast<decltype(g_GetFileWeaponInfoFromHandleClient)>();
    g_GetFileWeaponInfoFromNameClient = module.Offset(0x3CB050).RCast<decltype(g_GetFileWeaponInfoFromNameClient)>();
    FileWeaponInfo_Client__Parse = module.Offset(0x3CFAC0).RCast<decltype(FileWeaponInfo_Client__Parse)>();
    g_ReadWeaponDataFromFileForSlotClient = module.Offset(0x3D2950).RCast<decltype(g_ReadWeaponDataFromFileForSlotClient)>();
    g_ReadEncryptedKVFileClient = module.Offset(0x3D2710).RCast<decltype(g_ReadEncryptedKVFileClient)>();
    g_AllocWeaponStringClient = module.Offset(0x3C9030).RCast<decltype(g_AllocWeaponStringClient)>();
    g_GetIndexForModNameClient = module.Offset(0x3CB1C0).RCast<decltype(g_GetIndexForModNameClient)>();
    g_CalcWeaponModsClient = module.Offset(0x3CA0B0).RCast<decltype(g_CalcWeaponModsClient)>();
    g_ParseWeaponModClient = module.Offset(0x3D15D0).RCast<decltype(g_ParseWeaponModClient)>();
    g_InsertWeaponModAssemblyItemClient = module.Offset(0x3C8F60).RCast<decltype(g_InsertWeaponModAssemblyItemClient)>();
    g_PrecacheImpactEffectTableClient = module.Offset(0x195CD0).RCast<decltype(g_PrecacheImpactEffectTableClient)>();
    g_PrecacheParticleSystemByNameClient = module.Offset(0x195F20).RCast<decltype(g_PrecacheParticleSystemByNameClient)>();
    g_PrecacheModelClient = module.Offset(0x3EDEB0).RCast<decltype(g_PrecacheModelClient)>();
    g_PrecacheScriptSoundClient = module.Offset(0x5B9A60).RCast<decltype(g_PrecacheScriptSoundClient)>();

    s_MPEDict_client = module.Offset(0x23F9920).RCast<RHashMapString*>();
    ModParseEntries_Client = module.Offset(0x942CA0).RCast<const WeaponModParseTableEntry*>();

    DISPATCH_HOOK(ClientWeaponParseHooks, UpdateWeaponUiValue)
});

using WeaponReparseCommand_t = void(__fastcall*)(const CCommand&);
static WeaponReparseCommand_t g_WeaponReparseServer;

static FileWeaponInfo_Server* (*g_GetFileWeaponInfoFromHandleServer)(WEAPON_FILE_INFO_HANDLE);
static KeyValues* (*g_ParseWeaponModServer)(KeyValues*, FileWeaponInfo_Server*, const char*, WeaponMod*);
static bool (*g_InsertWeaponModAssemblyItemServer)(WeaponModEntry_t*, const WeaponModParseTableEntry*, WeaponModAssemblyItem_t*);
static int (*g_PrecacheImpactEffectTableServer)(const char*);
static int (*g_PrecacheParticleSystemByNameServer)(const char*);
static int (*g_PrecacheModelServer)(const char*);
static void (*g_PrecacheScriptSoundServer)(CBaseEntity*, const char*);
static std::uint8_t* g_AimAssistAdspullClassesInitializedServer;

DECLARE_MODULE(ServerWeaponParseHooks)

static RHashMapString* s_MPEDict_server;
static const WeaponModParseTableEntry* ModParseEntries_Server;

FileWeaponInfo_Server* GetFileWeaponInfoFromHandle_Server(WEAPON_FILE_INFO_HANDLE handle)
{
    return g_GetFileWeaponInfoFromHandleServer(handle);
}

const WeaponModParseTableEntry* FindWeaponModParseTableEntry_Server(const char* fieldName)
{
    const auto* entryType = static_cast<const std::uint32_t*>(s_MPEDict_server->Find(fieldName));
    return entryType && *entryType != WMET_INVALID && *entryType < MAX_WEAPON_MOD_PARSE_ENTRIES ? &ModParseEntries_Server[*entryType] : nullptr;
}

const WeaponModParseTableEntry* GetWeaponModParseTableEntry_Server(WeaponModEntryType entryType)
{
    const auto index = static_cast<std::uint16_t>(entryType);
    return index < MAX_WEAPON_MOD_PARSE_ENTRIES ? &ModParseEntries_Server[index] : nullptr;
}

KeyValues* ParseWeaponMod_Server(KeyValues* section, FileWeaponInfo_Server* info, const char* weaponName, WeaponMod* mod)
{
    return g_ParseWeaponModServer(section, info, weaponName, mod);
}

bool InsertWeaponModAssemblyItem_Server(WeaponModEntry_t* entry, const WeaponModParseTableEntry* parseEntry, WeaponModAssemblyItem_t* item)
{
    return g_InsertWeaponModAssemblyItemServer(entry, parseEntry, item);
}

void PrecacheWeaponModAsset_Server(const WeaponModParseTableEntry& parseEntry, const char* assetName)
{
    if (parseEntry.parseFlags & WMPF_IMPACT_EFFECT_TABLE)
        g_PrecacheImpactEffectTableServer(assetName);
    else if (parseEntry.parseFlags & WMPF_PARTICLE_SYSTEM)
        g_PrecacheParticleSystemByNameServer(assetName);
}

int PrecacheWeaponModModel_Server(const char* modelName)
{
    return g_PrecacheModelServer(modelName);
}

void PrecacheWeaponModSound_Server(CBaseEntity* owner, const char* soundName)
{
    g_PrecacheScriptSoundServer(owner, soundName);
}

void WeaponReparseServer(const CCommand& args)
{
    if (g_WeaponReparseServer && g_pServer && g_pServer->IsActive())
        g_WeaponReparseServer(args);
}

DECLARE_HOOK(InitializeWeaponInfo_Server, server.dll + 0x6CB600, [](auto& hook, FileWeaponInfo_Server* weaponInfo) -> std::uintptr_t
{
    g_ServerWeaponMods.InitializeWeaponInfo(weaponInfo);
    return hook.Original(weaponInfo);
});

DECLARE_HOOK(ParseWeaponInfoFile_Server, server.dll + 0x6CE3F0,
             [](auto& hook, FileWeaponInfo_Server* weaponInfo, KeyValues* root, const char* weaponName) -> std::uint32_t*
{ return g_ServerWeaponMods.ParseWeaponInfo(weaponInfo, root, weaponName, hook.s_original); });

DECLARE_HOOK(ParseWeaponModGroups_Server, server.dll + 0x6CFA10,
             [](auto& hook, FileWeaponInfo_Server* weaponInfo, KeyValues* root, const char* weaponName, WeaponMod* outputGroups,
                std::uint32_t* outputGroupCount) -> std::uint32_t*
{ return g_ServerWeaponMods.ParseGroups(weaponInfo, root, weaponName, outputGroups, outputGroupCount, hook.s_original); });

DECLARE_HOOK(ParseWeaponModGroup_Server, server.dll + 0x6CFDE0,
             [](auto& hook, KeyValues* section, FileWeaponInfo_Server* weaponInfo, const char* weaponName, WeaponMod* outputGroup) -> KeyValues*
{ return g_ServerWeaponMods.ParseGroup(section, weaponInfo, weaponName, outputGroup, hook.s_original); });

DECLARE_HOOK(AssembleWeaponMods_Server, server.dll + 0x6C8B80,
             [](auto& hook, int enabledMods, FileWeaponInfo_Server* weaponInfo, WeaponModValues* output, bool singlePlayer,
                int removedMods) -> std::uint8_t
{
    const auto result = g_ServerWeaponMods.Assemble(enabledMods, weaponInfo, output, singlePlayer, removedMods, hook.s_original);
    g_ServerWeaponMods.ApplyFieldOverrides(output);
    return result;
});

DECLARE_HOOK(ApplyWeaponModEntry_Server, server.dll + 0x6C75D0, [](auto& hook, WeaponModEntry_t* entry, bool setBaseValue, bool remove) -> bool
{
    if (!CWeaponModHandler<FileWeaponInfo_Server>::HasActiveAssembly())
        return hook.Original(entry, setBaseValue, remove);

    return CWeaponModHandler<FileWeaponInfo_Server>::ApplyActiveEntry(entry, setBaseValue, remove);
});

DECLARE_HOOK(PrecacheWeaponModAssets_Server, server.dll + 0x6D0190, [](auto& hook, FileWeaponInfo_Server* weaponInfo, const WeaponMod* group) -> void
{ g_ServerWeaponMods.PrecacheAssets(weaponInfo, *group, hook.s_original); });

DECLARE_HOOK(PrecacheWeaponModStrings_Server, server.dll + 0x6D0680, [](auto& hook, FileWeaponInfo_Server* weaponInfo, const WeaponMod* group) -> void
{ g_ServerWeaponMods.PrecacheStringsNoResult(weaponInfo, *group, hook.s_original); });

DECLARE_HOOK(NotifyWeaponModStringField_Server, server.dll + 0x6D1970, [](auto& hook, CWeaponX* weapon, std::uint16_t fieldIndex) -> void
{ g_ServerWeaponMods.NotifyStringField(weapon, static_cast<WeaponModEntryType>(fieldIndex), hook.s_original); });

DECLARE_HOOK(ConCommand_weapon_reparse_server, server.dll + 0x6D1B70, [](auto& hook, const CCommand& args) -> void
{
    g_pModManager->InvalidateKeyValuesPatches("scripts/weapons/");
    hook.Original(args);
});

ON_DLL_LOAD("server.dll", WeaponMods_Server, [](CModule)
{
    g_ServerWeaponMods.Reset();
    DISPATCH_HOOK(ServerWeaponParseHooks, InitializeWeaponInfo_Server)
    DISPATCH_HOOK(ServerWeaponParseHooks, ParseWeaponInfoFile_Server)
    DISPATCH_HOOK(ServerWeaponParseHooks, ParseWeaponModGroups_Server)
    DISPATCH_HOOK(ServerWeaponParseHooks, ParseWeaponModGroup_Server)
    DISPATCH_HOOK(ServerWeaponParseHooks, AssembleWeaponMods_Server)
    DISPATCH_HOOK(ServerWeaponParseHooks, ApplyWeaponModEntry_Server)
    DISPATCH_HOOK(ServerWeaponParseHooks, PrecacheWeaponModAssets_Server)
    DISPATCH_HOOK(ServerWeaponParseHooks, PrecacheWeaponModStrings_Server)
    DISPATCH_HOOK(ServerWeaponParseHooks, NotifyWeaponModStringField_Server)
});

ON_DLL_LOAD("server.dll", WeaponReparse_Server, [](CModule)
{
    DISPATCH_HOOK(ServerWeaponParseHooks, ConCommand_weapon_reparse_server)
    g_WeaponReparseServer = HookSys::GetOriginalFunction<WeaponReparseCommand_t>(HookSys::FindHook("ConCommand_weapon_reparse_server"));
});

DECLARE_HOOK(AimAssistAdspullClassInit, server.dll + 0x6CC300, [](auto& hook) -> std::int64_t
{
    static std::mutex s_Mutex;

    const std::lock_guard lock(s_Mutex);
    if (*g_AimAssistAdspullClassesInitializedServer)
        return 0;

    return hook.Original();
})

ON_DLL_LOAD("server.dll", ServerWeaponParseSdk, [](CModule module)
{
    g_GetFileWeaponInfoFromHandleServer = module.Offset(0x6CA0A0).RCast<decltype(g_GetFileWeaponInfoFromHandleServer)>();
    g_ParseWeaponModServer = module.Offset(0x6CFDE0).RCast<decltype(g_ParseWeaponModServer)>();
    g_InsertWeaponModAssemblyItemServer = module.Offset(0x6C7690).RCast<decltype(g_InsertWeaponModAssemblyItemServer)>();
    g_PrecacheImpactEffectTableServer = module.Offset(0x159C00).RCast<decltype(g_PrecacheImpactEffectTableServer)>();
    g_PrecacheParticleSystemByNameServer = module.Offset(0x159E20).RCast<decltype(g_PrecacheParticleSystemByNameServer)>();
    g_PrecacheModelServer = module.Offset(0x429550).RCast<decltype(g_PrecacheModelServer)>();
    g_PrecacheScriptSoundServer = module.Offset(0x6A8C70).RCast<decltype(g_PrecacheScriptSoundServer)>();
    g_AimAssistAdspullClassesInitializedServer = module.Offset(0x160B477).RCast<std::uint8_t*>();

    s_MPEDict_server = module.Offset(0x1615BD0).RCast<RHashMapString*>();
    ModParseEntries_Server = module.Offset(0x997DC0).RCast<const WeaponModParseTableEntry*>();

    DISPATCH_HOOK(ServerWeaponParseHooks, AimAssistAdspullClassInit)
});
