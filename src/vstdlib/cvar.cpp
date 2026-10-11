#include "vstdlib/cvar.h"
#include "tier1/interface.h"
#include "tier1/cvar.h"

//-----------------------------------------------------------------------------
// Purpose: returns all ConVars
//-----------------------------------------------------------------------------
std::unordered_map<std::string, ConCommandBase*> CCvar::DumpToMap()
{
	CCVarIteratorInternal* itint = FactoryInternalIterator(); // Allocate new InternalIterator.

	std::unordered_map<std::string, ConCommandBase*> allConVars;

	for (itint->SetFirst(); itint->IsValid(); itint->Next()) // Loop through all instances.
	{
		ConCommandBase* pCommand = itint->Get();
		allConVars[pCommand->GetName()] = pCommand;
	}

	return allConVars;
}

CCvar* g_pCVar;

void InitialiseCVarInterface()
{
	g_pCVar = Sys_GetFactoryPtr("vstdlib.dll", CVAR_INTERFACE_VERSION).RCast<CCvar*>();
}
