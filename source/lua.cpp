#include <GarrysMod/FactoryLoader.hpp>
#include <GarrysMod/Lua/Interface.h>
#include "util.h"

static SourceSDK::FactoryLoader luashared_loader("lua_shared");

LUA_FUNCTION_STATIC(cache_checkasync) {
	LUA->CheckType(1, GarrysMod::Lua::Type::Bool);
	bool print = LUA->GetBool(1);

	Cache->GetAsyncCacheDataType(MDLCACHE_STUDIOHDR, print, "MDLCACHE_STUDIOHDR");
	Cache->GetAsyncCacheDataType(MDLCACHE_STUDIOHWDATA, print,"MDLCACHE_STUDIOHWDATA");
	Cache->GetAsyncCacheDataType(MDLCACHE_VCOLLIDE, print, "MDLCACHE_VCOLLIDE");
	Cache->GetAsyncCacheDataType(MDLCACHE_ANIMBLOCK, print, "MDLCACHE_ANIMBLOCK");
	Cache->GetAsyncCacheDataType(MDLCACHE_VIRTUALMODEL, print, "MDLCACHE_VIRTUALMODEL");
	Cache->GetAsyncCacheDataType(MDLCACHE_VERTEXES, print, "MDLCACHE_VERTEXES");
	Cache->GetAsyncCacheDataType(MDLCACHE_DECODEDANIMBLOCK, print, "MDLCACHE_DECODEDANIMBLOCK");

	return 0;
}

LUA_FUNCTION_STATIC(cache_setasync) 
{
	LUA->CheckType(2, GarrysMod::Lua::Type::Bool);
	int type = LUA->CheckNumber(1);
	bool enabled = LUA->GetBool(2);

	bool worked = Cache->SetAsyncCacheDataType((MDLCacheDataType_t)type, enabled);
	LUA->PushBool(worked);

	return 1;
}

LUA_FUNCTION_STATIC(cache_getasync)
{
	int type = LUA->CheckNumber(1);

	bool worked = Cache->GetAsyncCacheDataType((MDLCacheDataType_t)type);
	LUA->PushBool(worked);

	return 1;
}

LUA_FUNCTION_STATIC(cache_flush) {
	if (Cache->Flush(1000, true, true, false)) {
		LuaPrint("[Cache] Flush begins in 1 second!");
	} else {
		LuaPrint("[Cache] You cannot flush the Cache if you are connected to a Server!");
	}

	return 0;
}

LUA_FUNCTION_STATIC(cache_shouldclear) {
	const char* address = LUA->CheckString(1);
	LUA->CheckType(2, GarrysMod::Lua::Type::Bool);
	bool clear = LUA->GetBool(2);

	Cache->SetClearOnConnect(address, clear);

	return 0;
}

GarrysMod::Lua::ILuaBase* GetRealm(int realm) {
	CLuaShared* LuaShared = (CLuaShared*)luashared_loader.GetFactory()("LUASHARED003", nullptr);
	if (LuaShared == nullptr)
		Msg("I've failed\nYou\n");

	return LuaShared->GetLuaInterface(realm);
}

void LUA_InitMenu(GarrysMod::Lua::ILuaBase* LUA) {
	LUA->PushSpecial(SPECIAL_GLOB);
		LUA->GetField(-1, "concommand");
		LUA->GetField(-1, "Add");
		LUA->PushString("cache_flush");
		LUA->PushCFunction(cache_flush);
		LUA->Call(2, 0);
	LUA->Pop();

#if Cache_Debug == 1
	LUA->GetField(-1, "engine");
	LUA->PushCFunction(check_async);
	LUA->SetField(-2, "check_async");
	LUA->Pop(1);
#endif

	//LUA->PushSpecial(SPECIAL_GLOB);
	//	LUA->GetField(-2, "cache");
	//	bool type = LUA->IsType(-1, Type::NIL);
	//	if (!type) {
	//		LuaPrint("[Cache] Override protection. The table cache already exists!", LUA);
	//		return;
	//	}
	//LUA->Pop();

	Start_Table(LUA);
		Add_Func(LUA, cache_flush, "Flush");
		Add_Func(LUA, cache_setasync, "SetAsync");
		Add_Func(LUA, cache_getasync, "GetAsync");
		Add_Func(LUA, cache_shouldclear, "ShouldClear");
	Finish_Table(LUA, "cache");

	LUA->PushSpecial(SPECIAL_GLOB);
		LUA->PushNumber(1);
		LUA->SetField(-2, "MDLCACHE_STUDIOHDR");
		LUA->PushNumber(2);
		LUA->SetField(-2, "MDLCACHE_STUDIOHWDATA");
		LUA->PushNumber(3);
		LUA->SetField(-2, "MDLCACHE_VCOLLIDE");
		LUA->PushNumber(4);
		LUA->SetField(-2, "MDLCACHE_ANIMBLOCK");
		LUA->PushNumber(5);
		LUA->SetField(-2, "MDLCACHE_VIRTUALMODEL");
		LUA->PushNumber(6);
		LUA->SetField(-2, "MDLCACHE_VERTEXES");
		LUA->PushNumber(7);
		LUA->SetField(-2, "MDLCACHE_DECODEDANIMBLOCK");
	LUA->Pop();

	LuaPrint("[Cache] Successfully Loaded.");
}

static bool Init = false;
void LUA_InitClient(GarrysMod::Lua::ILuaBase* LUA) {
	if (Init) { return; }

	//LUA->PushSpecial(SPECIAL_GLOB);
	//	LUA->GetField(-1, "cache");
	//	bool type = LUA->IsType(-1, Type::NIL);
	//	if (!type) {
	//		LuaPrint("[Cache] Override protection. The table cache already exists!", LUA);
	//		return;
	//	}
	//LUA->Pop();

	LUA->PushSpecial(SPECIAL_GLOB);
		LUA->PushNumber(1);
		LUA->SetField(-2, "MDLCACHE_STUDIOHDR");
		LUA->PushNumber(2);
		LUA->SetField(-2, "MDLCACHE_STUDIOHWDATA");
		LUA->PushNumber(3);
		LUA->SetField(-2, "MDLCACHE_VCOLLIDE");
		LUA->PushNumber(4);
		LUA->SetField(-2, "MDLCACHE_ANIMBLOCK");
		LUA->PushNumber(5);
		LUA->SetField(-2, "MDLCACHE_VIRTUALMODEL");
		LUA->PushNumber(6);
		LUA->SetField(-2, "MDLCACHE_VERTEXES");
		LUA->PushNumber(7);
		LUA->SetField(-2, "MDLCACHE_DECODEDANIMBLOCK");
	LUA->Pop();

	Start_Table(LUA);
		Add_Func(LUA, cache_setasync, "SetAsync");
		Add_Func(LUA, cache_getasync, "GetAsync");
		Add_Func(LUA, cache_shouldclear, "ShouldClear");
	Finish_Table(LUA, "cache");

	LuaPrint("[Cache] Successfully added client functions.");

	Init = true;
}

void LUA_UnLoadClient() {
	Init = false;
}