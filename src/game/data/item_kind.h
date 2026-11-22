// item_kind.h: the namespace-scope pos alias of rgl::matrix and the resource lookup item_kind.cpp (0x433bf0..0x434b20) uses
// beside the item definition record. The record itself (ItemKind, ItemMesh, g_items, itp_type_*, imod_*) is the shared
// src/game/data/item_kind_include.h, included here. Names provisional.
#ifndef AGENT_ITEM_KIND_H
#define AGENT_ITEM_KIND_H
#include <vector>
#include "engine/math/rgl_math.h"
#include "engine/core/rgl_string.h"
#include "engine/core/tstring.h"
#include "game/data/item_kind_include.h"

using rgl::vector4;

namespace rgl { struct matrix; } typedef rgl::matrix pos;                                 // the global-namespace twin of rgl::matrix that the definition of 0x40cd00 names (src/engine/math/mat4_include.h)

#endif // AGENT_ITEM_KIND_H
