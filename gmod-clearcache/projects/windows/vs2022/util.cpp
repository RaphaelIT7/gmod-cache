#include "GarrysMod/Lua/Interface.h"
#include "util.h"
#include <string>

GarrysMod::Lua::ILuaBase* GlobalLUA;

// should never be used outside of main thread!!! what happends: memory access violation
void LuaPrint(char* Text) {
	GlobalLUA->PushSpecial(SPECIAL_GLOB);
	GlobalLUA->GetField(-1, "print");
	GlobalLUA->PushString(Text);
	GlobalLUA->Call(1, 0);
}

// should never be used outside of main thread!!! what happends: memory access violation
void LuaPrint(std::string Text) {
	GlobalLUA->PushSpecial(SPECIAL_GLOB);
	GlobalLUA->GetField(-1, "print");
	GlobalLUA->PushString(Text.c_str());
	GlobalLUA->Call(1, 0);
}