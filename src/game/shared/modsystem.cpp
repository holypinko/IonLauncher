#include "modsystem/modbrowser.h"
#include "modsystem/modinstaller.h"
#include "modsystem/modinventory.h"
#include "modsystem/modmanager.h"
#include "modsystem/platform/modworkshop.h"
#include "vscript/languages/squirrel_re/squirrel.h"

#include <Windows.h>
#include <shellapi.h>
#include <algorithm>
#include <array>
#include <filesystem>
#include <limits>
#include <mutex>
#include <string>
#include <string_view>
#include <vector>

class CModMenuSquirrel final
{
public:
	static bool FindModIcon(const Mod& mod, fs::path& iconPath);
	static const ModTrackedPackage* FindManagedPackage(const Mod& mod, const ModInventorySnapshot* inventory);

	template <ScriptContext context> static void PushMod(HSQUIRRELVM sqvm, Mod& mod, const ModInventorySnapshot* inventory, size_t modIndex);


private:
	static fs::path FindRemotePackageRoot(const Mod& mod);
	static std::vector<std::string> CollectModAssets(const Mod& mod);
	static void AddAsset(std::vector<std::string>& assets, std::string_view type, std::string_view value);
	inline static constexpr std::array<std::string_view, 4> ICON_FILENAMES = {"icon.webp", "icon.png", "icon.jpg", "icon.jpeg"};
};

fs::path CModMenuSquirrel::FindRemotePackageRoot(const Mod& mod)
{
	if (!mod.IsRemote())
		return {};
	const fs::path remoteRoot = GetRemoteModFolderPath().lexically_normal();
	const fs::path relative = mod.m_ModDirectory.lexically_normal().lexically_relative(remoteRoot);
	const auto first = relative.begin();
	if (relative.empty() || relative == "." || first == relative.end() || *first == "..")
		return {};
	return remoteRoot / *first;
}

bool CModMenuSquirrel::FindModIcon(const Mod& mod, fs::path& iconPath)
{
	const std::array<fs::path, 3> roots = {mod.m_PackageDirectory, mod.m_ModDirectory, FindRemotePackageRoot(mod)};
	for (const fs::path& root : roots)
	{
		if (root.empty())
			continue;
		for (const std::string_view filename : ICON_FILENAMES)
		{
			const fs::path candidate = root / filename;
			std::error_code error;
			if (fs::is_regular_file(candidate, error) && !error)
			{
				iconPath = fs::canonical(candidate, error);
				if (!error)
					return true;
			}
		}
	}
	return false;
}

const ModTrackedPackage* CModMenuSquirrel::FindManagedPackage(const Mod& mod, const ModInventorySnapshot* inventory)
{
	if (!inventory || !mod.m_ManagedId || mod.m_ManagedId->empty())
		return nullptr;
	const auto package = std::ranges::find_if(inventory->packages, [&mod](const ModTrackedPackage& candidate) {
		return candidate.source == mod.m_Source && candidate.packageId == *mod.m_ManagedId;
	});
	return package == inventory->packages.end() ? nullptr : &*package;
}


void CModMenuSquirrel::AddAsset(std::vector<std::string>& assets, std::string_view type, std::string_view value)
{
	if (value.empty())
		return;
	std::string& asset = assets.emplace_back();
	asset.reserve(type.size() + value.size() + 2);
	asset.append(type).append(": ").append(value);
}


std::vector<std::string> CModMenuSquirrel::CollectModAssets(const Mod& mod)
{
	std::vector<std::string> assets;

	for (const ModScript& script : mod.Scripts)
		AddAsset(assets, "Script", script.Path);
	AddAsset(assets, "Init script", mod.initScript);
	for (const ModVPKEntry& vpk : mod.Vpks)
		AddAsset(assets, "VPK", vpk.m_sVpkPath);
	for (const ModRpakEntry& rpak : mod.Rpaks)
		AddAsset(assets, "RPAK", rpak.m_pakName);
	for (const std::string& path : mod.LocalisationFiles)
		AddAsset(assets, "Localization", path);
	for (const std::string& path : mod.BinkVideos)
		AddAsset(assets, "Bink", path);
	for (const auto& [hash, path] : mod.KeyValues)
	{
		NOTE_UNUSED(hash);
		AddAsset(assets, "KeyValues", path);
	}
	AddAsset(assets, "PDiff", mod.Pdiff);

	std::ranges::sort(assets);
	assets.erase(std::ranges::unique(assets).begin(), assets.end());
	return assets;
}

