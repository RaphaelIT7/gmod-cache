#include <GarrysMod/Lua/Interface.h>
#include <GarrysMod/FactoryLoader.hpp>
#include "util.h"
#include "GameEventListener.h"
#include "datacache/imdlcache.h"
#include "vstdlib/jobthread.h"

#define Cache_Debug 0

static SourceSDK::FactoryLoader engine_loader("engine");
static SourceSDK::FactoryLoader datacache_loader("datacache");

static IGameEventManager2* eventmanager = nullptr;
static IMDLCache* MDLCache = nullptr;

static bool connected = false;
unsigned ClearCache(void *params) {
	ThreadSleep(1000);	// We need to wait for the client to shutdown. Because it uses the MDLCache and if we try to use it, it crashes.

	CThreadMutex().Lock();

	MDLCache->Flush(MDLCACHE_FLUSH_ALL);

	CThreadMutex().Unlock();

	return 0;
}

class DisconnectEventListener : public IGameEventListener2
{
public:
	DisconnectEventListener() = default;

	void FireGameEvent(IGameEvent* event)
	{
		if (connected) 
		{
			CreateSimpleThread(ClearCache, nullptr);
			connected = false;
		}
	}
};

// We need this listener because client_disconnect is called while joining a server, and we don't want to clear the cache when joining the server.
class ActivateEventListener : public IGameEventListener2
{
public:
	ActivateEventListener() = default;

	void FireGameEvent(IGameEvent* event)
	{
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

GMOD_MODULE_OPEN()
{
	GlobalLUA = LUA;
	eventmanager = (IGameEventManager2*)engine_loader.GetFactory()(INTERFACEVERSION_GAMEEVENTSMANAGER2, nullptr);
	if (eventmanager == nullptr)
		LUA->ThrowError("unable to initialize IGameEventManager2");

	MDLCache = (IMDLCache*)datacache_loader.GetFactory()(MDLCACHE_INTERFACE_VERSION, nullptr);
	if (MDLCache == nullptr)
		GlobalLUA->ThrowError("unable to initialize IMDLCache");

	eventmanager->AddListener(DisconnectListener, "client_disconnect", false);
	eventmanager->AddListener(ActivateListener, "player_activate", false);

	LuaPrint((char*)"[ClearCache] Successfully Loaded.");
	
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

	return 0;
}

GMOD_MODULE_CLOSE()
{
	return 0;
}