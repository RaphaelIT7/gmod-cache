#pragma once

#include "notify.h"
#include <unordered_map>
#include "GameEventListener.h"
#include "datacache/imdlcache.h"
#include "datacache/idatacache.h"

#define Cache_Debug 1
#define Cache_UseNotify 1 // Set this to 1 if you want to use a Cache instead of clearing the whole cache each time. This can cause the lightning on some models to break!
#define Cache_AwaysFlush 0 // Enable this if you want to clear the cache every time you disconnect from a server. This can cause crashes!
#define Cache_Experimental 0 // Enable this to enable features that are in development like clearing the Materials too. (This is Experimental)

class Cache_Entry
{
public:
	MDLHandle_t handle;
	MDLCacheDataType_t type;

	Cache_Entry(MDLHandle_t, MDLCacheDataType_t);

	void Unload(IMDLCache*);
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

class CacheMgr
{
public:
	bool connected = false;
	bool alwaysflush = false;
	const char* address = "";
	bool clear_on_connect = false;

	#if Cache_AwaysFlush == 0
		const char* last_map = "";
		const char* last_server = "";
	#endif

	CacheMgr();

	bool Flush(int, bool, bool, bool);

	void Connect(IGameEvent* event);
	void Disconnect();
	void Shutdown();
	
	bool SetAsyncCacheDataType(MDLCacheDataType_t, bool);
	bool SetAsyncCacheDataType(MDLCacheDataType_t, bool, bool, const char*);

	bool GetAsyncCacheDataType(MDLCacheDataType_t);
	bool GetAsyncCacheDataType(MDLCacheDataType_t, bool, const char*);

	void SetClearOnConnect(const char*, bool); // Give an address of an server the client should or should not clear the cache when he connects to it. SetClearOnConnect("localhost", true) This will clear the cache when the client connects into a singleplayer game.
};

// PS: I'm not good in naming things :(