template <ScriptContext context>
void CModMenuSquirrel::PushMod(HSQUIRRELVM sqvm, Mod& mod, const ModInventorySnapshot* inventory, size_t modIndex)
{
	g_pSquirrel[context]->pushnewstructinstance(sqvm, 21);

	// name
	g_pSquirrel[context]->pushstring(sqvm, mod.Name.c_str(), -1);
	g_pSquirrel[context]->sealstructslot(sqvm, 0);

	// description
	g_pSquirrel[context]->pushstring(sqvm, mod.Description.c_str(), -1);
	g_pSquirrel[context]->sealstructslot(sqvm, 1);

	// version
	g_pSquirrel[context]->pushstring(sqvm, mod.Version.c_str(), -1);
	g_pSquirrel[context]->sealstructslot(sqvm, 2);

	// download link
	g_pSquirrel[context]->pushstring(sqvm, mod.DownloadLink.c_str(), -1);
	g_pSquirrel[context]->sealstructslot(sqvm, 3);

	// load priority
	g_pSquirrel[context]->pushinteger(sqvm, mod.LoadPriority);
	g_pSquirrel[context]->sealstructslot(sqvm, 4);

	// enabled
	g_pSquirrel[context]->pushbool(sqvm, mod.m_bEnabled);
	g_pSquirrel[context]->sealstructslot(sqvm, 5);

	// required on client
	g_pSquirrel[context]->pushbool(sqvm, mod.RequiredOnClient);
	g_pSquirrel[context]->sealstructslot(sqvm, 6);

	// is remote
	g_pSquirrel[context]->pushbool(sqvm, mod.IsRemote());
	g_pSquirrel[context]->sealstructslot(sqvm, 7);

	// convars
	g_pSquirrel[context]->newarray(sqvm);
	for (ModConVar* cvar : mod.ConVars)
	{
		g_pSquirrel[context]->pushstring(sqvm, cvar->Name.c_str());
		g_pSquirrel[context]->arrayappend(sqvm, -2);
	}
	g_pSquirrel[context]->sealstructslot(sqvm, 8);

	const std::string managedId = mod.m_ManagedId.value_or("");
	g_pSquirrel[context]->pushstring(sqvm, managedId.c_str(), -1);
	g_pSquirrel[context]->sealstructslot(sqvm, 9);

	g_pSquirrel[context]->pushinteger(sqvm, static_cast<int>(mod.m_Source));
	g_pSquirrel[context]->sealstructslot(sqvm, 10);

	const ModTrackedPackage* trackedPackage = CModMenuSquirrel::FindManagedPackage(mod, inventory);
	const ModUpdateState updateState = trackedPackage ? trackedPackage->updateState : ModUpdateState::LegacyUnknown;
	g_pSquirrel[context]->pushinteger(sqvm, static_cast<int>(updateState));
	g_pSquirrel[context]->sealstructslot(sqvm, 11);

	const InstalledModRemovalInfo removalInfo = CModInstallService::Get().GetInstalledModRemovalInfo(static_cast<int>(modIndex));
	g_pSquirrel[context]->pushbool(sqvm, removalInfo.canDelete);
	g_pSquirrel[context]->sealstructslot(sqvm, 12);

	g_pSquirrel[context]->pushinteger(sqvm, removalInfo.deleteModCount);
	g_pSquirrel[context]->sealstructslot(sqvm, 13);

	g_pSquirrel[context]->pushinteger(sqvm, static_cast<int>(modIndex));
	g_pSquirrel[context]->sealstructslot(sqvm, 14);

	fs::path iconPath;
	const bool hasLocalIcon = CModMenuSquirrel::FindModIcon(mod, iconPath);
	g_pSquirrel[context]->pushbool(sqvm, hasLocalIcon || (trackedPackage && (!trackedPackage->remoteThumbnailUrl.empty() || trackedPackage->remoteThumbnail)));
	g_pSquirrel[context]->sealstructslot(sqvm, 15);

	g_pSquirrel[context]->newarray(sqvm);
	for (const std::string& asset : CModMenuSquirrel::CollectModAssets(mod))
	{
		g_pSquirrel[context]->pushstring(sqvm, asset.c_str());
		g_pSquirrel[context]->arrayappend(sqvm, -2);
	}
	g_pSquirrel[context]->sealstructslot(sqvm, 16);

	const std::u8string iconPathUtf8 = hasLocalIcon ? iconPath.u8string() : std::u8string();
	g_pSquirrel[context]->pushstring(sqvm, reinterpret_cast<const char*>(iconPathUtf8.c_str()));
	g_pSquirrel[context]->sealstructslot(sqvm, 17);
	std::string iconUrl = trackedPackage ? trackedPackage->remoteThumbnailUrl : std::string();
	std::string iconVersion = trackedPackage ? trackedPackage->remoteUpdatedAt : std::string();
	std::string iconFallbackUrl;
	if (trackedPackage && trackedPackage->remoteThumbnail)
	{
		const ModWorkshopThumbnail& thumbnail = *trackedPackage->remoteThumbnail;
		if (iconUrl.empty())
			iconUrl = CModWorkshopClient::BuildThumbnailUrl(thumbnail);
		iconVersion = thumbnail.updatedAt;
		if (thumbnail.hasThumbnail)
		{
			ModWorkshopThumbnail original = thumbnail;
			original.hasThumbnail = false;
			iconFallbackUrl = CModWorkshopClient::BuildThumbnailUrl(original);
		}
	}
	g_pSquirrel[context]->pushstring(sqvm, iconUrl.c_str());
	g_pSquirrel[context]->sealstructslot(sqvm, 18);
	g_pSquirrel[context]->pushstring(sqvm, iconVersion.c_str());
	g_pSquirrel[context]->sealstructslot(sqvm, 19);
	g_pSquirrel[context]->pushstring(sqvm, iconFallbackUrl.c_str());
	g_pSquirrel[context]->sealstructslot(sqvm, 20);

	// add current object to squirrel array
	g_pSquirrel[context]->arrayappend(sqvm, -2);
}

