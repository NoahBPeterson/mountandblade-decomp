// Slice s0042b080 (second file): the scene's ambient colour (0x42b6d0). It is compiled apart from
// create_shadow_mesh (0x42b080): with its body visible to that function cl shares the stack slot of its
// returned vector with other locals, which the original does not.
#include "shadow.h"

// @ 0x42b6d0
// The ambient light colour of the scene: the colour of the last enabled ambient light (type 2, flags bit 0),
// scaled by 0.7 and added to the base ambient at +0x68.
vector4 Ctx::ambient()
{
    vector4 c(0.0f, 0.0f, 0.0f);
    for (unsigned i = 0; i < lights.size(); i++) {
        Light* l = lights[i];
        if (l->type == 2 && (l->flags & 1))
            c = l->color;
    }
    vector4 t(c.x * 0.7f, c.y * 0.7f, c.z * 0.7f);
    return vector4(t.x + amb[0], t.y + amb[1], t.z + amb[2]);
}
