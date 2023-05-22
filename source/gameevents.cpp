#include "gameevents.h"
#include "util.h"
#include "lua.h"

// We need this listener because client_disconnect is called while joining a server, and we don't want to clear the cache when joining the server.
class ActivateEventListener : public IGameEventListener2
{
public:
	ActivateEventListener() = default;

	void FireGameEvent(IGameEvent* event)
	{
		Cache->Connect(event);
	}
};

class DisconnectEventListener : public IGameEventListener2
{
public:
	DisconnectEventListener() = default;

	void FireGameEvent(IGameEvent* event)
	{
		Cache->Disconnect();
		LUA_UnLoadClient();
	}
};

// This should be called, when the clientstate(LUA) is initialized.
class InGameEventListener : public IGameEventListener2
{
public:
	InGameEventListener() = default;

	void FireGameEvent(IGameEvent* event) {
		LUA_InitClient(GetRealm(Realm_Client));
	}
};

static IGameEventManager2* eventmanager = nullptr;
void Gameevents_AddListener(IGameEventListener2* listener, const char* event, bool server) {
	eventmanager->AddListener(listener, event, server);
}

void Gameevents_Init()
{
	SourceSDK::FactoryLoader engine_loader("engine");
	eventmanager = (IGameEventManager2*)engine_loader.GetFactory()(INTERFACEVERSION_GAMEEVENTSMANAGER2, nullptr);
	if (eventmanager == nullptr)
		ThrowError("unable to initialize IGameEventManager2");

	Gameevents_AddListener(new ActivateEventListener, "server_spawn", false);
	Gameevents_AddListener(new DisconnectEventListener, "client_disconnect", false);
	Gameevents_AddListener(new InGameEventListener, "player_activate", false);
}