ADD_SQFUNC("void", NSDeleteRemoteMod, "string modName, string modVersion", "", ScriptContext::CLIENT | ScriptContext::UI)
{
	const SQChar* modName = g_pSquirrel[context]->getstring(sqvm, 1);
	const SQChar* modVersion = g_pSquirrel[context]->getstring(sqvm, 2);
	g_pModManager->DeleteRemoteMod(modName, modVersion);

	return SQRESULT_NULL;
}

ADD_SQFUNC("bool", NSRemoveMod, "int modIndex", "Queues safe removal of the indexed installed mod.", ScriptContext::UI)
{
	const int modIndex = static_cast<int>(g_pSquirrel[context]->getinteger(sqvm, 1));
	const bool queued = CModInstallService::Get().RequestInstalledModRemoval(modIndex);
	g_pSquirrel[context]->pushbool(sqvm, queued);
	return SQRESULT_NOTNULL;
}


ADD_SQFUNC("array<ModInfo>", NSGetModsInformation, "", "", ScriptContext::SERVER | ScriptContext::CLIENT | ScriptContext::UI)
{
	g_pSquirrel[context]->newarray(sqvm, 0);
	const std::shared_ptr<const ModInventorySnapshot> inventory = CModInventory::Get().GetSnapshot();

	for (size_t modIndex = 0; modIndex < g_pModManager->m_LoadedMods.size(); ++modIndex)
	{
		CModMenuSquirrel::PushMod<context>(sqvm, g_pModManager->m_LoadedMods[modIndex], inventory.get(), modIndex);
	}

	return SQRESULT_NOTNULL;
}

ADD_SQFUNC("array<ModInfo>", NSGetModInformation, "string modName", "", ScriptContext::SERVER | ScriptContext::CLIENT | ScriptContext::UI)
{
	const SQChar* modName = g_pSquirrel[context]->getstring(sqvm, 1);
	g_pSquirrel[context]->newarray(sqvm, 0);
	const std::shared_ptr<const ModInventorySnapshot> inventory = CModInventory::Get().GetSnapshot();

	for (size_t modIndex = 0; modIndex < g_pModManager->m_LoadedMods.size(); ++modIndex)
	{
		Mod& mod = g_pModManager->m_LoadedMods[modIndex];
		if (mod.Name.compare(modName) != 0)
			continue;
		CModMenuSquirrel::PushMod<context>(sqvm, mod, inventory.get(), modIndex);
	}

	return SQRESULT_NOTNULL;
}

ADD_SQFUNC("array<string>", NSGetModNames, "", "", ScriptContext::SERVER | ScriptContext::CLIENT | ScriptContext::UI)
{
	g_pSquirrel[context]->newarray(sqvm, 0);

	for (Mod& mod : g_pModManager->m_LoadedMods)
	{
		g_pSquirrel[context]->pushstring(sqvm, mod.Name.c_str());
		g_pSquirrel[context]->arrayappend(sqvm, -2);
	}

	return SQRESULT_NOTNULL;
}

ADD_SQFUNC(
	"void",
	NSSetModEnabled,
	"string modName, string modVersion, bool enabled",
	"",
	ScriptContext::SERVER | ScriptContext::CLIENT | ScriptContext::UI)
{
	const SQChar* modName = g_pSquirrel[context]->getstring(sqvm, 1);
	const SQChar* modVersion = g_pSquirrel[context]->getstring(sqvm, 2);
	const SQBool enabled = g_pSquirrel[context]->getbool(sqvm, 3);

	// manual lookup, not super performant but eh not a big deal
	for (Mod& mod : g_pModManager->m_LoadedMods)
	{
		if (!mod.Name.compare(modName) && !mod.Version.compare(modVersion))
		{
			mod.m_bEnabled = enabled;
			return SQRESULT_NULL;
		}
	}

	return SQRESULT_NULL;
}

ADD_SQFUNC("void", NSReloadMods, "", "", ScriptContext::UI)
{
	NOTE_UNUSED(sqvm);
	g_pModManager->ReloadMods();
	return SQRESULT_NULL;
}

class CModBrowserSquirrel final
{
  public:
    static int SquirrelInteger(uint64_t value);
    static std::string OperationId(const ModInstallOperationSnapshot* operation);
    static void EnsureCallbacks();

    template <ScriptContext context>
    static void PushCatalogEntry(HSQUIRRELVM sqvm, const ModBrowserEntry& entry, const ModInstallOperationSnapshot* operation,
                                 std::string_view operationId);
    template <ScriptContext context> static void PushPageSnapshot(HSQUIRRELVM sqvm);
    template <ScriptContext context> static void PushDetailsSnapshot(HSQUIRRELVM sqvm);
    template <ScriptContext context> static void PushOperationSnapshot(HSQUIRRELVM sqvm);
    template <ScriptContext context> static void PushInventorySnapshot(HSQUIRRELVM sqvm);
    template <ScriptContext context> static SQRESULT RequestOperation(HSQUIRRELVM sqvm, ModInstallAction action);

