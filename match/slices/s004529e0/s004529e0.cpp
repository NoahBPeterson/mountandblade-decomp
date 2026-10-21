// Slice s004529e0: Agent::drop_held (a random chance per frame to throw a held item into the world), the text
// layout wrapper (0x452bb0), the on-screen message log (MsgLog::add: wraps long lines at a space, builds the text
// mesh and its background and scrolls the 32-entry ring), the animator visibility switch (0x452f80), a log helper
// (0x453020) and Agent::get_label_box (the box above an agent's head for its name/health label).
#include <stdlib.h>
#include "agent_fx.h"

// @ 0x4529e0
void Agent::drop_held()
{
    int i = 0;
    ItemRef* it = items;
    SlotLink** lp = links[1];
    for (; i < 4; i++, lp++, it++) {
        if (it->item >= 0) {
            int v = rand() % 15817;
            if ((float)v * (1.0f / 15817.0f) < 0.25f) {
                ItemKind* kind = &g_items[it->item];
                pos M;
                identity_pos(&M);
                int bone;
                int bflags;
                v = -1;
                if (wield_a == i || wield_b == i)
                    bone = kind->wield_bone();
                else if ((kind->capability_flags & itcf_carry_mask) != 0) {
                    kind->get_carry_pos(&M, &v, &bflags);
                    bone = v;
                } else
                    bone = 0x13;
                Skeleton* sk = body->skel;
                mat4 T1 = body->m * *(mat4*)sk->rec_inl(bone);
                mat4 T2 = T1 * *(mat4*)&M;
                T2.o.z += 0.1f;
                if (lp[-4])
                    lp[-4]->m_cc = 0;
                if (lp[0])
                    lp[0]->m_cc = 0;
                if (lp[4])
                    lp[4]->m_cc = 0;
                v = 6;
                g_mission->add_placed(&v, &it->item, (int*)&it->m4, (matrix*)&T2, &id);
                it->item = -1;
            }
        }
    }
}

// @ 0x452bb0
vector4* Font::layout_clear(vector4* out_extent, const tstring* text, const vector4* pos, const vec2* scale, mesh* m,
                         const int* a, const int* b, const float* c, int flags)
{
    m->clear_geometry();
    layout(out_extent, text, pos, scale, m, a, b, c, flags);
    return out_extent;
}

// @ 0x452c00
void MsgLog::add(tstring msg, int color)
{
    pos M;
    tstring S = msg.strip_tags();
    while (lines.size() > 100)
        lines.erase(lines.begin());
    if (enabled) {
        if (S.len > 0x50) {
            int a = 0, b = 0;
            while (b >= 0 && b < 0x50) {
                a = b;
                char c = ' ';
                b = msg.find(c, b + 1);
            }
            if (a > 0) {
                tstring first = msg.substr(0, a);
                msg = msg.substr(a + 1, msg.len);
                add(first, color);
                add(msg, color);
                return;
            }
        }
        {
            vec2 scale;
            int zero;
            float f08;
            int minus1;
            vector4 p;
            vector4 ext;
            scale.x = 0.0197f;
            scale.y = 0.0197f;
            f08 = 0.8f;
            cur = (cur + 31) % 32;
            text[cur].clear_geometry();
            bg[cur].clear_geometry();
            zero = 0;
            minus1 = -1;
            p.x = 0.02f;
            p.y = -0.03f;
            p.z = 0.0f;
            g_font->layout_clear(&ext, &S, &p, &scale, &text[cur], &minus1, &zero, &f08, 0);
            identity_pos(&M);
            bg[cur].append_skinned(&text[cur], (mat4*)&M, -1, -1, 0);
            this->color[cur] = color;
            age[cur] = 0;
            text[cur].calc_bounds(&ext, &p);
            height[cur] = p.y - ext.y;
            total_h += height[cur];
        }
        text[cur].res = g_resmgr.get_material(tstring("ui_font"));
        bg[cur].res = g_resmgr.get_material(tstring("ui_font_background"));
        lines.push_back(msg);
    }
}

// @ 0x452f80
void Skeleton::set_visible(char on)
{
    if (!frozen) {
        if (on) {
            if (!done) {
                if (has_poses)
                    update_poses();
                else
                    update();
            }
            for (unsigned i = 0; i < psys.size(); i++)
                skin(psys[i], 1);
            for (unsigned j = 0; j < groups.size(); j++)
                skin_group(groups[j], 1);
        }
    }
    frozen = on;
}

// @ 0x453020
void __stdcall log_message(const tstring* s, int unused)
{
    g_msglog.add(*s, -261);
}

// box above the agent's head (two corner vectors and a scale), for its label
struct LabelBox {
    char pad0[0x20];
    vector4 a;                  // +0x20
    vector4 b;                  // +0x30
    char pad40[0x60 - 0x40];
    float scale;                // +0x60
};

// @ 0x453050
void Agent::get_label_box(void* out_)
{
    LabelBox* out = (LabelBox*)out_;
    if (f10 != 0) {
        out->a = vector4(px, py - 0.3f, pz + 0.8f);
        out->b = vector4(px, py + 0.8f, pz + 0.85f);
        out->scale = 0.5f;
    } else if (body != 0) {
        vector4 p1 = body->m.transform_point(*(vector4*)(body->skel->elems + 0x648));
        vector4 p2 = body->m.transform_point(*(vector4*)(body->skel->elems + 0xbdc));
        p2.z = p2.z + p1.z;
        p1.x = (p2.x + p1.x) * 0.5f;
        p1.y = (p2.y + p1.y) * 0.5f;
        p1.z = p2.z * 0.5f;
        out->a = p1;
        out->b = body->m.transform_point(*(vector4*)(body->skel->elems + 0x1170));
        out->scale = f80 * 0.33f;
    } else {
        g_msglog.add(tstring("WARNING:CHECK INVALID ENTITY"), -1);
        float z;
        if (f2c == -1)
            z = pz + 0.8f;
        else
            z = pz + 1.5f;
        vector4 lo(px, py, z - 0.5f);
        vector4 hi(px, py, z + 0.5f);
        out->a = lo;
        out->b = hi;
        out->scale = 0.4f;
    }
}
