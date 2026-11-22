//-----------------------------------------------------------------------------
// File: terrain_brush.cpp  (0x52bf40..0x52cf60)
//
// Desc: Terrain::update / clear_brushed / brush (the scene editor brush), TerrainDescEntry clear / put, Terrain::clamp_cell
//       One part of what was game/mission5/mission_5.cpp (the object's functions in address order; the cut points and the evidence for
//       them are in docs/source_tree.md section 3). The prelude and the helper definitions below are the
//       parts of the old file's prelude and earlier sections this part's code uses.
//-----------------------------------------------------------------------------

#define PLACEDOBJ_CTOR_OUT_OF_LINE   // fn_529d80 0x529d80 calls PlacedObj::PlacedObj (0x437830) for the resize(0) temporary
#include "platform/image_addrs.h"
#include "game/data/skill_cap.h"      // SkillCap g_skill_tbl (0x8ee3100): [36].cap is the clamp at 0x8ee3ac4
#include "game/config/config_block.h"      // Config g_cfg (0x5a9f98)
#include "game/mission/mission_frame_update.h"
#include "game/campaign/game_state_load.h"      // Game::more_start (g_game is the game constructor's Game)
#include "game/data/troop.h"   // Troop::unequip (0x4630d0)
#include "engine/core/tstring.h"
#include "engine/math/rgl_math.h"
#include "engine/io/stream.h"
#include "engine/io/stream_inline.h"   // the file writer built on save_scene's stack: constructor inline
#include "game/script/script_state.h"   // ScriptState g_script (0xa70440): the one object
#include "engine/render/mesh.h"
#include "game/ui/gui_manager.h"      // g_gui (0x5d7080)
#include "game/ui/screens/conversation/conv_screen.h"      // Rec1208 (ConvScreen): the conversation screen record, whose slot 3 is handle_event 0x525160
#include "platform/wl_marker.h"   // WL_MARKER: the original's inline WinLicense region markers
#include "game/data/item_kind_include.h"   // g_items 0x5e29d0
#include "game/data/menu_rec.h"   // g_menus 0x5e29b0
#include "game/data/mode_rec.h"   // g_mission_templates 0x5e2a00
#include "game/data/presentation.h"                               // widget_mgr, g_presentation (0x5e299c)
#include "game/input/key_names.h"
#include "game/data/module_settings.h"      // ModuleSettings, g_module_settings (0xa180f8, defined in game/app/options_dialog_and_module_tables.cpp)
#include "engine/render/render_flags.h"
#include "game/ui/ui_globals.h"      // UI / core globals
#include "game/campaign/map_flags.h"      // MapFlags::commit 0x513710
#include "game/config/core.h"     // GameApp g_app_obj (0x8ee3c48): the map editor dialog's members

// ---- definitions from the old file's prelude and earlier sections that this part's code uses (copies; each is a static
// or inline helper, a type or a macro: none of them emits code or data of its own)
// 0x407d60 (engine/physics/collision_shapes.cpp owns the row): body visible in the unit, so pointers passed to it do not escape
namespace rgl {
inline __declspec(noinline) float vector4::normalize()      // 0x407d60
{
    float len = length();
    if (len > 1e-10f) {
        float inv = 1.0f / len;
        x *= inv; y *= inv; z *= inv;
    } else {
        x = 0.0f; y = 1.0f; z = 0.0f;
    }
    return len;
}
}