  private:
    static void OnPageChanged(uint64_t generation);
    static void OnDetailsChanged(const std::string& id);
    static void OnUpdatesChanged(uint64_t generation, int updateCount, ModInventoryUpdateStage stage);
    static void OnOperationChanged();

    inline static std::once_flag g_modBrowserCallbacksInitialized;
};

int CModBrowserSquirrel::SquirrelInteger(uint64_t value)
{
    return static_cast<int>(std::min<uint64_t>(value, std::numeric_limits<int>::max()));
}

std::string CModBrowserSquirrel::OperationId(const ModInstallOperationSnapshot* operation)
{
    if (!operation)
        return {};
    if (operation->source == ModSource::ModWorkshop)
        return operation->modId ? CModBrowserService::BuildId(operation->source, std::to_string(operation->modId)) : std::string();
    return CModBrowserService::BuildId(operation->source, operation->packageId);
}

void CModBrowserSquirrel::OnPageChanged(uint64_t generation)
{
    SquirrelManager* squirrel = g_pSquirrel[ScriptContext::UI];
    if (squirrel && squirrel->m_pSQVM)
        squirrel->AsyncCall("NSUICodeCallback_ModBrowserPageChanged", SquirrelInteger(generation));
}

void CModBrowserSquirrel::OnDetailsChanged(const std::string& id)
{
    SquirrelManager* squirrel = g_pSquirrel[ScriptContext::UI];
    if (squirrel && squirrel->m_pSQVM)
        squirrel->AsyncCall("NSUICodeCallback_ModBrowserDetailsChanged", id);
}


void CModBrowserSquirrel::OnUpdatesChanged(uint64_t generation, int updateCount, ModInventoryUpdateStage stage)
{
    SquirrelManager* squirrel = g_pSquirrel[ScriptContext::UI];
    if (squirrel && squirrel->m_pSQVM)
        squirrel->AsyncCall("NSUICodeCallback_ModBrowserUpdatesChanged", SquirrelInteger(generation), updateCount, static_cast<int>(stage));
}

void CModBrowserSquirrel::OnOperationChanged()
{
    SquirrelManager* squirrel = g_pSquirrel[ScriptContext::UI];
    if (squirrel && squirrel->m_pSQVM)
        squirrel->AsyncCall("NSUICodeCallback_ModBrowserOperationChanged");
}

void CModBrowserSquirrel::EnsureCallbacks()
{
    std::call_once(g_modBrowserCallbacksInitialized, []
    {
        CModBrowserService::Get().SetPageChangedCallback(OnPageChanged);
        CModBrowserService::Get().SetDetailsChangedCallback(OnDetailsChanged);
        CModBrowserService::Get().SetUpdatesChangedCallback(OnUpdatesChanged);
        CModInstallService::Get().SetOperationChangedCallback(OnOperationChanged);
    });
}

