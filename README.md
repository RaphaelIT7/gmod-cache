This Project clears the Model Cache when disconnecting from a server, only the main branch of Gmod is supported.  

Note: This Project uses [Garry's Mod Common](https://github.com/danielga/garrysmod_common)

## How to Install
1. You need to download the `gmsv_clearcache_win32.dll` from the latest [release](https://github.com/RaphaelIT7/gmod-clearcache/releases)  
2. Move the `gmsv_clearcache_win32.dll` into your `garrysmod/lua/bin` folder.
3. You need insert
```lua
require("clearcache")
```
into the `garrysmod/lua/menu/menu.lua` file at the bottom.
