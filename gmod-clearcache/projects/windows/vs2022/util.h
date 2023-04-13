#pragma once

#include "GarrysMod/Lua/Interface.h"
#include <string>

using namespace GarrysMod::Lua;

extern GarrysMod::Lua::ILuaBase* GlobalLUA;

extern void LuaPrint(char*);
extern void LuaPrint(std::string);