template <ScriptContext context>
void CModBrowserSquirrel::PushCatalogEntry(HSQUIRRELVM sqvm, const ModBrowserEntry& entry,
                                           const ModInstallOperationSnapshot* operation, std::string_view operationId)
{
    const bool operationMatches = operation && operationId == entry.id;
    g_pSquirrel[context]->pushnewstructinstance(sqvm, 20);
    g_pSquirrel[context]->pushstring(sqvm, entry.id.c_str());
    g_pSquirrel[context]->sealstructslot(sqvm, 0);
    g_pSquirrel[context]->pushstring(sqvm, entry.name.c_str());
    g_pSquirrel[context]->sealstructslot(sqvm, 1);
    g_pSquirrel[context]->pushstring(sqvm, entry.author.c_str());
    g_pSquirrel[context]->sealstructslot(sqvm, 2);
    const std::string& summary = entry.shortDescription.empty() ? entry.description : entry.shortDescription;
    g_pSquirrel[context]->pushstring(sqvm, summary.c_str());
    g_pSquirrel[context]->sealstructslot(sqvm, 3);
    g_pSquirrel[context]->pushstring(sqvm, entry.version.c_str());
    g_pSquirrel[context]->sealstructslot(sqvm, 4);
    g_pSquirrel[context]->pushinteger(sqvm, SquirrelInteger(entry.downloads));
    g_pSquirrel[context]->sealstructslot(sqvm, 5);
    g_pSquirrel[context]->pushinteger(sqvm, SquirrelInteger(entry.likes));
    g_pSquirrel[context]->sealstructslot(sqvm, 6);
    g_pSquirrel[context]->pushinteger(sqvm, SquirrelInteger(entry.views));
    g_pSquirrel[context]->sealstructslot(sqvm, 7);
    g_pSquirrel[context]->pushstring(sqvm, entry.thumbnailUrl.c_str());
    g_pSquirrel[context]->sealstructslot(sqvm, 8);
    g_pSquirrel[context]->pushbool(sqvm, entry.installed);
    g_pSquirrel[context]->sealstructslot(sqvm, 9);
    g_pSquirrel[context]->pushinteger(sqvm, entry.updateState);
    g_pSquirrel[context]->sealstructslot(sqvm, 10);
    g_pSquirrel[context]->pushinteger(sqvm, operationMatches ? static_cast<int>(operation->state) : static_cast<int>(ModInstallOperationState::Idle));
    g_pSquirrel[context]->sealstructslot(sqvm, 11);
    g_pSquirrel[context]->pushbool(sqvm, entry.approved);
    g_pSquirrel[context]->sealstructslot(sqvm, 12);
    g_pSquirrel[context]->pushbool(sqvm, entry.suspended);
    g_pSquirrel[context]->sealstructslot(sqvm, 13);
    g_pSquirrel[context]->pushstring(sqvm, entry.pageUrl.c_str());
    g_pSquirrel[context]->sealstructslot(sqvm, 14);
    g_pSquirrel[context]->pushbool(sqvm, entry.hasDownload && entry.approved && !entry.suspended && !entry.disableModManagers);
    g_pSquirrel[context]->sealstructslot(sqvm, 15);
    g_pSquirrel[context]->pushstring(sqvm, entry.selectedFileId.c_str());
    g_pSquirrel[context]->sealstructslot(sqvm, 16);
    g_pSquirrel[context]->pushinteger(sqvm, static_cast<int>(entry.source));
    g_pSquirrel[context]->sealstructslot(sqvm, 17);
    g_pSquirrel[context]->pushstring(sqvm, entry.thumbnailFallbackUrl.c_str());
    g_pSquirrel[context]->sealstructslot(sqvm, 18);
    g_pSquirrel[context]->pushstring(sqvm, entry.thumbnailVersion.c_str());
    g_pSquirrel[context]->sealstructslot(sqvm, 19);
}

template <ScriptContext context> void CModBrowserSquirrel::PushPageSnapshot(HSQUIRRELVM sqvm)
{
    const auto snapshot = CModBrowserService::Get().GetPageSnapshot();
    const auto operation = CModInstallService::Get().GetSnapshot();
    const std::string operationId = OperationId(operation.get());
    g_pSquirrel[context]->pushnewstructinstance(sqvm, 11);
    g_pSquirrel[context]->pushinteger(sqvm, snapshot ? static_cast<int>(snapshot->state) : static_cast<int>(ModBrowserLoadState::Idle));
    g_pSquirrel[context]->sealstructslot(sqvm, 0);
    g_pSquirrel[context]->pushinteger(sqvm, snapshot ? SquirrelInteger(snapshot->generation) : 0);
    g_pSquirrel[context]->sealstructslot(sqvm, 1);
    g_pSquirrel[context]->pushstring(sqvm, snapshot ? snapshot->search.c_str() : "");
    g_pSquirrel[context]->sealstructslot(sqvm, 2);
    g_pSquirrel[context]->pushstring(sqvm, snapshot ? snapshot->sort.c_str() : "");
    g_pSquirrel[context]->sealstructslot(sqvm, 3);
    g_pSquirrel[context]->pushinteger(sqvm, snapshot ? snapshot->requestedPage : 1);
    g_pSquirrel[context]->sealstructslot(sqvm, 4);
    g_pSquirrel[context]->pushinteger(sqvm, snapshot && snapshot->page ? snapshot->page->metadata.currentPage : 1);
    g_pSquirrel[context]->sealstructslot(sqvm, 5);
    g_pSquirrel[context]->pushinteger(sqvm, snapshot && snapshot->page ? snapshot->page->metadata.lastPage : 1);
    g_pSquirrel[context]->sealstructslot(sqvm, 6);
    g_pSquirrel[context]->pushinteger(sqvm, snapshot && snapshot->page ? snapshot->page->metadata.total : 0);
    g_pSquirrel[context]->sealstructslot(sqvm, 7);
    g_pSquirrel[context]->pushbool(sqvm, snapshot && snapshot->fromCache);
    g_pSquirrel[context]->sealstructslot(sqvm, 8);
    g_pSquirrel[context]->pushstring(sqvm, snapshot ? snapshot->error.message.c_str() : "");
    g_pSquirrel[context]->sealstructslot(sqvm, 9);
    g_pSquirrel[context]->newarray(sqvm, 0);
    if (snapshot && snapshot->page)
    {
        for (const ModBrowserEntry& entry : snapshot->page->entries)
        {
            PushCatalogEntry<context>(sqvm, entry, operation.get(), operationId);
            g_pSquirrel[context]->arrayappend(sqvm, -2);
        }
    }
    g_pSquirrel[context]->sealstructslot(sqvm, 10);
}

