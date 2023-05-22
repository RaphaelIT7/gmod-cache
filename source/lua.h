#include <GarrysMod/FactoryLoader.hpp>
#include <GarrysMod/Lua/Interface.h>

extern GarrysMod::Lua::ILuaBase* GetRealm(int);
extern void LUA_InitMenu(GarrysMod::Lua::ILuaBase*);
extern void LUA_InitClient(GarrysMod::Lua::ILuaBase*);
extern void LUA_UnLoadClient();