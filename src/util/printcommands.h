#pragma once
#include "tier1/convar.h"

void PrintCommandHelpDialogue(const ConCommandBase* command, const char* name);
void TryPrintCvarHelpForCommand(const char* pCommand);
void InitialiseCommandPrint();