template <ScriptContext context> void CModBrowserSquirrel::PushDetailsSnapshot(HSQUIRRELVM sqvm)
{
    const auto snapshot = CModBrowserService::Get().GetDetailsSnapshot();
    const ModBrowserDetails* details = snapshot && snapshot->details ? snapshot->details.get() : nullptr;
    g_pSquirrel[context]->pushnewstructinstance(sqvm, 21);
    g_pSquirrel[context]->pushinteger(sqvm, snapshot ? static_cast<int>(snapshot->state) : static_cast<int>(ModBrowserLoadState::Idle));
    g_pSquirrel[context]->sealstructslot(sqvm, 0);
    g_pSquirrel[context]->pushinteger(sqvm, snapshot ? SquirrelInteger(snapshot->generation) : 0);
    g_pSquirrel[context]->sealstructslot(sqvm, 1);
    g_pSquirrel[context]->pushstring(sqvm, snapshot ? snapshot->id.c_str() : "");
    g_pSquirrel[context]->sealstructslot(sqvm, 2);
    g_pSquirrel[context]->pushstring(sqvm, details ? details->name.c_str() : "");
    g_pSquirrel[context]->sealstructslot(sqvm, 3);
    g_pSquirrel[context]->pushstring(sqvm, details ? details->author.c_str() : "");
    g_pSquirrel[context]->sealstructslot(sqvm, 4);
    g_pSquirrel[context]->pushstring(sqvm, details ? details->description.c_str() : "");
    g_pSquirrel[context]->sealstructslot(sqvm, 5);
    g_pSquirrel[context]->pushstring(sqvm, details ? details->version.c_str() : "");
    g_pSquirrel[context]->sealstructslot(sqvm, 6);
    g_pSquirrel[context]->pushstring(sqvm, details ? details->selectedFileId.c_str() : "");
    g_pSquirrel[context]->sealstructslot(sqvm, 7);
    g_pSquirrel[context]->pushstring(sqvm, details && details->selectedFileSize ? std::to_string(details->selectedFileSize).c_str() : "");
    g_pSquirrel[context]->sealstructslot(sqvm, 8);
    g_pSquirrel[context]->pushstring(sqvm, details ? details->selectedFileUpdatedAt.c_str() : "");
    g_pSquirrel[context]->sealstructslot(sqvm, 9);
    g_pSquirrel[context]->pushinteger(sqvm, details ? SquirrelInteger(details->downloads) : 0);
    g_pSquirrel[context]->sealstructslot(sqvm, 10);
    g_pSquirrel[context]->pushinteger(sqvm, details ? SquirrelInteger(details->likes) : 0);
    g_pSquirrel[context]->sealstructslot(sqvm, 11);
    g_pSquirrel[context]->pushinteger(sqvm, details ? SquirrelInteger(details->views) : 0);
    g_pSquirrel[context]->sealstructslot(sqvm, 12);
    g_pSquirrel[context]->pushbool(sqvm, details && details->installed);
    g_pSquirrel[context]->sealstructslot(sqvm, 13);
    g_pSquirrel[context]->pushinteger(sqvm, details ? details->updateState : static_cast<int>(ModUpdateState::LegacyUnknown));
    g_pSquirrel[context]->sealstructslot(sqvm, 14);
    g_pSquirrel[context]->pushbool(sqvm, details && details->hasDownload && details->approved && !details->suspended && !details->disableModManagers);
    g_pSquirrel[context]->sealstructslot(sqvm, 15);
    g_pSquirrel[context]->pushstring(sqvm, details ? details->pageUrl.c_str() : "");
    g_pSquirrel[context]->sealstructslot(sqvm, 16);
    g_pSquirrel[context]->newarray(sqvm, 0);
    if (details)
    {
        for (const std::string& dependency : details->dependencies)
        {
            g_pSquirrel[context]->pushstring(sqvm, dependency.c_str());
            g_pSquirrel[context]->arrayappend(sqvm, -2);
        }
    }
    g_pSquirrel[context]->sealstructslot(sqvm, 17);
    g_pSquirrel[context]->pushbool(sqvm, snapshot && snapshot->fromCache);
    g_pSquirrel[context]->sealstructslot(sqvm, 18);
    g_pSquirrel[context]->pushstring(sqvm, snapshot ? snapshot->error.message.c_str() : "");
    g_pSquirrel[context]->sealstructslot(sqvm, 19);
    g_pSquirrel[context]->pushinteger(sqvm, static_cast<int>(details ? details->source : ModSource::Unknown));
    g_pSquirrel[context]->sealstructslot(sqvm, 20);
}