// ======================================================= slice s0052bf40: functions
// @ 0x52bf40
void Terrain::update(TerrainDesc* desc, vector4* pos, int sel, int mode, int flag)
{
    clear_brushed();

    if (mode == 1)
        sel = desc->n_minus2;
    else if (mode == 2)
        sel = desc->n_minus1;

    float speed;
    if (g_game->editor.game_mode == 1)
        speed = g_game->editor.mx;
    else if (g_game->editor.game_mode == 3)
        speed = g_game->editor.mz;
    else
        speed = g_game->editor.my;

    float r1 = speed * speed;
    float r2 = this->scale * 10.0f + speed;
    r2 = r2 * r2;

    int cell[2];   // brush() reads {col, row} through one pointer (0x52c0a1: [esp+0x1c], [esp+0x20])
    for (cell[1] = 0; cell[1] <= this->h; cell[1]++) {
        for (cell[0] = 0; cell[0] <= this->w; cell[0]++) {
            terrain_cell& tc = this->cell[cell[0]][cell[1]];
            const vector4 dv = tc.pos - *pos;
            float d2 = dv.x * dv.x + dv.z * dv.z + dv.y * dv.y;
            if ((mode == 0 && d2 <= r2) || d2 <= r1) {
                char place = 1;
                float d = (float)sqrt(d2);
                if (!(d2 <= r1))   // 0x52c065: fcomp vs the squared distance (the fld st(0) copy), not d
                    place = 0;
                if (sel < 0)
                    return;
                if (sel >= desc->n)
                    return;
                brush(desc, &sel, cell, &d, &mode, &flag, &place);
            }
        }
    }

    if (mode == 0) {
        int rowbase = 0;
        for (int i = 0; i < desc->n - 2; i++) {
            char found = 0;
            for (int j = 0; j <= nx; j++) {
                for (int k = 0; k <= ny; k++) {
                    if (i + 4 < 0xf && brushed[i + 4][j][k] != 0)
                        found = 1;
                }
            }
            if (found)
                this->load_layer(desc, i);
            rowbase += 5;
        }
    } else if (mode == 1) {
        this->load_layer(desc, desc->n_minus2);
    } else if (mode == 2) {
        this->load_layer(desc, desc->n_minus1);
    }

    float h;
    {
        vector4 v = this->world->ambient;
        h = (v.x + v.y + v.z) * 0.3f;
    }

    for (int bx = 0; bx < this->nx; bx++) {
        for (int by = 0; by < this->ny; by++) {
            for (int layer = 0; layer < 0xf; layer++) {
                if (brushed[layer][bx][by] != 0) {
                    int blend = -1;
                    if (g_use_shaders != 0 && this->layer[1].mat2 != 0) {
                        if (layer == 1)
                            blend = 2;
                        else if (layer == 2)
                            continue;
                    }
                    this->emit_particle(layer, blend, bx, by, h);
                }
            }
        }
    }
}

// =====================================================================================================
//  Terrain_s2bf40::clear
// =====================================================================================================
// @ 0x52c290
__declspec(noinline) void Terrain::clear_brushed()
{
    // written as a byte loop (not memset) so cl expands it exactly like the
    // original: lea edi,[ecx+0x845e030]; xor eax,eax; mov ecx,0x5d;
    // rep stosd; stosw; stosb.
    char* p = (char*)brushed;
    for (int i = 0; i < 0x177; i++)
        p[i] = 0;
}

