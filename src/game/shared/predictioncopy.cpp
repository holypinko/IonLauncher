#include "game/shared/predictioncopy.h"
#include "engine/client/clientstate.h"
#include "game/client/cliententitylist.h"
#include "icliententity.h"
#include "logging/logging.h"
#include "tier0/hooks.h"

#include <cstdio>
#include <cstring>
#include <string_view>

DECLARE_MODULE(ClientPredictionCopy)

thread_local int g_nPredictionErrorCommand = -1;

// annoying pred error in retail
bool IsRepeatedSkinPredictionError(const CPredictionCopy& copy, const datamap_t* map, const typedescription_t* field,
                                          const char* format, va_list args)
{
    if (!map || !map->dataClassName ||
        (std::strcmp(map->dataClassName, "CBaseViewModel") != 0 && std::strcmp(map->dataClassName, "CWeaponX") != 0) ||
        !field || field->fieldType != FIELD_INTEGER || field->fieldSize != 1 || !field->fieldName ||
        std::strcmp(field->fieldName, "m_nSkin") != 0 || !format ||
        std::strcmp(format, "int differs (net %i pred %i) diff(%i)\n") != 0)
        return false;

    if (!GetBaseLocalClient || !g_pClientEntityList || copy.m_nEntIndex < 0)
        return false;
    CClientState* const client = GetBaseLocalClient();
    IClientEntity* const entity = g_pClientEntityList->GetClientEntity(copy.m_nEntIndex);
    if (!client || !entity)
        return false;
    const CBaseHandle handle = entity->GetRefEHandle();
    if (!handle.IsValid() || handle.GetEntryIndex() != copy.m_nEntIndex)
        return false;

    va_list skinArgs;
    va_copy(skinArgs, args);
    const int networkSkin = va_arg(skinArgs, int);
    const int predictedSkin = va_arg(skinArgs, int);
    va_end(skinArgs);

    auto& errors = client->GetClientStateExtended()->m_PredictionSkinErrorLog;
    if (errors.owner != client || errors.channel != client->m_NetChannel || errors.serverCount != client->m_nServerCount)
    {
        errors = {};
        errors.owner = client;
        errors.channel = client->m_NetChannel;
        errors.serverCount = client->m_nServerCount;
    }
    for (auto& entry : errors.entries)
    {
        if (entry.entity != handle)
            continue;
        const bool repeated = entry.networkSkin == networkSkin && entry.predictedSkin == predictedSkin;
        entry.networkSkin = networkSkin;
        entry.predictedSkin = predictedSkin;
        return repeated;
    }
    errors.entries[errors.nextEntry] = {handle, networkSkin, predictedSkin};
    errors.nextEntry = (errors.nextEntry + 1) % errors.entries.size();
    return false;
}
DECLARE_HOOK_CC(CPredictionCopy_ReportFieldsDiffer, client.dll + 0x2DE3D0, __cdecl,
                [](auto& hook, CPredictionCopy* self, const datamap_t* map, const typedescription_t* field, const char* format, ...)
{
    ++self->m_nErrorCount;
    if (self->m_FieldCompareFunc)
        return;

	if (IsRepeatedSkinPredictionError(*self, map, field, format, *hook.VarArgs()))
        return;

    char message[4096];
    va_list args;
    va_copy(args, *hook.VarArgs());
    vsnprintf_s(message, sizeof(message), _TRUNCATE, format, args);
    va_end(args);
    std::string_view text(message);
    while (!text.empty() && (text.back() == '\n' || text.back() == '\r'))
        text.remove_suffix(1);

    NS::log::NATIVE_CL->info("[prediction] cmd={} ent={} error={} {}::{} tolerance={} {}", g_nPredictionErrorCommand, self->m_nEntIndex,
                             self->m_nErrorCount, map && map->dataClassName ? map->dataClassName : "unknown",
                             field && field->fieldName ? field->fieldName : "unknown", field ? field->fieldTolerance : 0.0f, text);
})

ON_DLL_LOAD_CLIENT("client.dll", PredictionCopy, [](CModule)
{
    DISPATCH_MODULE(ClientPredictionCopy)
})
