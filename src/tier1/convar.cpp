#include "tier1/convar.h"
#include "vstdlib/cvar.h"

#include <cstddef>
#include <cstring>
#include <float.h>

#include <utility>

using ConVarRegister_t = void (*)(
	ConVar* conVar,
	const char* name,
	const char* defaultValue,
	int flags,
	const char* helpString,
	bool hasMin,
	float minValue,
	bool hasMax,
	float maxValue,
	FnChangeCallback_t callback);
static ConVarRegister_t ConVar__Create;

using ConVarCallbacksConstructor_t = void (*)(CUtlVector<ConVar::CVChange_t>* callbacks, int growSize, int initialCapacity);
static ConVarCallbacksConstructor_t g_ConVarCallbacksConstructor;

static void* g_pConVarVTable;
static void* g_pIConVarVTable;

static constexpr std::pair<int, const char*> g_ConVarFlagNames[] = {
	{FCVAR_UNREGISTERED, "UNREGISTERED"},
	{FCVAR_DEVELOPMENTONLY, "DEVELOPMENTONLY"},
	{FCVAR_GAMEDLL, "GAMEDLL"},
	{FCVAR_CLIENTDLL, "CLIENTDLL"},
	{FCVAR_HIDDEN, "HIDDEN"},
	{FCVAR_PROTECTED, "PROTECTED"},
	{FCVAR_SPONLY, "SPONLY"},
	{FCVAR_ARCHIVE, "ARCHIVE"},
	{FCVAR_NOTIFY, "NOTIFY"},
	{FCVAR_USERINFO, "USERINFO"},
	{FCVAR_PRINTABLEONLY, "PRINTABLEONLY"},
	{FCVAR_GAMEDLL_FOR_REMOTE_CLIENTS, "GAMEDLL_FOR_REMOTE_CLIENTS"},
	{FCVAR_UNLOGGED, "UNLOGGED"},
	{FCVAR_NEVER_AS_STRING, "NEVER_AS_STRING"},
	{FCVAR_REPLICATED, "REPLICATED"},
	{FCVAR_CHEAT, "CHEAT"},
	{FCVAR_SS, "SS"},
	{FCVAR_DEMO, "DEMO"},
	{FCVAR_DONTRECORD, "DONTRECORD"},
	{FCVAR_SS_ADDED, "SS_ADDED"},
	{FCVAR_RELEASE, "RELEASE"},
	{FCVAR_RELOAD_MATERIALS, "RELOAD_MATERIALS"},
	{FCVAR_RELOAD_TEXTURES, "RELOAD_TEXTURES"},
	{FCVAR_NOT_CONNECTED, "NOT_CONNECTED"},
	{FCVAR_MATERIAL_SYSTEM_THREAD, "MATERIAL_SYSTEM_THREAD"},
	{FCVAR_ARCHIVE_PLAYERPROFILE, "ARCHIVE_PLAYERPROFILE"},
	{FCVAR_ACCESSIBLE_FROM_THREADS, "ACCESSIBLE_FROM_THREADS"},
	{FCVAR_STUDIO_SYSTEM, "STUDIO_SYSTEM"},
	{FCVAR_SERVER_FRAME_THREAD, "SERVER_FRAME_THREAD"},
	{FCVAR_SERVER_CAN_EXECUTE, "SERVER_CAN_EXECUTE"},
	{FCVAR_SERVER_CANNOT_QUERY, "SERVER_CANNOT_QUERY"},
	{FCVAR_CLIENTCMD_CAN_EXECUTE, "CLIENTCMD_CAN_EXECUTE"},
	{static_cast<int>(FCVAR_PLATFORM_SYSTEM), "PLATFORM_SYSTEM"},
};

std::span<const std::pair<int, const char*>> GetConVarFlagNames()
{
	return g_ConVarFlagNames;
}

using CCommand__Tokenize_t = bool (*)(CCommand&, const char*, cmd_source_t);
static CCommand__Tokenize_t CCommand__Tokenize;

using ConCommandConstructor_t =
	void (*)(ConCommand* command, const char* name, FnCommandCallback_t callback, const char* helpString, int flags, void* parent);
static ConCommandConstructor_t ConCommand__ConCommand;

ON_DLL_LOAD("engine.dll", ConCommand, [](CModule module)
{
	ConCommand__ConCommand = module.Offset(0x415F60).RCast<ConCommandConstructor_t>();
    CCommand__Tokenize = module.Offset(0x418380).RCast<CCommand__Tokenize_t>();
})