// ======================================================= slice s0052c2b0: functions
// the element's lazy value block: read with a default, store with the allocate-on-positive rule (expanded at every use)
static __forceinline float elem_get_s2c2b0(TerrainDescEntry* E, const int* c)
{
    float r = 0.0f;
    if (E->data)
        r = E->data[c[0] * 0x1a4 + c[1]];
    return r;
}
static __forceinline float clamp0_s2c2b0(float v) { return 0.0f > v ? 0.0f : v; }
static __forceinline void elem_set_s2c2b0(TerrainDescEntry* E, const int* c, float val)
{
    if (E->data == 0) {
        if (!(val > 0.0f))
            return;
        E->data = new float[0x1a4 * 0x1a4];
        E->clear();
    }
    E->data[c[0] * 0x1a4 + c[1]] = val;
}
// @ 0x52c2b0
void Terrain::brush(TerrainDesc* desc, int* pidx, int* cell, float* amt, int* op, int* fA, char* fB)
{
    // Stand-ins, no code: 0x52bf40 needs brush's cell, pidx, op and fA to count as escaping (without them it keeps its locals in shared slots).
    // An opaque brush gives 0x52bf40 the same bytes, so the original's escape was real or the callee was opaque; its real form is not known yet.
    int*& cell_r = cell; int*& pidx_r = pidx; int*& op_r = op; int*& fA_r = fA;
    int stampbase = 0;
    float sel;
    if (g_game->editor.game_mode == 1)      sel = g_game->editor.mx;
    else if (g_game->editor.game_mode == 3) sel = g_game->editor.mz;
    else                       sel = g_game->editor.my;

    float lo;
    if (g_game->editor.game_mode == 1)      lo = g_game->editor.tx;
    else if (g_game->editor.game_mode == 3) lo = g_game->editor.tz;
    else                       lo = g_game->editor.ty;

    float t = *amt / sel;
    if (t < 0.0f) t = 0.0f;
    else if (t >= 1.0f) t = 1.0f;

    float q = 1.0f - (1.0f - lo) * t;
    float hi;
    if (g_game->editor.game_mode == 1)      hi = g_game->editor.sx;
    else if (g_game->editor.game_mode == 3) hi = g_game->editor.sz;
    else                       hi = g_game->editor.sy;

    const float amount = hi * q;

    if (*(char*)fA != 0 && *fB != 0) {
        // ---- both flags set ------------------------------------------------------------------------
        if (*op == 0) {
            for (int i = 0; i < desc->n - 2; i++) {
                elem_set_s2c2b0(&desc->layers[i], cell,
                                clamp0_s2c2b0(elem_get_s2c2b0(&desc->layers[i], cell) - amount));
            }
            stampbase = 4;
        } else if (*op == 1) {
            TerrainDescEntry* E = &desc->layers[*pidx];
            elem_set_s2c2b0(E, cell, clamp0_s2c2b0(elem_get_s2c2b0(E, cell) * (1.0f - amount)));
        } else if (*op == 2) {
            TerrainDescEntry* E = &desc->layers[*pidx];
            unsigned argb = 0xffffffff;
            if (E->data) argb = *(unsigned*)&E->data[cell[0] * 0x1a4 + cell[1]];
            rgl::vector4 col = rgl::unpack_color(&argb);
            float k = 1.0f - amount;
            col = col * k + rgl::vector4(amount, amount, amount);   // the whole-struct assignment carries the temporary's unset w, as the original
            argb = rgl::pack_color(&col);
            E->put(cell, cell + 1, (int*)&argb);
        }
    } else {
        // ---- one of the two flags is clear: normal brush -------------------------------------------
        TerrainDescEntry* E = &desc->layers[*pidx];
        float v = elem_get_s2c2b0(E, cell);

        if (*op == 0) {
            float nv = v + amount;
            if (1.0f < nv) nv = 1.0f;

            int n = desc->n - 2;
            bool any = false;
            for (int i = *pidx + 1; i < n; i++) {
                if (elem_get_s2c2b0(&desc->layers[i], cell) > 0.98f)
                    any = true;
            }

            if (any) {
                elem_set_s2c2b0(E, cell, 1.0f);
            } else {
                if (*fB == 0) goto done;
                elem_set_s2c2b0(E, cell, nv);
            }

            if (*fB != 0) {
                for (int i = *pidx + 1; i < desc->n - 2; i++) {
                    if (desc->layers[i].data) {
                        float old = desc->layers[i].data[cell[0] * 0x1a4 + cell[1]];
                        if (old > 0.0f) {
                            float dv = old - amount;
                            if (dv < 0.0f) dv = 0.0f;
                            else if (dv >= old) dv = old;
                            elem_set_s2c2b0(&desc->layers[i], cell, dv);
                        }
                    }
                }
            }
        } else if (*op == 1) {
            if (g_game->editor.axis == 1) {
                v = v - amount;
            } else if (g_game->editor.axis == 0) {
                v = v + amount;
            } else {
                // response = average of the nine surrounding terrain tiles, blended by `amount`
                int x = cell[0];
                int cnt = 0;
                int yp1 = cell[1] + 1;
                float sum = 0.0f;
                if (x >= 0 && x < w && yp1 >= 0 && yp1 < h) {
                    sum = flat()[x * 251 + yp1].f88
                        + flat()[x * 251 + yp1].h2;
                    cnt = 1;
                }
                int xp1 = x + 1;
                if (xp1 >= 0) {
                    if (xp1 < w && yp1 >= 0 && yp1 < h) {
                        sum += flat()[xp1 * 251 + yp1].f88
                             + flat()[xp1 * 251 + yp1].h2;
                        cnt++;
                    }
                    if (xp1 >= 0 && xp1 < w && cell[1] >= 0 && h > cell[1]) {
                        cnt++;
                        sum += flat()[xp1 * 251 + cell[1]].f88
                             + flat()[xp1 * 251 + cell[1]].h2;
                    }
                }
                int ym1 = cell[1] - 1;
                if (xp1 >= 0 && xp1 < w && ym1 >= 0 && ym1 < h) {
                    sum += flat()[xp1 * 251 + ym1].f88
                         + flat()[xp1 * 251 + ym1].h2;
                    cnt++;
                }
                if (x >= 0 && x < w && ym1 >= 0 && ym1 < h) {
                    sum += flat()[x * 251 + ym1].f88
                         + flat()[x * 251 + ym1].h2;
                    cnt++;
                }
                int xm1 = x - 1;
                if (xm1 >= 0) {
                    if (xm1 < w && ym1 >= 0 && ym1 < h) {
                        cnt++;
                        sum += flat()[xm1 * 251 + ym1].f88
                             + flat()[xm1 * 251 + ym1].h2;
                    }
                    if (xm1 >= 0 && xm1 < w && cell[1] >= 0 && cell[1] < h) {
                        cnt++;
                        sum += flat()[xm1 * 251 + cell[1]].f88
                             + flat()[xm1 * 251 + cell[1]].h2;
                    }
                    if (xm1 >= 0 && xm1 < w && yp1 >= 0 && yp1 < h) {
                        sum += flat()[xm1 * 251 + yp1].f88
                             + flat()[xm1 * 251 + yp1].h2;
                        cnt++;
                    }
                }
                if (x >= 0 && x < w && cell[1] >= 0 && cell[1] < h) {
                    cnt++;
                    sum += flat()[x * 251 + cell[1]].f88
                         + flat()[x * 251 + cell[1]].h2;
                }
                float centre = flat()[x * 251 + cell[1]].f88
                             + flat()[x * 251 + cell[1]].h2;
                v = ((1.0f - amount) * centre + ((centre + sum) * amount) / (float)(cnt + 1)) + v - centre;
            }
            elem_set_s2c2b0(E, cell, v);
        } else if (*op == 2) {
            // blend the tile colour toward the brush colour
            unsigned argb = 0xffffffff;
            if (E->data) argb = *(unsigned*)&E->data[cell[0] * 0x1a4 + cell[1]];
            rgl::vector4 col = rgl::unpack_color(&argb);
            float k = 1.0f - amount;
            col = col * k + amount * g_game->editor.col;   // the whole-struct assignment carries the temporary's unset w, as the original
            argb = rgl::pack_color(&col);
            E->put(cell, cell + 1, (int*)&argb);
        }
    }

done:
    int P1[2], P2[2], P3[2];
    clamp_cell(P1, cell);
    int up[2]   = { cell[0] + 1, cell[1] + 1 };
    int down[2] = { cell[0] - 1, cell[1] - 1 };
    clamp_cell(P2, up);
    clamp_cell(P3, down);

    if (stampbase < 0xf) {
        int k = stampbase * 5;
        do {
            ((char*)brushed)[(P1[0] + k) * 5 + P1[1]] = 1;
            ((char*)brushed)[(P2[0] + k) * 5 + P2[1]] = 1;
            ((char*)brushed)[(P3[0] + k) * 5 + P3[1]] = 1;
            k += 5;
        } while (k < 0x4b);
    }
}

