#include <GarrysMod/FactoryLoader.hpp>
#include "GameEventListener.h"
#include "datacache/imdlcache.h"
#include "datacache/idatacache.h"
#include "vstdlib/jobthread.h"
#include "cache.h"
#include "util.h"

#if Cache_UseNotify == 1
#include <unordered_map>
#endif 

#if Cache_Experimental == 1
#include <unordered_map>
#include "materialsystem/imaterialsystem.h"
#endif

static SourceSDK::FactoryLoader datacache_loader("datacache");
#if Cache_Experimental == 1
static SourceSDK::FactoryLoader materialsystem_loader("materialsystem");
#endif

static IMDLCache* MDLCache = nullptr;
static IDataCache* DataCache = nullptr;
#if Cache_Experimental == 1
static IMaterialSystem* MaterialSystem = nullptr;
static std::unordered_map<const char*, bool> material_cache;
#endif

#if Cache_UseNotify == 1
static MDlCacheUpdate* cacheupdate = new MDlCacheUpdate;

class Cache_Entry
{
public:
	MDLHandle_t handle;
	MDLCacheDataType_t type;

	Cache_Entry(MDLHandle_t handle, MDLCacheDataType_t type)
	{
		this->handle = handle;
		this->type = type;
	};

	void Unload(IMDLCache* MDLCache) {
		int invalid = MDLCache->GetRef(this->handle);
		if (invalid == 0) {
			return;
		}
		//Msg(std::to_string(MDLCache->GetRef(this->handle)).c_str());
		MDLCache->Flush(this->handle, MDLCACHE_FLUSH_ALL);
	}
};

static std::unordered_map<MDLHandle_t, Cache_Entry*> cache;
class MDlCacheUpdate : public IMDLCacheNotify
{
	void OnDataLoaded(MDLCacheDataType_t type, MDLHandle_t handle)
	{
		Cache_Entry* entry = new Cache_Entry(handle, type);
		cache[handle] = entry;
	}

	void OnDataUnloaded(MDLCacheDataType_t type, MDLHandle_t handle)
	{
		//Msg(std::to_string(MDLCache->GetRef(handle)).c_str());
		//Msg(MDLCache->GetModelName(handle)); // Just some testing. It seems like GetModelName returns garbage when the data is already unloaded? Returned Data: ????=???7????T??=?
		cache.erase(handle);
	}
};
#endif

unsigned ClearCache(void* params) {
	ThreadParams_t* vars = (ThreadParams_t*)params;
	if (vars->delay > 0) {
		ThreadSleep(vars->delay);
	}

	Msg("[Cache] Starting to clear the Cache\n");
	CThreadMutex().Lock();

#if Cache_UseNotify == 1
#if Cache_Debug == 1
	Msg("----Cache----\n");
	for (auto& [mdl, entry] : vars->cache) {
		Msg(MDLCache->GetModelName(mdl));
		Msg("\n");
	}
#endif

	int i = 0;
	for (auto& [mdl, entry] : vars->cache) {
		i++;
		entry->Unload(vars->MDLCache);
		cache.erase(entry->handle);
	}
#else
	if (vars->fullflush) {
		vars->MDLCache->Flush(MDLCACHE_FLUSH_ALL);
	}
	else {
		//vars->MDLCache->Flush(MDLCACHE_FLUSH_STUDIOHDR); // Crashes the DataCache
		vars->MDLCache->Flush(MDLCACHE_FLUSH_STUDIOHWDATA);
		//vars->MDLCache->Flush(MDLCACHE_FLUSH_VCOLLIDE); // If this is enabled, it will crash vphysics.
		vars->MDLCache->Flush(MDLCACHE_FLUSH_ANIMBLOCK);
		vars->MDLCache->Flush(MDLCACHE_FLUSH_VIRTUALMODEL);
		vars->MDLCache->Flush(MDLCACHE_FLUSH_AUTOPLAY);
		vars->MDLCache->Flush(MDLCACHE_FLUSH_VERTEXES);
		vars->MDLCache->Flush(MDLCACHE_FLUSH_IGNORELOCK);
	}
#endif

#if Cache_Experimental == 1
	bool loop = true;
	MaterialHandle_t last = MaterialSystem->FirstMaterial();
#if Cache_Debug == 1
	Msg("----Materials----\n");
#endif
	while (loop) {
		IMaterial* mat = MaterialSystem->GetMaterial(last);
		const char* name = mat->GetName();
		if (!vars->material_cache[name]) {
#if Cache_Debug == 1
			Msg(mat->GetName());
			Msg("\n");
#endif
			mat->Release();
			mat->DeleteIfUnreferenced();
		}

		last = MaterialSystem->NextMaterial(last);
		if (last == MaterialSystem->InvalidMaterial()) {
			Msg("Stopping");
			loop = false;
		}
	}
#endif

	if (vars->fullflush) {
#if Cache_Debug == 1
		Msg("----DataCache----\n");
		Msg("Cleared: ");
		Msg(std::to_string(vars->DataCache->Flush(true) / 1024 / 1024).c_str());
		Msg("MB\n-----------------\n");
#else
		vars->DataCache->Flush(true);
#endif
	}

	CThreadMutex().Unlock();
	Msg("[Cache] Cleared the Cache\n");

	return 0;
}