//-----------------------------------------------------------------------------
// Purpose: ConVar interface initialization
//-----------------------------------------------------------------------------
ON_DLL_LOAD("engine.dll", ConVar, [](CModule module)
{
	g_ConVarCallbacksConstructor = module.Offset(0x415C20).RCast<ConVarCallbacksConstructor_t>();
	ConVar__Create = module.Offset(0x417230).RCast<ConVarRegister_t>();
	g_pConVarVTable = module.Offset(0x67FD28).RCast<void*>();
	g_pIConVarVTable = module.Offset(0x67FDC8).RCast<void*>();

	InitialiseCVarInterface();
})

//-----------------------------------------------------------------------------
// Purpose: constructor
//-----------------------------------------------------------------------------
ConVar::ConVar(const char* pszName, const char* pszDefaultValue, int nFlags, const char* pszHelpString)
{
	spdlog::info("Registering Convar {}", pszName);

	*reinterpret_cast<void**>(static_cast<ConCommandBase*>(this)) = g_pConVarVTable;
	*reinterpret_cast<void**>(static_cast<IConVar*>(this)) = g_pIConVarVTable;
	g_ConVarCallbacksConstructor(&m_fnChangeCallbacks, 0, 0);
	ConVar__Create(this, pszName, pszDefaultValue, nFlags, pszHelpString, false, 0.0f, false, 0.0f, nullptr);
}

//-----------------------------------------------------------------------------
// Purpose: constructor
//-----------------------------------------------------------------------------
ConVar::ConVar(
	const char* pszName,
	const char* pszDefaultValue,
	int nFlags,
	const char* pszHelpString,
	bool bMin,
	float fMin,
	bool bMax,
	float fMax,
	FnChangeCallback_t pCallback)
{
	spdlog::info("Registering Convar {}", pszName);

	*reinterpret_cast<void**>(static_cast<ConCommandBase*>(this)) = g_pConVarVTable;
	*reinterpret_cast<void**>(static_cast<IConVar*>(this)) = g_pIConVarVTable;
	g_ConVarCallbacksConstructor(&m_fnChangeCallbacks, 0, 0);
	ConVar__Create(this, pszName, pszDefaultValue, nFlags, pszHelpString, bMin, fMin, bMax, fMax, pCallback);
}

// Purpose: destructor
//-----------------------------------------------------------------------------
ConVar::~ConVar()
{
	if (m_pParent == this && m_Value.m_pszString)
		delete[] m_Value.m_pszString;
}

//-----------------------------------------------------------------------------
// Purpose: Returns the base ConVar name.
// Output : const char*
//-----------------------------------------------------------------------------
const char* ConVar::GetName() const
{
	return m_pParent->m_pszName;
}

const char* ConVar::GetBaseName() const
{
	return m_pParent->m_pszName;
}

//-----------------------------------------------------------------------------
// Purpose: Returns the ConVar help text.
// Output : const char*
//-----------------------------------------------------------------------------
const char* ConVar::GetHelpText() const
{
	return m_pParent->m_pszHelpString;
}

//-----------------------------------------------------------------------------
// Purpose: Add's flags to ConVar.
// Input  : nFlags -
//-----------------------------------------------------------------------------
void ConVar::AddFlags(int flags)
{
	m_pParent->m_nFlags |= flags;
}

//-----------------------------------------------------------------------------
// Purpose: Removes flags from ConVar.
// Input  : nFlags -
//-----------------------------------------------------------------------------
void ConVar::RemoveFlags(int flags)
{
	m_pParent->m_nFlags &= ~flags;
}

int ConVar::GetFlags() const
{
	return m_pParent->m_nFlags;
}

int ConVar::GetSplitScreenPlayerSlot() const
{
	return 0;
}

//-----------------------------------------------------------------------------
// Purpose: Return ConVar value as a boolean.
// Output : bool
//-----------------------------------------------------------------------------
bool ConVar::GetBool(void) const
{
	return !!GetInt();
}

//-----------------------------------------------------------------------------
// Purpose: Return ConVar value as a float.
// Output : float
//-----------------------------------------------------------------------------
float ConVar::GetFloat() const
{
	return m_pParent->m_Value.m_fValue;
}

//-----------------------------------------------------------------------------
// Purpose: Return ConVar value as an integer.
// Output : int
//-----------------------------------------------------------------------------
int ConVar::GetInt() const
{
	return m_pParent->m_Value.m_nValue;
}

//-----------------------------------------------------------------------------
// Purpose: Return ConVar value as a color.
// Output : Color
//-----------------------------------------------------------------------------
Color ConVar::GetColor() const
{
	const unsigned char* color = reinterpret_cast<const unsigned char*>(&m_pParent->m_Value.m_nValue);
	return Color(color[0], color[1], color[2], color[3]);
}