// ======================================================= slice s0052ce00: functions
// @ 0x52ce00
void TerrainDescEntry::clear()
{
    if (data != 0) {
        for (int x = 0; x < 0x1a4; x++)
            for (int y = 0; y < 0x1a4; y++)
                data[x * 0x1a4 + y] = 0.0f;
    }
}

// @ 0x52ce70
void TerrainDescEntry::put(int* ix, int* iy, int* value)
{
    if (data == 0) {
        if (*value == -1)
            return;
        data = new float[0x1a4 * 0x1a4];
        for (int x = 0; x < 0x1a4; x++)
            for (int y = 0; y < 0x1a4; y++)
                ((int*)data)[x * 0x1a4 + y] = -1;
    }
    ((int*)data)[*ix * 0x1a4 + *iy] = *value;
}

// @ 0x52cef0
__declspec(noinline) int* Terrain::clamp_cell(int* out, int* in)
{
    int ox, oy;
    int ix = in[0] / block;
    if (ix < 0)
        ox = 0;
    else if (ix >= nx)
        ox = nx - 1;
    else
        ox = ix;
    int iy = in[1] / block;
    if (iy < 0)
        oy = 0;
    else if (iy >= ny)
        oy = ny - 1;
    else
        oy = iy;
    out[0] = ox;
    out[1] = oy;
    return out;
}
