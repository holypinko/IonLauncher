#pragma once

#include <interface.h>
#include "tier0/memaddr.h"
#include <string>

CMemory Sys_GetFactoryPtr(const std::string& svModuleName, const std::string& svFact);
