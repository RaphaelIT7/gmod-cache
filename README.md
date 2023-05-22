This Project clears the Model Cache when disconnecting from a server, only the main branch of Gmod is supported.  
It also activates Async Load for some DataTypes. (`MDLCACHE_STUDIOHWDATA`, `MDLCACHE_VCOLLIDE` and `MDLCACHE_VERTEXES`)

Note: This Project uses [Garry's Mod Common](https://github.com/danielga/garrysmod_common)

## Current Status
Bugs:  
- Content from Addons also get unloaded.
- Playermodel sometimes break

What I'm currently working on:  
- Better Cache handling
  - Try to only unload Map content first
  - Try to only unload Content downloaded by the Server.
  - Try not to unload Content from Addons
  - Add some type of tag that allows a server to stop the cache / tell the cache to not unload Content.

## How to Install
1. You need to download the `gmsv_cache_win32.dll` from the latest [release](https://github.com/RaphaelIT7/gmod-clearcache/releases)  
2. Move the `gmsv_cache_win32.dll` into your `garrysmod/lua/bin` folder.
3. You need insert

Windows:
```lua
if file.Exists("lua/bin/gmsv_cache_win32.dll", "GAME") then
  require("cache")
end
```

Linux:
```lua
if file.Exists("lua/bin/gmsv_cache_linux.dll", "GAME") then
  require("cache")
end
```

Into the `garrysmod/lua/menu/menu.lua` file at the bottom.

Note: If you change your Gmod branch or an update is made, your changes could get removed.

## How it currently works.
You can use the `cache_flush` command to manually flush the cache if it somehow didn't flush it itself, or if you just want to clear the cache completely.   
This command only works when you're not connected to a Server!  
NOTE: This command will flush some data that is not cleared automatically like `MDLCACHE_FLUSH_STUDIOHDR` or `MDLCACHE_FLUSH_VCOLLIDE` data because flushing them while joining a Server can cause some unexpected crashes or bugs like some Models having no collisions.  
(If you want to know what exactly crashes, `vphysics.dll` and `datacache.dll` are the ones crashing)

```mermaid
graph TD;
    %%{init: {"theme": "dark", "fontFamily": "monospace", "flowchart": {"htmlLabels": true, "curve": "linear"}, "sequence": {"mirrorActors": true}}}%%
    startup[Gmod Starts]-->cache[Cache Initializes]
    cache-->cache_async[Enable AsyncLoad for some DataTypes]
    cache-->cache_cmd[Add the cache_flush command]
    
    cache_cmd-->first_server
    cache_async-->first_server
    
    first_server[First time Connecting to a Server]-->cache_savemap[Cache saves the Mapname]
    cache_savemap-->connect((You connect to a different Server))
    connect-->cache_check[The cache checks if it's the same map by\ncomparing the saved Mapname with the Mapname of the Server]
    cache_check-- If it's not the same Map, clear the Cache\nIf it's the same Map, do nothing -->connect
```

## Next Update  
[+] Added LUA API  
[#] Cleaned up the Code  
[#] Small problems fixed  

--- MDLCACHE_ Types ---  
MDLCACHE_STUDIOHDR = 1  
MDLCACHE_STUDIOHWDATA = 2  
MDLCACHE_VCOLLIDE = 3  
MDLCACHE_ANIMBLOCK = 4  
MDLCACHE_VIRTUALMODEL = 5  
MDLCACHE_VERTEXES = 6  
MDLCACHE_DECODEDANIMBLOCK = 7  

--- Menu State ---  
void cache.Flush() Call this if you want to flush the cache.  
bool cache.GetAsync() retuns a bool if the given MDLCACHE type is Async.  
bool cache.SetAsync(int MDLCACHE_[any], bool enabled) Enables Async for the given type. retuns a bool if the given MDLCACHE type has Async enabled. Some types can fail or cause problems!  
void cache.ShouldClear(string IP, bool clear) When the clients joins the given IP, the cache will be cleared if you want it to. (You can only controll the Cache for 1 IP!)  

--- Client State ---  
bool cache.GetAsync() retuns a bool if the given MDLCACHE type is Async.  
bool cache.SetAsync(int MDLCACHE_[any], bool enabled) Enables Async for the given type. retuns a bool if the given MDLCACHE type has Async enabled. Some types can fail or cause problems!  
void cache.ShouldClear(string IP, bool clear) When the clients joins the given IP, the cache will be cleared if you want it to. (You can only controll the Cache for 1 IP!)  