//-----------------------------------------------------------------------------
// Purpose: Return ConVar value as a string.
// Output : const char *
//-----------------------------------------------------------------------------
const char* ConVar::GetString() const
{
	if (m_pParent->m_nFlags & FCVAR_NEVER_AS_STRING)
		return "FCVAR_NEVER_AS_STRING";

	const char* value = m_pParent->m_Value.m_pszString;
	return value ? value : "";
}

//-----------------------------------------------------------------------------
// Purpose:
// Input  : flMinVal -
// Output : true if there is a min set.
//-----------------------------------------------------------------------------
bool ConVar::GetMin(float& minValue) const
{
	minValue = m_pParent->m_fMinVal;
	return m_pParent->m_bHasMin;
}

//-----------------------------------------------------------------------------
// Purpose:
// Input  : flMaxVal -
// Output : true if there is a max set.
//-----------------------------------------------------------------------------
bool ConVar::GetMax(float& maxValue) const
{
	maxValue = m_pParent->m_fMaxVal;
	return m_pParent->m_bHasMax;
}

//-----------------------------------------------------------------------------
// Purpose: returns the min value.
// Output : float
//-----------------------------------------------------------------------------
float ConVar::GetMinValue() const
{
	return m_pParent->m_fMinVal;
}

//-----------------------------------------------------------------------------
// Purpose: returns the max value.
// Output : float
//-----------------------------------------------------------------------------
float ConVar::GetMaxValue() const
{
	return m_pParent->m_fMaxVal;
}

//-----------------------------------------------------------------------------
// Purpose: checks if ConVar has min value.
// Output : bool
//-----------------------------------------------------------------------------
bool ConVar::HasMin() const
{
	return m_pParent->m_bHasMin;
}

//-----------------------------------------------------------------------------
// Purpose: checks if ConVar has max value.
// Output : bool
//-----------------------------------------------------------------------------
bool ConVar::HasMax() const
{
	return m_pParent->m_bHasMax;
}

//-----------------------------------------------------------------------------
// Purpose: sets the ConVar int value.
// Input  : nValue -
//-----------------------------------------------------------------------------
void ConVar::SetValue(int value)
{
	ConVar* const parent = m_pParent;
	if (value == parent->m_Value.m_nValue)
		return;

	float floatValue = static_cast<float>(value);
	if (ClampValue(floatValue))
		value = static_cast<int>(floatValue);

	const float oldValue = parent->m_Value.m_fValue;
	parent->m_Value.m_fValue = floatValue;
	parent->m_Value.m_nValue = value;

	if (!(parent->m_nFlags & FCVAR_NEVER_AS_STRING))
	{
		char text[32];
		snprintf(text, sizeof(text), "%d", parent->m_Value.m_nValue);
		ChangeStringValue(text, oldValue);
	}
}

//-----------------------------------------------------------------------------
// Purpose: sets the ConVar float value.
// Input  : flValue -
//-----------------------------------------------------------------------------
void ConVar::SetValue(float value)
{
	ConVar* const parent = m_pParent;
	if (value == parent->m_Value.m_fValue)
		return;

	ClampValue(value);

	const float oldValue = parent->m_Value.m_fValue;
	parent->m_Value.m_fValue = value;
	parent->m_Value.m_nValue = static_cast<int>(value);

	if (!(parent->m_nFlags & FCVAR_NEVER_AS_STRING))
	{
		char text[32];
		snprintf(text, sizeof(text), "%f", value);
		ChangeStringValue(text, oldValue);
	}
}

//-----------------------------------------------------------------------------
// Purpose: sets the ConVar string value.
// Input  : *szValue -
//-----------------------------------------------------------------------------
void ConVar::SetValue(const char* value)
{
	ConVar* const parent = m_pParent;
	if (value && parent->m_Value.m_pszString && strcmp(parent->m_Value.m_pszString, value) == 0)
		return;

	char clampedText[32] {};
	const char* newValue = value ? value : "";
	const float oldValue = parent->m_Value.m_fValue;

	if (!SetColorFromString(newValue))
	{
		float floatValue = static_cast<float>(atof(newValue));
		if (!std::isfinite(floatValue))
		{
			spdlog::warn("Warning: ConVar '{}' = '{}' is infinite, clamping value.\n", GetBaseName(), newValue);
			floatValue = FLT_MAX;
		}

		if (ClampValue(floatValue))
		{
			snprintf(clampedText, sizeof(clampedText), "%f", floatValue);
			newValue = clampedText;
		}

		parent->m_Value.m_fValue = floatValue;
		parent->m_Value.m_nValue = static_cast<int>(floatValue);
	}

	if (!(parent->m_nFlags & FCVAR_NEVER_AS_STRING))
		ChangeStringValue(newValue, oldValue);
}