template <ScriptContext context> void CModBrowserSquirrel::PushOperationSnapshot(HSQUIRRELVM sqvm)
{
    const auto snapshot = CModInstallService::Get().GetSnapshot();
    g_pSquirrel[context]->pushnewstructinstance(sqvm, 11);
    g_pSquirrel[context]->pushinteger(sqvm, snapshot ? SquirrelInteger(snapshot->generation) : 0);
    g_pSquirrel[context]->sealstructslot(sqvm, 0);
    g_pSquirrel[context]->pushstring(sqvm, OperationId(snapshot.get()).c_str());
    g_pSquirrel[context]->sealstructslot(sqvm, 1);
    g_pSquirrel[context]->pushinteger(sqvm, snapshot ? static_cast<int>(snapshot->action) : static_cast<int>(ModInstallAction::Install));
    g_pSquirrel[context]->sealstructslot(sqvm, 2);
    g_pSquirrel[context]->pushinteger(sqvm, snapshot ? static_cast<int>(snapshot->state) : static_cast<int>(ModInstallOperationState::Idle));
    g_pSquirrel[context]->sealstructslot(sqvm, 3);
    g_pSquirrel[context]->pushstring(sqvm, snapshot ? snapshot->name.c_str() : "");
    g_pSquirrel[context]->sealstructslot(sqvm, 4);
    g_pSquirrel[context]->pushstring(sqvm, snapshot ? snapshot->version.c_str() : "");
    g_pSquirrel[context]->sealstructslot(sqvm, 5);
    g_pSquirrel[context]->pushstring(sqvm, snapshot ? snapshot->message.c_str() : "");
    g_pSquirrel[context]->sealstructslot(sqvm, 6);
    g_pSquirrel[context]->pushinteger(sqvm, snapshot ? SquirrelInteger(snapshot->progress) : 0);
    g_pSquirrel[context]->sealstructslot(sqvm, 7);
    g_pSquirrel[context]->pushinteger(sqvm, snapshot ? SquirrelInteger(snapshot->total) : 0);
    g_pSquirrel[context]->sealstructslot(sqvm, 8);
    g_pSquirrel[context]->pushfloat(sqvm, snapshot ? snapshot->ratio : 0.0f);
    g_pSquirrel[context]->sealstructslot(sqvm, 9);
    g_pSquirrel[context]->pushbool(sqvm, snapshot && snapshot->cancellationDeferred);
    g_pSquirrel[context]->sealstructslot(sqvm, 10);
}

template <ScriptContext context> void CModBrowserSquirrel::PushInventorySnapshot(HSQUIRRELVM sqvm)
{
    const auto snapshot = CModBrowserService::Get().GetInventorySnapshot();
    g_pSquirrel[context]->pushnewstructinstance(sqvm, 6);
    g_pSquirrel[context]->pushinteger(sqvm, snapshot ? SquirrelInteger(snapshot->generation) : 0);
    g_pSquirrel[context]->sealstructslot(sqvm, 0);
    g_pSquirrel[context]->pushinteger(sqvm, snapshot ? snapshot->updateCount : 0);
    g_pSquirrel[context]->sealstructslot(sqvm, 1);
    g_pSquirrel[context]->pushbool(sqvm, snapshot && snapshot->checking);
    g_pSquirrel[context]->sealstructslot(sqvm, 2);
    g_pSquirrel[context]->pushstring(sqvm, snapshot ? snapshot->checkedAt.c_str() : "");
    g_pSquirrel[context]->sealstructslot(sqvm, 3);
    g_pSquirrel[context]->pushstring(sqvm, snapshot ? snapshot->error.c_str() : "");
    g_pSquirrel[context]->sealstructslot(sqvm, 4);
    g_pSquirrel[context]->pushinteger(sqvm, snapshot ? SquirrelInteger(snapshot->packages.size()) : 0);
    g_pSquirrel[context]->sealstructslot(sqvm, 5);
}

template <ScriptContext context> SQRESULT CModBrowserSquirrel::RequestOperation(HSQUIRRELVM sqvm, ModInstallAction action)
{
    EnsureCallbacks();
    const SQChar* id = g_pSquirrel[context]->getstring(sqvm, 1);
    g_pSquirrel[context]->pushbool(sqvm, CModBrowserService::Get().RequestOperation(action, id ? id : ""));
    return SQRESULT_NOTNULL;
}

ADD_SQFUNC("int", NSModBrowserRequestPage, "string search, string sort, int page, int filter, bool forceRefresh, int source",
           "Requests a cancellable mod catalog page.", ScriptContext::UI)
{
    CModBrowserSquirrel::EnsureCallbacks();
    const SQChar* search = g_pSquirrel[context]->getstring(sqvm, 1);
    const SQChar* sort = g_pSquirrel[context]->getstring(sqvm, 2);
    const int page = static_cast<int>(g_pSquirrel[context]->getinteger(sqvm, 3));
    const int filter = static_cast<int>(g_pSquirrel[context]->getinteger(sqvm, 4));
    const bool refresh = g_pSquirrel[context]->getbool(sqvm, 5);
    const auto source = static_cast<ModSource>(g_pSquirrel[context]->getinteger(sqvm, 6));
    const uint64_t generation = CModBrowserService::Get().RequestPage(search ? search : "", sort ? sort : "bumped_at", page, filter, source, refresh);
    g_pSquirrel[context]->pushinteger(sqvm, CModBrowserSquirrel::SquirrelInteger(generation));
    return SQRESULT_NOTNULL;
}

ADD_SQFUNC("ModBrowserPageSnapshot", NSModBrowserGetPage, "", "Returns the current catalog snapshot.", ScriptContext::UI)
{
    CModBrowserSquirrel::EnsureCallbacks();
    CModBrowserSquirrel::PushPageSnapshot<context>(sqvm);
    return SQRESULT_NOTNULL;
}

