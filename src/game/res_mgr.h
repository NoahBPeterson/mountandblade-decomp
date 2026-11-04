// res_mgr.h: what the game-side modules that call the resource manager (g_resmgr, rgl/resource.h's resmgr, global 0x5d6fa0) include: the
// manager's loader members (get_object 0x419540 -- callers clone the mesh with 0x41c6d0 --, load_pack 0x41e150, get_shadow_skin 0x41c420)
// are core/resmgr.h's companions (mesh_x, the stream and resource readers).
#ifndef GAME_RES_MGR_H
#define GAME_RES_MGR_H
#include "./tstring.h"
#include "../rgl/mesh.h"
#include "../rgl/core/resmgr.h"

#endif // GAME_RES_MGR_H
