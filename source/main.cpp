#include <GarrysMod/Lua/Interface.h>
#include <GarrysMod/FactoryLoader.hpp>
#include "util.h"
#include "GameEventListener.h"
#include "datacache/imdlcache.h"
#include "datacache/idatacache.h"
#include "vstdlib/jobthread.h"

#define Cache_Debug 0;
#define Cache_UseNotify 0; // Set this to 1 if you want to use a Cache instead of clearing the whole cache each time. This can cause the lightning on some models to break!
#define Cache_AwaysFlush 0; // Enable this if you want to clear the cache every time you disconnect from a server. This can cause crashes!
#define Cache_Experimental 0; // Enable this to enable features that are in development like clearing the Materials too. (This is Experimental)

#if Cache_UseNotify == 1;
	#include <unordered_map>
#endif 

#if Cache_Experimental == 1;
	#include <unordered_map>
	#include "materialsystem/imaterialsystem.h"
#endif

static SourceSDK::FactoryLoader engine_loader("engine");
static SourceSDK::FactoryLoader datacache_loader("datacache");
#if Cache_Experimental == 1
	static SourceSDK::FactoryLoader materialsystem_loader("materialsystem");
#endif

static IGameEventManager2* eventmanager = nullptr;
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

	struct ThreadParams_t
	{
		#if Cache_Experimental == 1
				std::unordered_map<const char*, bool> material_cache;
				IMaterialSystem* MaterialSystem;
		#endif
		std::unordered_map<MDLHandle_t, Cache_Entry*> cache;
		int delay;
		bool fullflush;
		IDataCache* DataCache;
		IMDLCache* MDLCache;
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
#else
	struct ThreadParams_t
	{
		#if Cache_Experimental == 1
			std::unordered_map<const char*, bool> material_cache;
			IMaterialSystem* MaterialSystem;
		#endif
		int delay;
		bool fullflush;
		IDataCache* DataCache;
		IMDLCache* MDLCache;
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
	} else {
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
		while (loop){
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

static bool connected = false;
class DisconnectEventListener : public IGameEventListener2
{
public:
	DisconnectEventListener() = default;

	void FireGameEvent(IGameEvent* event)
	{
		#if Cache_AwaysFlush == 1
			if (connected)
			{
				ThreadParams_t* vars = new ThreadParams_t;
				vars->delay = 1000;
				#if Cache_UseNotify == 1
					vars->cache = cache;
				#endif
				vars->fullflush = true;
				vars->MDLCache = MDLCache;
				vars->DataCache = DataCache;
				#if Cache_Experimental == 1
					vars->material_cache = material_cache;
					vars->MaterialSystem = MaterialSystem;
				#endif
				CreateSimpleThread(ClearCache, vars);
			}
		#endif

		connected = false;
	}
};

#if Cache_AwaysFlush == 0
	static const char* last_map = "";
	static const char* last_server = "";
#endif
// We need this listener because client_disconnect is called while joining a server, and we don't want to clear the cache when joining the server.
class ActivateEventListener : public IGameEventListener2
{
public:
	ActivateEventListener() = default;

	void FireGameEvent(IGameEvent* event)
	{
		#if Cache_AwaysFlush == 0 // We don't need to clear the Cache because it got already cleared when the client disconnected from the server if Cache_AwaysFlush is enabled.
			const char* address = event->GetString("address");
			const char* map = event->GetString("mapname");
			if (strcmp(last_server, "") == 1 && strcmp(last_map, map) == 1) {
				ThreadParams_t* vars = new ThreadParams_t;
				vars->delay = 0;
				#if Cache_UseNotify == 1
					vars->cache = cache;
				#endif
				vars->fullflush = false;
				vars->MDLCache = MDLCache;
				vars->DataCache = DataCache;
				#if Cache_Experimental == 1
					vars->material_cache = material_cache;
					vars->MaterialSystem = MaterialSystem;
				#endif
				ClearCache(vars);
			} 
		
			last_server = strdup(address);
			last_map = strdup(map);
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

		connected = true;
	}
};

static ActivateEventListener* ActivateListener = new ActivateEventListener;
static DisconnectEventListener* DisconnectListener = new DisconnectEventListener;
void AddAsyncCacheDataType(MDLCacheDataType_t type, char* type_str) {
	MDLCache->SetAsyncLoad(type, true);

	if (!MDLCache->GetAsyncLoad(type)) {
		std::string msg = "Failed to enable AsyncLoad for type ";
		msg.append(type_str);
		msg.append("\n");
		ConDMsg(msg.c_str());
	}
}

#if Cache_Debug == 1
void CheckAsyncCacheDataType(MDLCacheDataType_t type, char* type_str) {
	if (!MDLCache->GetAsyncLoad(type)) {
		std::string msg = "Failed to enable AsyncLoad for type ";
		msg.append(type_str);
		msg.append("\n");
		ConDMsg(msg.c_str());
	}
}

LUA_FUNCTION_STATIC(check_async) {
	CheckAsyncCacheDataType(MDLCACHE_STUDIOHDR, (char*)"MDLCACHE_STUDIOHDR");
	CheckAsyncCacheDataType(MDLCACHE_STUDIOHWDATA, (char*)"MDLCACHE_STUDIOHWDATA");
	CheckAsyncCacheDataType(MDLCACHE_VCOLLIDE, (char*)"MDLCACHE_VCOLLIDE");
	CheckAsyncCacheDataType(MDLCACHE_ANIMBLOCK, (char*)"MDLCACHE_ANIMBLOCK");
	CheckAsyncCacheDataType(MDLCACHE_VIRTUALMODEL, (char*)"MDLCACHE_VIRTUALMODEL");
	CheckAsyncCacheDataType(MDLCACHE_VERTEXES, (char*)"MDLCACHE_VERTEXES");
	CheckAsyncCacheDataType(MDLCACHE_DECODEDANIMBLOCK, (char*)"MDLCACHE_DECODEDANIMBLOCK");

	return 1;
}
#endif

LUA_FUNCTION_STATIC(cache_flush) {

	if (!connected) {
		ThreadParams_t* vars = new ThreadParams_t;
		vars->delay = 1000;
		#if Cache_UseNotify == 1
				vars->cache = cache;
		#endif
		vars->fullflush = true;
		vars->MDLCache = MDLCache;
		vars->DataCache = DataCache;
		#if Cache_Experimental == 1
			vars->material_cache = material_cache;
			vars->MaterialSystem = MaterialSystem;
		#endif
		CreateSimpleThread(ClearCache, vars);

		LuaPrint("[Cache] Flush begins in " + std::to_string(vars->delay / 1000) + " second!");
	}
	else {
		LuaPrint("[Cache] You cannot flush the Cache if you are connected to a Server!");
	}

	return 1;
}

GMOD_MODULE_OPEN()
{
	GlobalLUA = LUA;
	eventmanager = (IGameEventManager2*)engine_loader.GetFactory()(INTERFACEVERSION_GAMEEVENTSMANAGER2, nullptr);
	if (eventmanager == nullptr)
		LUA->ThrowError("unable to initialize IGameEventManager2");

	MDLCache = (IMDLCache*)datacache_loader.GetFactory()(MDLCACHE_INTERFACE_VERSION, nullptr);
	if (MDLCache == nullptr)
		LUA->ThrowError("unable to initialize IMDLCache");

	DataCache = (IDataCache*)datacache_loader.GetFactory()(DATACACHE_INTERFACE_VERSION, nullptr);
	if (DataCache == nullptr)
		LUA->ThrowError("unable to initialize IDataCache");

	#if Cache_Experimental == 1
		MaterialSystem = (IMaterialSystem*)materialsystem_loader.GetFactory()(MATERIAL_SYSTEM_INTERFACE_VERSION, nullptr);
		if (MaterialSystem == nullptr)
			LUA->ThrowError("unable to initialize IMaterialSystem");
	#endif

	#if Cache_UseNotify == 1
		MDLCache->SetCacheNotify(cacheupdate);
	#endif

	eventmanager->AddListener(DisconnectListener, "client_disconnect", false);
	eventmanager->AddListener(ActivateListener, "server_spawn", false);

	LuaPrint((char*)"[Cache] Successfully Loaded.");
	
	// AddAsyncCacheDataType(MDLCACHE_STUDIOHDR, (char*)"MDLCACHE_STUDIOHDR"); cannot be activated
	AddAsyncCacheDataType(MDLCACHE_STUDIOHWDATA, (char*)"MDLCACHE_STUDIOHWDATA");
	AddAsyncCacheDataType(MDLCACHE_VCOLLIDE, (char*)"MDLCACHE_VCOLLIDE");
	// AddAsyncCacheDataType(MDLCACHE_ANIMBLOCK, (char*)"MDLCACHE_ANIMBLOCK"); if activated, it breaks some playermodels
	// AddAsyncCacheDataType(MDLCACHE_VIRTUALMODEL, (char*)"MDLCACHE_VIRTUALMODEL"); cannot be activated
	AddAsyncCacheDataType(MDLCACHE_VERTEXES, (char*)"MDLCACHE_VERTEXES");
	// AddAsyncCacheDataType(MDLCACHE_DECODEDANIMBLOCK, (char*)"MDLCACHE_DECODEDANIMBLOCK"); cannot be activated

	#if Cache_Debug == 1
		LUA->GetField(-1, "engine");
			LUA->PushCFunction(check_async);
			LUA->SetField(-2, "check_async");
		LUA->Pop(1);
	#endif

	LUA->GetField(-1, "concommand");
		LUA->GetField(-1, "Add");
		LUA->PushString("cache_flush");
		LUA->PushCFunction(cache_flush);
		LUA->Call(2, 0);
	LUA->Pop();

	return 0;
}

GMOD_MODULE_CLOSE()
{
	return 0;
}