ADD_SQFUNC("void", NSModBrowserCancelPage, "", "Cancels the active catalog request.", ScriptContext::UI)
{
    NOTE_UNUSED(sqvm);
    CModBrowserService::Get().CancelPageRequest();
    return SQRESULT_NULL;
}

ADD_SQFUNC("int", NSModBrowserRequestDetails, "string modId, bool forceRefresh", "Requests details for a source-qualified mod ID.", ScriptContext::UI)
{
    CModBrowserSquirrel::EnsureCallbacks();
    const SQChar* id = g_pSquirrel[context]->getstring(sqvm, 1);
    const uint64_t generation = CModBrowserService::Get().RequestDetails(id ? id : "", g_pSquirrel[context]->getbool(sqvm, 2));
    g_pSquirrel[context]->pushinteger(sqvm, CModBrowserSquirrel::SquirrelInteger(generation));
    return SQRESULT_NOTNULL;
}

ADD_SQFUNC("ModBrowserDetailsSnapshot", NSModBrowserGetDetails, "", "Returns the current details snapshot.", ScriptContext::UI)
{
    CModBrowserSquirrel::EnsureCallbacks();
    CModBrowserSquirrel::PushDetailsSnapshot<context>(sqvm);
    return SQRESULT_NOTNULL;
}

ADD_SQFUNC("void", NSModBrowserCancelDetails, "", "Cancels the active details request.", ScriptContext::UI)
{
    NOTE_UNUSED(sqvm);
    CModBrowserService::Get().CancelDetailsRequest();
    return SQRESULT_NULL;
}

ADD_SQFUNC("bool", NSModBrowserInstall, "string modId", "Installs a catalog mod.", ScriptContext::UI)
{
    return CModBrowserSquirrel::RequestOperation<context>(sqvm, ModInstallAction::Install);
}

ADD_SQFUNC("bool", NSModBrowserUpdate, "string modId", "Updates a managed mod.", ScriptContext::UI)
{
    return CModBrowserSquirrel::RequestOperation<context>(sqvm, ModInstallAction::Update);
}

ADD_SQFUNC("bool", NSModBrowserRemove, "string modId", "Removes a managed mod.", ScriptContext::UI)
{
    return CModBrowserSquirrel::RequestOperation<context>(sqvm, ModInstallAction::Remove);
}

ADD_SQFUNC("ModBrowserOperationSnapshot", NSModBrowserGetOperationState, "", "Returns the synchronized install operation snapshot.",
           ScriptContext::UI)
{
    CModBrowserSquirrel::EnsureCallbacks();
    CModBrowserSquirrel::PushOperationSnapshot<context>(sqvm);
    return SQRESULT_NOTNULL;
}

ADD_SQFUNC("bool", NSModBrowserDecideMigration, "int generation, bool accept", "Accepts or declines replacement of an existing package.",
           ScriptContext::UI)
{
    const uint64_t generation = static_cast<uint64_t>(std::max<SQInteger>(0, g_pSquirrel[context]->getinteger(sqvm, 1)));
    g_pSquirrel[context]->pushbool(sqvm, CModInstallService::Get().DecideMigration(generation, g_pSquirrel[context]->getbool(sqvm, 2)));
    return SQRESULT_NOTNULL;
}

ADD_SQFUNC("void", NSModBrowserCancelOperation, "", "Cancels the active install operation.", ScriptContext::UI)
{
    NOTE_UNUSED(sqvm);
    CModInstallService::Get().Cancel();
    return SQRESULT_NULL;
}

ADD_SQFUNC("int", NSModBrowserRefreshTrackedMods, "bool checkRemote", "Refreshes local packages and optionally checks for updates.",
           ScriptContext::UI)
{
    CModBrowserSquirrel::EnsureCallbacks();
    const uint64_t generation = CModBrowserService::Get().RefreshTrackedMods(g_pSquirrel[context]->getbool(sqvm, 1));
    g_pSquirrel[context]->pushinteger(sqvm, CModBrowserSquirrel::SquirrelInteger(generation));
    return SQRESULT_NOTNULL;
}

ADD_SQFUNC("ModBrowserInventorySnapshot", NSModBrowserGetInventoryState, "", "Returns the managed-package inventory snapshot.", ScriptContext::UI)
{
    CModBrowserSquirrel::EnsureCallbacks();
    CModBrowserSquirrel::PushInventorySnapshot<context>(sqvm);
    return SQRESULT_NOTNULL;
}

ADD_SQFUNC("bool", NSModBrowserOpenPage, "string modId", "Opens a catalog mod's provider page.", ScriptContext::UI)
{
    const SQChar* id = g_pSquirrel[context]->getstring(sqvm, 1);
    const std::string url = CModBrowserService::BuildPageUrl(id ? id : "");
    const bool opened = !url.empty() && reinterpret_cast<intptr_t>(ShellExecuteA(nullptr, "open", url.c_str(), nullptr, nullptr, SW_SHOWNORMAL)) > 32;
    g_pSquirrel[context]->pushbool(sqvm, opened);
    return SQRESULT_NOTNULL;
}

