// Slice s00454e30: three Agent members around the blow/name code at 0x454e30..0x4556c3 (names provisional):
//  0x454e30 Agent::reduce_blow  - armor absorption of an incoming blow: a shield worn on the back (capsule along the blow
//                                 direction vs. the weapon sphere) adds bonus armor, then an exponential falloff splits the
//                                 damage into the absorbed part (rounded) and the remainder
//  0x4552c0 Agent::draw_name    - draws the troop's name above an agent that the local player considers a friend
//  0x4554c0 Agent::spawn_corpse - drops the agent onto the floor below its position with a random spin and starts the ragdoll
#include "agent_life.h"

// @ 0x454e30
void Agent::reduce_blow(Blow* b)
{
    float dmg = b->damage_in;
    int idx;
    if (f10 == 1)
        idx = 0;
    else {
        int t = b->type;
        if (t == 9)
            idx = 0;
        else if (t == 1 || t == 4 || t == 2 || t == 5 || t == 3 || t == 6)
            idx = 2;
        else
            idx = 1;
    }
    float armor_v = (float)armor[idx];
    if (f10 == 0) {
        ItemRef w = best_weapon_slot();
        if (w.item >= 0 && b->hit) {
            ItemKind* ik = &g_items[w.item];
            mat4 carry;
            int bone, aux;
            ik->get_carry_pos(&carry, &bone, &aux);
            mat4 r = *(mat4*)body->skel->rec(bone);
            mat4 u = body->m * (r * carry);
            HitInfo* h = b->hit;
            if (u.u.y * h->dy + u.u.z * h->dz + u.u.x * h->dx < -0.5f) {
                float hi[3], lo[3];
                hi[0] = h->dx * 4.0f + h->px;
                hi[1] = h->dy * 4.0f + h->py;
                hi[2] = h->dz * 4.0f + h->pz;
                lo[0] = h->px - h->dx * 4.0f;
                lo[1] = h->py - h->dy * 4.0f;
                lo[2] = h->pz - h->dz * 4.0f;
                capsule cap(0.0f, (vector4*)lo, (vector4*)hi);
                float zero = 0.0f;
                sphere_shape sph(*(vector4*)&u.o, &zero);
                float gap = capsule_sphere_gap(&cap, &sph);
                if (gap < (float)ik->f20c * 0.01f) {
                    int bonus = w.stat_200() + 10;
                    armor_v = (float)bonus + armor_v;
                    if (id == g_mission->local_id) {
                        tstring key("ui_hit_shield_on_back");
                        g_msglog.add(*g_strtable.lookup(key), -1);
                    }
                }
            }
        }
    }
    float x, y;
    if (b->dmg_kind == 0) {
        x = g_armor_a[0] * armor_v;
        armor_v = g_armor_b[0] * armor_v;
    } else if (b->dmg_kind == 1) {
        x = g_armor_a[1] * armor_v;
        armor_v = g_armor_b[1] * armor_v;
    } else {
        x = g_armor_a[2] * armor_v;
        armor_v = g_armor_b[2] * armor_v;
    }
    y = ((float)(rand() % 0x3dc9) * 3.4772715e-05f + 0.45f) * x;
    x = dmg - y;
    if (!(0.0f > x)) {
    } else
        x = 0.0f;
    int iv = rand() % 0x3dc9;
    float part = (1.0f - 1.0f / (float)exp((double)(armor_v * 0.014f * ((float)iv * 3.4772715e-05f + 0.45f)))) * x;
    if (armor_v < 0.0001f)
        part = 0.0f;
    y = part + y;
    double fl = ::floor((double)y);
    int n = (int)fl;
    if (!(y - fl <= 0.5))
        n++;
    b->absorbed = n;
    x = ((float)(rand() % 0x3dc9) * 2.5289246e-05f + 0.6f) * dmg - b->absorbed;
    b->damage_out = x;
    if (f10 != 1) {
        if (b->type == 9)
            b->damage_out = x * 1.2f;
        else if (b->type == 1 || b->type == 4)
            b->damage_out = x * 0.9f;
    }
    if (b->damage_out < 0.0f)
        b->damage_out = 0.0f;
}

// @ 0x4552c0
void Agent::draw_name()
{
    Mission* ms = g_mission;
    unsigned local = ms->local_id;
    unsigned col = 0xffffffff;
    float origin[3];
    origin[0] = 0.0f; origin[1] = 0.0f; origin[2] = 0.0f;
    vec2 scale;
    scale.x = 0.14f; scale.y = 0.14f;
    float a = -1.0f;
    float bb = 1.0f;
    if ((int)local >= 0) {
        if (team == ms->pages[local >> 4][local & 0xf].team)
            col = 0xff44ff44;
        else if ((int)local >= 0 && ms->pages[local >> 4][local & 0xf].is_friend(this))
            col = 0xff4444ff;
    }
    if (f10 == 0 && id != (int)local) {
        unsigned fl = g_rec58[ms->cur_rec].flags;
        if (((fl >> 1) & 1) && (int)local >= 0 && ms->pages[local >> 4][local & 0xf].is_friend(this)) {
            char buf[512];
            tstring* nm = g_game->chars[troop].display_name();
            sprintf(buf, "%s", nm->c_str());
            if (f14 != 1) {
                fx->scale = 0.3f;
                if (state == 3)
                    buf[0] = 0;
            } else if (f2c >= 0)
                fx->scale = 2.65f;
            else
                fx->scale = 1.88f;
            unsigned colp = col;
            vector4 ext;
            tstring text(buf);
            g_font->layout_clear(&ext, &text, (vector4*)origin, &scale, *fx->pmesh, &colp, &a, &bb, 0x10);
        }
    }
}

// @ 0x4554c0
void Agent::spawn_corpse()
{
    floor = 0;
    vector4 saved = apos;
    f56e8 = 0;
    int i = 0;
    {
        float z;
        face_shape** pf = &floor;
        do {
            if (i >= 100)
                break;
            i++;
            if (i == 30 || i == 60)
                apos = saved;
            g_mission->scene->floor_at(&apos.x, &z, pf);
            if (*pf != 0)
                break;
            apos.x = ((float)(rand() % 0x3dc9) * 6.322312e-05f - 0.5f) * 0.01f + apos.x;
            apos.y = ((float)(rand() % 0x3dc9) * 6.322312e-05f - 0.5f) * 0.01f + apos.y;
        } while (*pf == 0);
        apos.z = z;
    }
    int r;
    float fv;
    r = rand() % 0x3dc9;
    fv = (float)r * 6.322312e-05f - 0.5f;
    spin[0] = fv + fv;
    r = rand() % 0x3dc9;
    fv = (float)r * 6.322312e-05f - 0.5f;
    spin[1] = fv + fv;
    r = rand() % 0x3dc9;
    fv = (float)r * 6.322312e-05f - 0.5f;
    spin[2] = fv + fv;
    r = rand() % 0x3dc9;
    f290 = 5.0f;
    spawn = apos;
    fv = (float)r * 6.322312e-05f - 0.5f;
    spin[3] = fv + fv;
    ragdoll_update();
    f34 = 1;
    if ((g_game->chars[troop].f30 & 0xf) == 1)
        f34 = 0;
    f38 = 9;
}
