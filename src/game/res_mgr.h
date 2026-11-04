// res_mgr.h: the resource manager (global 0x5d6fa0) for the game-side modules that call it. The object is one: rgl/resource.h declares
// its lookup half as resource_manager (g_resmgr), rgl/core/resmgr.h its full layout as resmgr (get_object 0x419540 -- callers clone the
// mesh with 0x41c6d0 --, load_pack 0x41e150, get_shadow_skin 0x41c420); res_manager() is the object under the full class.
#ifndef GAME_RES_MGR_H
#define GAME_RES_MGR_H
#include "./tstring.h"
#include "../rgl/mesh.h"
#include "../rgl/core/resmgr.h"

static __forceinline resmgr& res_manager() { return *(resmgr*)&g_resmgr; }       // 0x5d6fa0

#endif // GAME_RES_MGR_H
