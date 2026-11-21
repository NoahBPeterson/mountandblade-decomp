// item_kind.h: the namespace-scope pos alias of rgl::matrix and the resource lookup item_kind.cpp (0x433bf0..0x434b20) uses
// beside the item definition record. The record itself (ItemKind, ItemMesh, g_items, itp_type_*, imod_*) is the shared
// src/include/item_kind.h, included here. Names provisional.
#ifndef AGENT_ITEM_KIND_H
#define AGENT_ITEM_KIND_H
#include <vector>
#include "../../rgl/rgl_math.h"
#include "../../rgl/rgl_string.h"
#include "../tstring.h"
#include "../../include/item_kind.h"

using rgl::vector4;

namespace rgl { struct matrix; } typedef rgl::matrix pos;                                 // the global-namespace twin of rgl::matrix that the definition of 0x40cd00 names (src/include/mat4.h)

#endif // AGENT_ITEM_KIND_H