//-----------------------------------------------------------------------------
// Purpose: sets the ConVar color value.
// Input  : clValue -
//-----------------------------------------------------------------------------
void ConVar::SetValue(Color value)
{
	std::string text;
	for (int i = 0; i < 4; ++i)
	{
		if (value.GetValue(i) != 0 || !text.empty())
		{
			text += std::to_string(value.GetValue(i));
			text.push_back(' ');
		}
	}
	SetValue(text.c_str());
}

//-----------------------------------------------------------------------------
// Purpose: changes the ConVar string value.
// Input  : *pszTempVal - flOldValue
//-----------------------------------------------------------------------------
void ConVar::ChangeStringValue(const char* value, float oldValue)
{
	NOTE_UNUSED(oldValue);
	ConVar* const parent = m_pParent;
	assert(!(parent->m_nFlags & FCVAR_NEVER_AS_STRING));

	char* oldString = static_cast<char*>(_malloca(parent->m_Value.m_iStringLength));
	if (oldString)
		memcpy(oldString, parent->m_Value.m_pszString, parent->m_Value.m_iStringLength);

	if (value)
	{
		const size_t length = strlen(value) + 1;
		if (length > parent->m_Value.m_iStringLength)
		{
			if (parent->m_Value.m_pszString)
				delete[] parent->m_Value.m_pszString;

			parent->m_Value.m_pszString = new char[length];
			parent->m_Value.m_iStringLength = length;
		}

		memcpy(const_cast<char*>(parent->m_Value.m_pszString), value, length);
	}
	else
	{
		parent->m_Value.m_pszString = nullptr;
	}

	_freea(oldString);
}

//-----------------------------------------------------------------------------
// Purpose: sets the ConVar color value from string.
// Input  : *pszValue -
//-----------------------------------------------------------------------------
bool ConVar::SetColorFromString(const char* value)
{
	int rgba[4] {};
	const int count = sscanf_s(value, "%i %i %i %i", &rgba[0], &rgba[1], &rgba[2], &rgba[3]);
	if (count < 3)
		return false;

	if (count == 3)
		rgba[3] = 255;

	for (int component : rgba)
	{
		if (component < 0 || component > 255)
			return false;
	}

	ConVar* const parent = m_pParent;
	unsigned char* color = reinterpret_cast<unsigned char*>(&parent->m_Value.m_nValue);
	color[0] = static_cast<unsigned char>(rgba[0]);
	color[1] = static_cast<unsigned char>(rgba[1]);
	color[2] = static_cast<unsigned char>(rgba[2]);
	color[3] = static_cast<unsigned char>(rgba[3]);
	parent->m_Value.m_fValue = static_cast<float>(parent->m_Value.m_nValue);
	return true;
}

//-----------------------------------------------------------------------------
// Purpose: Checks if ConVar is registered.
// Output : bool
//-----------------------------------------------------------------------------
bool ConVar::IsRegistered() const
{
	return m_pParent->m_bRegistered;
}

//-----------------------------------------------------------------------------
// Purpose: Returns true if this is a command
// Output : bool
//-----------------------------------------------------------------------------
bool ConVar::IsCommand() const
{
	return false;
}

//-----------------------------------------------------------------------------
// Purpose: Test each ConVar query before setting the value.
// Input  : nFlags
// Output : False if change is permitted, true if not.
//-----------------------------------------------------------------------------
bool ConVar::IsFlagSet(int flags) const
{
	return (m_pParent->m_nFlags & flags) != 0;
}

//-----------------------------------------------------------------------------
// Purpose: Check whether to clamp and then perform clamp.
// Input  : flValue -
// Output : Returns true if value changed.
//-----------------------------------------------------------------------------
bool ConVar::ClampValue(float& value)
{
	ConVar* const parent = m_pParent;
	if (parent->m_bHasMin && value < parent->m_fMinVal)
	{
		value = parent->m_fMinVal;
		return true;
	}

	if (parent->m_bHasMax && value > parent->m_fMaxVal)
	{
		value = parent->m_fMaxVal;
		return true;
	}

	return false;
}


bool CCommand::Tokenize(const char* commandString, cmd_source_t commandSource)
{
    return CCommand__Tokenize(*this, commandString, commandSource);
}

ConCommandBase* ConCommandBase::s_pConCommandBases = nullptr;
IConCommandBaseAccessor* ConCommandBase::s_pAccessor = nullptr;

