#include "game/client/c_basecombatweapon.h"
#include "tier0/callbacks.h"

using C_BaseCombatWeapon__GetPrintName_t = ScriptStringOrNull* (*)(const C_BaseCombatWeapon*, ScriptStringOrNull*);
using C_BaseCombatWeapon__GetWeaponDescription_t = ScriptStringOrNull* (*)(const C_BaseCombatWeapon*, ScriptStringOrNull*);
using C_BaseCombatWeapon__Script_SetDroppedModel_t = void (*)(C_BaseCombatWeapon*, const char*);
using C_BaseCombatWeapon__ScriptLookupWorldModelAttachment_t = int (*)(C_BaseCombatWeapon*, const char*);
using C_BaseCombatWeapon__ScriptLookupViewModelAttachment_t = int (*)(C_BaseCombatWeapon*, const char*);

static C_BaseCombatWeapon__GetPrintName_t C_BaseCombatWeapon__GetPrintName;
static C_BaseCombatWeapon__GetWeaponDescription_t C_BaseCombatWeapon__GetWeaponDescription;
static C_BaseCombatWeapon__Script_SetDroppedModel_t C_BaseCombatWeapon__Script_SetDroppedModel;
static C_BaseCombatWeapon__ScriptLookupWorldModelAttachment_t C_BaseCombatWeapon__ScriptLookupWorldModelAttachment;
static C_BaseCombatWeapon__ScriptLookupViewModelAttachment_t C_BaseCombatWeapon__ScriptLookupViewModelAttachment;

ScriptStringOrNull C_BaseCombatWeapon::GetPrintName() const
{
    ScriptStringOrNull result;
    C_BaseCombatWeapon__GetPrintName(this, &result);
    return result;
}

ScriptStringOrNull C_BaseCombatWeapon::GetWeaponDescription() const
{
    ScriptStringOrNull result;
    C_BaseCombatWeapon__GetWeaponDescription(this, &result);
    return result;
}

void C_BaseCombatWeapon::Script_SetDroppedModel(const char* modelName)
{
    return C_BaseCombatWeapon__Script_SetDroppedModel(this, modelName);
}

int C_BaseCombatWeapon::ScriptLookupWorldModelAttachment(const char* attachmentName)
{
    return C_BaseCombatWeapon__ScriptLookupWorldModelAttachment(this, attachmentName);
}

int C_BaseCombatWeapon::ScriptLookupViewModelAttachment(const char* attachmentName)
{
    return C_BaseCombatWeapon__ScriptLookupViewModelAttachment(this, attachmentName);
}

ON_DLL_LOAD_CLIENT("client.dll", BaseCombatWeaponSdkMethods, [](CModule module)
{
    C_BaseCombatWeapon__GetPrintName = module.Offset(0xBB180).RCast<C_BaseCombatWeapon__GetPrintName_t>();
    C_BaseCombatWeapon__GetWeaponDescription = module.Offset(0xBB360).RCast<C_BaseCombatWeapon__GetWeaponDescription_t>();
    C_BaseCombatWeapon__Script_SetDroppedModel = module.Offset(0xBD340).RCast<C_BaseCombatWeapon__Script_SetDroppedModel_t>();
    C_BaseCombatWeapon__ScriptLookupWorldModelAttachment = module.Offset(0xBD260).RCast<C_BaseCombatWeapon__ScriptLookupWorldModelAttachment_t>();
    C_BaseCombatWeapon__ScriptLookupViewModelAttachment = module.Offset(0xBD180).RCast<C_BaseCombatWeapon__ScriptLookupViewModelAttachment_t>();
});
