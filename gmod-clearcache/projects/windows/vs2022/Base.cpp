#include <GarrysMod/Lua/Interface.h>
#include <GarrysMod/FactoryLoader.hpp>
#include "util.h"
#include "GameEventListener.h"
#include "datacache/imdlcache.h"
#include "vstdlib/jobthread.h";


static SourceSDK::FactoryLoader engine_loader("engine");
static SourceSDK::FactoryLoader datacache_loader("datacache");

static IGameEventManager2* gameeventmanager = nullptr;
static IMDLCache* MDLCache = nullptr;

static bool connected = false;
unsigned ClearCache(void *params) {
	ThreadSleep(1000); // We need to wait for the client to shutdown. Because it uses the MDLCache and if we try to use it, it crashes.
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

GMOD_MODULE_OPEN()
{
	GlobalLUA = LUA;
	gameeventmanager = (IGameEventManager2*)engine_loader.GetFactory()(INTERFACEVERSION_GAMEEVENTSMANAGER2, nullptr);
	if (gameeventmanager == nullptr)
		LUA->ThrowError("unable to initialize IGameEventManager2");

	MDLCache = (IMDLCache*)datacache_loader.GetFactory()(MDLCACHE_INTERFACE_VERSION, nullptr);
	if (MDLCache == nullptr)
		GlobalLUA->ThrowError("unable to initialize IMDLCache");

	gameeventmanager->AddListener(DisconnectListener, "client_disconnect", false);
	gameeventmanager->AddListener(ActivateListener, "player_activate", false);

	LuaPrint("[ClearCache] Successfully Loaded.");

	return 0;
}

GMOD_MODULE_CLOSE()
{
	return 0;
}

// lua_run_menu PrintTable(engine.GetCachedModels())