static int s_ConVarDLLIdentifier = -1;

CCommand::CCommand(const CCommand& other)
{
	m_nArgc = other.m_nArgc;
	m_nArgv0Size = other.m_nArgv0Size;
	std::memcpy(m_pArgSBuffer, other.m_pArgSBuffer, sizeof(m_pArgSBuffer));
	std::memcpy(m_pArgvBuffer, other.m_pArgvBuffer, sizeof(m_pArgvBuffer));

	for (int i = 0; i < COMMAND_MAX_ARGC; ++i)
	{
		if (!other.m_ppArgv[i])
		{
			m_ppArgv[i] = nullptr;
			continue;
		}

		const std::ptrdiff_t offset = other.m_ppArgv[i] - other.m_pArgvBuffer;
		m_ppArgv[i] = offset >= 0 && offset < COMMAND_MAX_LENGTH ? m_pArgvBuffer + offset : nullptr;
	}
}

std::int64_t CCommand::ArgC() const
{
	return m_nArgc;
}

const char** CCommand::ArgV() const
{
	return m_nArgc ? const_cast<const char**>(m_ppArgv) : nullptr;
}

const char* CCommand::ArgS() const
{
	return m_nArgv0Size ? &m_pArgSBuffer[m_nArgv0Size] : "";
}

const char* CCommand::GetCommandString() const
{
	return m_nArgc ? m_pArgSBuffer : "";
}

const char* CCommand::Arg(int index) const
{
	return index >= 0 && index < m_nArgc ? m_ppArgv[index] : "";
}

const char* CCommand::operator[](int index) const
{
	return Arg(index);
}

int CCommand::MaxCommandLength()
{
	return COMMAND_MAX_LENGTH - 1;
}

ConCommandBase::~ConCommandBase() = default;

bool ConCommandBase::IsCommand() const
{
	return true;
}

bool ConCommandBase::IsFlagSet(int flags) const
{
	return (m_nFlags & flags) != 0;
}

void ConCommandBase::AddFlags(int flags)
{
	m_nFlags |= flags;
}

void ConCommandBase::RemoveFlags(int flags)
{
	m_nFlags &= ~flags;
}

int ConCommandBase::GetFlags() const
{
	return m_nFlags;
}

const char* ConCommandBase::GetName() const
{
	return m_pszName;
}

const char* ConCommandBase::GetHelpText() const
{
	return m_pszHelpString;
}

bool ConCommandBase::IsRegistered() const
{
	return m_bRegistered;
}

int ConCommandBase::GetDLLIdentifier() const
{
	return s_ConVarDLLIdentifier;
}

ConCommandBase* ConCommandBase::Create(const char* name, const char* helpString, int flags)
{
	m_bRegistered = false;
	m_pszName = name;
	m_pszHelpString = helpString ? helpString : "";
	m_nFlags = flags;

	if (flags & FCVAR_UNREGISTERED)
	{
		m_pNext = nullptr;
	}
	else
	{
		m_pNext = s_pConCommandBases;
		s_pConCommandBases = this;
	}

	if (s_pAccessor)
		Init();

	return this;
}

void ConCommandBase::Init()
{
	if (s_pAccessor)
		m_bRegistered = s_pAccessor->RegisterConCommandBase(this);
}

bool ConCommandBase::HasFlags(int flags) const
{
	return (m_nFlags & flags) != 0;
}

ConCommandBase* ConCommandBase::GetNext() const
{
	return m_pNext;
}

char* ConCommandBase::CopyString(const char* from) const
{
	const std::size_t length = std::strlen(from);
	char* copy = new char[length + 1];
	std::memcpy(copy, from, length + 1);
	return copy;
}

bool ConCommand::IsCommand() const
{
	return true;
}

ConCommand::ConCommand(const char* name, FnCommandCallback_t callback, const char* helpString, int flags)
{
	ConCommand__ConCommand(this, name, callback, helpString, flags, nullptr);
}


void RegisterConCommand(const char* name, FnCommandCallback_t callback, const char* helpString, int flags)
{
	spdlog::info("Registering ConCommand {}", name);
	new ConCommand(name, callback, helpString, flags);
}

void RegisterConCommand(
	const char* name,
	FnCommandCallback_t callback,
	const char* helpString,
	int flags,
	FnCommandCompletionCallback completionCallback)
{
	spdlog::info("Registering ConCommand {}", name);
	ConCommand* command = new ConCommand(name, callback, helpString, flags);
	command->m_pCompletionCallback = completionCallback;
	command->m_nCallbackFlags |= 0x3;
}

