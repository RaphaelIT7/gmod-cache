#include <GarrysMod/Lua/Interface.h>
#include <GarrysMod/FactoryLoader.hpp>
#include "GameEventListener.h"
#include "gameevents.h"
#include "util.h"
#include "cache.h"
#include "lua.h"

GMOD_MODULE_OPEN()
{
	GlobalLUA = LUA;

	Cache = new CacheMgr();
	Gameevents_Init();

	LUA_InitMenu(LUA);

	return 0;
}

GMOD_MODULE_CLOSE()
{
	return 0;
}