void CacheMgr::Connect(IGameEvent* event)
{
#if Cache_AwaysFlush == 0 // We don't need to clear the Cache because it got already cleared when the client disconnected from the server if Cache_AwaysFlush is enabled.
	const char* new_address = event->GetString("address");
	const char* new_map = event->GetString("mapname");
	if (strcmp(this->last_server, "") == 1 && strcmp(this->last_map, new_map) == 1) {
		if (strcmp(this->address, new_address) == 0) {
			if (this->clear_on_connect) {
				this->Flush(0, false, false, true);
			}
		} else {
			this->Flush(0, false, false, true);
		}
	}

	this->last_server = strdup(new_address);
	this->last_map = strdup(new_map);
#endif

#if Cache_Experimental == 1
	// This will create a list of all Materials that exist before connecting to a server.
	bool loop = true;
	MaterialHandle_t last = MaterialSystem->FirstMaterial();
	while (loop) {
		IMaterial* mat = MaterialSystem->GetMaterial(last);
		material_cache[mat->GetName()] = true;
		Msg(mat->GetName());
		Msg("\n");
		last = MaterialSystem->NextMaterial(last);
		if (last == MaterialSystem->InvalidMaterial()) {
			loop = false;
		}
	}
#endif

	this->connected = true;
};

void CacheMgr::Disconnect() {
	if (this->alwaysflush) {
		this->Flush(1000, true, true, true);
	}
		
	this->connected = false;
};

bool CacheMgr::SetAsyncCacheDataType(MDLCacheDataType_t type, bool enabled) {
	return this->SetAsyncCacheDataType(type, enabled, false, "");
}

bool CacheMgr::SetAsyncCacheDataType(MDLCacheDataType_t type, bool enabled, bool print, char* type_str) {
	MDLCache->SetAsyncLoad(type, enabled);

	bool worked = MDLCache->GetAsyncLoad(type);
	if (print && !worked) {
		std::string msg = "Failed to enable AsyncLoad for type ";
		msg.append(type_str);
		msg.append("\n");
		ConDMsg(msg.c_str());
	}

	return worked;
}

bool CacheMgr::GetAsyncCacheDataType(MDLCacheDataType_t type) {
	return this->GetAsyncCacheDataType(type, false, "");
}

bool CacheMgr::GetAsyncCacheDataType(MDLCacheDataType_t type, bool print, char* type_str) {
	bool async = MDLCache->GetAsyncLoad(type);
	if (print && !async) {
		std::string msg = "Failed to enable AsyncLoad for type ";
		msg.append(type_str);
		msg.append("\n");
		ConDMsg(msg.c_str());
	}

	return async;
}

bool CacheMgr::Flush(int delay, bool full, bool threaded, bool force)
{
	if (force || !this->connected) {
		ThreadParams_t* vars = new ThreadParams_t;
		vars->delay = delay;
		#if Cache_UseNotify == 1
			vars->cache = cache;
		#endif
		vars->fullflush = full;
		vars->MDLCache = MDLCache;
		vars->DataCache = DataCache;
		#if Cache_Experimental == 1
			vars->material_cache = material_cache;
			vars->MaterialSystem = MaterialSystem;
		#endif

		if (threaded) {
			CreateSimpleThread((ThreadFunc_t)ClearCache, vars);
		} else {
			ClearCache(vars);
		}

		return true;
	} else {
		return false;
	}
}

void CacheMgr::SetClearOnConnect(const char* address, bool clear)
{
	this->address = address;
	this->clear_on_connect = clear;
}

CacheMgr::CacheMgr()
{
	MDLCache = (IMDLCache*)datacache_loader.GetFactory()(MDLCACHE_INTERFACE_VERSION, nullptr);
	if (MDLCache == nullptr)
		ThrowError("unable to initialize IMDLCache");

	DataCache = (IDataCache*)datacache_loader.GetFactory()(DATACACHE_INTERFACE_VERSION, nullptr);
	if (DataCache == nullptr)
		ThrowError("unable to initialize IDataCache");

	#if Cache_Experimental == 1
		MaterialSystem = (IMaterialSystem*)materialsystem_loader.GetFactory()(MATERIAL_SYSTEM_INTERFACE_VERSION, nullptr);
		if (MaterialSystem == nullptr)
			ThrowError("unable to initialize IMaterialSystem");
	#endif

	#if Cache_UseNotify == 1
		MDLCache->SetCacheNotify(cacheupdate);
	#endif

	// this->SetAsyncCacheDataType(MDLCACHE_STUDIOHDR, true, false,  (char*)"MDLCACHE_STUDIOHDR"); cannot be activated
	this->SetAsyncCacheDataType(MDLCACHE_STUDIOHWDATA, true, false, (char*)"MDLCACHE_STUDIOHWDATA");
	this->SetAsyncCacheDataType(MDLCACHE_VCOLLIDE, true, false, (char*)"MDLCACHE_VCOLLIDE");
	// this->SetAsyncCacheDataType(MDLCACHE_ANIMBLOCK, true, false,  (char*)"MDLCACHE_ANIMBLOCK"); if activated, it breaks some playermodels
	// this->SetAsyncCacheDataType(MDLCACHE_VIRTUALMODEL, true, false,  (char*)"MDLCACHE_VIRTUALMODEL"); cannot be activated
	this->SetAsyncCacheDataType(MDLCACHE_VERTEXES, true, false, (char*)"MDLCACHE_VERTEXES");
	// this->SetAsyncCacheDataType(MDLCACHE_DECODEDANIMBLOCK, true, false,  (char*)"MDLCACHE_DECODEDANIMBLOCK"); cannot be activated
}