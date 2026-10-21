// Slice s00457b40: Agent::process_blow (provisional name), 0x457b40, the handler that consumes a pending blow
// (hit) on an agent: damage scaling by difficulty, UI damage messages, hit sounds, arrow-stuck meshes, blood, the
// horse falling / human death-or-knockout decision, knockdown animations and impulses, rider dismount, XP and
// weapon proficiency awards for the attacker and the kill / knock-out bookkeeping and messages.
#include "s00457b40.h"

// the register-convention helpers of neighbouring slices, defined here as statics so cl gives them the same
// convention (see s00453710, s0043a280)
// @ 0x4350c0 (copy of s00434ad0; not part of this slice)
static __declspec(noinline) int slot_class_impl(int item, int flag)
{
    unsigned t = g_items[item].type_flags & 0xff;
    if (t == 8) return 3;
    if (t == 9) return 4;
    if (t == 10) return 5;
    if (t == 0x10 || t == 0x11) return 6;
    if (t == 4) return 2;
    if (t == 2) return 0;
    return flag == 0;
}
// @ 0x43a280 (copy of s0043a280; not part of this slice)
// (register convention: type in EAX, destination buffer in ESI)
static __declspec(noinline) void ui_weapon_class_name(int type, char* out)
{
    if (type == 0) {
        tstring k("ui_one_handed_weapons");
        const char* s = g_strtable.lookup(k)->c_str();
        { char* d = out; char c; do { c = *s++; *d++ = c; } while (c != 0); }
    } else if (type == 1) {
        tstring k("ui_two_handed_weapons");
        const char* s = g_strtable.lookup(k)->c_str();
        { char* d = out; char c; do { c = *s++; *d++ = c; } while (c != 0); }
    } else if (type == 2) {
        tstring k("ui_polearms");
        const char* s = g_strtable.lookup(k)->c_str();
        { char* d = out; char c; do { c = *s++; *d++ = c; } while (c != 0); }
    } else if (type == 3) {
        tstring k("ui_archery");
        const char* s = g_strtable.lookup(k)->c_str();
        { char* d = out; char c; do { c = *s++; *d++ = c; } while (c != 0); }
    } else if (type == 4) {
        tstring k("ui_crossbows");
        const char* s = g_strtable.lookup(k)->c_str();
        { char* d = out; char c; do { c = *s++; *d++ = c; } while (c != 0); }
    } else if (type == 5) {
        tstring k("ui_throwing");
        const char* s = g_strtable.lookup(k)->c_str();
        { char* d = out; char c; do { c = *s++; *d++ = c; } while (c != 0); }
    } else if (type == 6) {
        tstring k("ui_firearms");
        const char* s = g_strtable.lookup(k)->c_str();
        { char* d = out; char c; do { c = *s++; *d++ = c; } while (c != 0); }
    }
}

#define PAGE_AGENT(m, i) ((m)->tbl.pages[(unsigned)(i) >> 4] + ((i) & 0xf))

// @ 0x457b40
void Agent::process_blow(int unused)
{
    int attacker;
    float dmg;
    int hit_part;
    int anim_a, anim_b;
    int atk_troop, atk_rider;
    Mission* m;
    bool stunned;
    bool cA, cB;
    vector4 imp;
    ItemKind* ik;

    if (f14 != 1)
        return;
    if (blow_pending == 0)
        return;
    attacker = blow_attacker;
    dmg = blow_damage;
    hit_part = blow_type;
    blow_pending = 0;
    anim_a = -1;
    anim_b = -1;
    if (attacker >= 0)
        last_attacker = attacker;
    m = g_mission;
    atk_troop = -1;
    atk_rider = -1;
    if (attacker >= 0) {
        Agent** pg = m->tbl.pages;
        int r = attacker;
        if (pg[(unsigned)attacker >> 4][attacker & 0xf].f10 != 0) {
            if (pg[(unsigned)attacker >> 4][attacker & 0xf].rider_id < 0)
                goto after_rider;
            r = m->tbl.get(attacker)->rider_id;
        }
        atk_rider = r;
        if (r >= 0)
            atk_troop = pg[(unsigned)r >> 4][r & 0xf].troop;
    }
after_rider:
    if ((f10 == 0 && blow_bone == 9) || (f10 == 1 && blow_bone == 7)) {
        if (blow_item >= 0 && g_items[blow_item].is_ranged()) {
            dmg = dmg * 1.75f;
            if (attacker == m->local_id && g_opt_damage_msgs != 0) {
                tstring k("ui_head_shot");
                g_msglog.add(*g_strtable.lookup(k), 0xffffdd11);
                m = g_mission;
            }
        }
    }
    if (id == m->local_id || rider_id == m->local_id) {
        if (g_opt_dmg_to_player == 1)
            dmg = dmg * 0.5f;
        else if (g_opt_dmg_to_player == 0)
            dmg = dmg * 0.25f;
    } else {
        int dd = g_opt_dmg_to_friends;
        GameData* g = g_game;
        if (dd != 2 &&
            ((f10 == 0 && f1bc == g->main_party) ||
             (f10 == 1 && rider_id >= 0 && m->tbl.get(rider_id)->f1bc == g->main_party))) {
            if (dd == 1)
                dmg = dmg * 0.75f;
            else if (dd == 0)
                dmg = dmg * 0.5f;
        }
    }
    hp = hp - dmg;

    // ---- the damage messages ----
    {
        int shown = g_opt_damage_msgs;
        if (blow_attacker == m->local_id && shown != 0) {
            char msgbuf[512];
            {
                tstring k((f10 != 0 && rider_id != -1) ? "ui_delivered_number_damage_to_horse"
                                                        : "ui_delivered_number_damage");
                sprintf(msgbuf, g_strtable.lookup(k)->c_str(), round_float(&dmg));
            }
            {
                tstring t(msgbuf);
                g_msglog.add(t, -261);
            }
        } else {
            int lid = m->local_id;
            if (lid >= 0 && m->tbl.pages[(unsigned)lid >> 4][lid & 0xf].mount_id >= 0 &&
                blow_attacker == m->tbl.pages[(unsigned)lid >> 4][lid & 0xf].mount_id && shown != 0 &&
                round_float(&dmg) != 0) {
                char hbuf[256];
                {
                    tstring k("ui_horse_charged_for_number_damage");
                    sprintf(hbuf, g_strtable.lookup(k)->c_str(), round_float(&dmg));
                }
                {
                    tstring t(hbuf);
                    g_mission->log_timed(&t, 20.0f);
                }
            } else {
                const char* key = 0;
                shown = g_opt_damage_msgs;
                m = g_mission;
                if (id == m->local_id && shown != 0) {
                    key = "ui_received_number_damage";
                } else if (m->local_id >= 0) {
                    int l2 = m->local_id;
                    if (m->tbl.pages[(unsigned)l2 >> 4][l2 & 0xf].mount_id >= 0 &&
                        id == m->tbl.pages[(unsigned)l2 >> 4][l2 & 0xf].mount_id && shown != 0)
                        key = "ui_horse_received_number_damage";
                }
                if (key) {
                    char hbuf[256];
                    {
                        tstring k(key);
                        sprintf(hbuf, g_strtable.lookup(k)->c_str(), round_float(&dmg));
                    }
                    g_msglog.add(tstring(hbuf), 0xffe0c0c0);
                }
            }
        }
    }

    // ---- hit sounds ----
    if (!(blow_flags & 0x20)) {
        int kind;
        float var;
        if (blow_item < 0) {
            var = 0.15f;
            kind = 0x65;
        } else if (g_items[blow_item].is_ranged()) {
            var = 0.1f;
            kind = (id == g_mission->local_id) ? 0x66 : 0x64;
        } else {
            bool blunt = (blow_item_flags & itp_wooden_attack) != 0 || blow_dmg_type == 2;
            var = 0.15f;
            if (dmg < 9.0f) {
                if (hit_part < 14)
                    kind = blunt ? 0x79 : 0x75;
                else
                    kind = blunt ? 0x7b : 0x77;
            } else {
                if (hit_part < 14)
                    kind = blunt ? 0x7a : 0x76;
                else
                    kind = blunt ? 0x7c : 0x78;
            }
        }
        play_sound_var(&kind, &var);
    }

    // ---- a missile that sticks in the body ----
    if (blow_weapon != 0 && g_items[blow_weapon->item_b].capability_flags != itcf_throw_stone &&
        !(g_mission->flags & 1) && dmg > 7.0f && (hp < 1.0f || blow_bone != 9 || dmg > 75.0f)) {
        mat4 M = body->m * *(mat4*)body->skel->rec(blow_bone);
        float len = blow_weapon->stuck_length() * 0.7f;
        matrix mm;
        mm.o = vector4(blow_pos.x - len * blow_dir.x, blow_pos.y - len * blow_dir.y, blow_pos.z - len * blow_dir.z);
        mm.f = blow_dir;
        mm.u = vector4(0.0f, 0.0f, 1.0f);
        mm.orthonormalize();
        if (blow_weapon != 0) {
            mesh* ms = blow_weapon->make_ranged_mesh();
            if (ms != 0) {
                vector4 off;
                off.x = 0.0f;
                off.y = blow_weapon->stuck_length();
                off.z = 0.0f;
                ms->translate(&off);
                body->skel->attach_mesh(blow_bone, ms, ((::pos*)&M)->relative(mm));
            }
        }
    }

    // ---- blood ----
    if (dmg > 1.0f && !(g_mission->flags & 0x100)) {
        float blood = 1.0f;
        if (blow_dmg_type == 2)
            blood = 0.2f;
        else if (blow_dmg_type == 1)
            blood = 0.7f;
        if (g_opt_blood > 0 && blow_dmg_type != 2) {
            bool go = false;
            if (g_opt_blood == 2) {
                go = true;
            } else if (g_mission->local_id >= 0) {
                if ((blow_pos - g_mission->tbl.get(g_mission->local_id)->pos).out_length_squared() < 70.0f)
                    go = true;
            }
            if (go)
                g_mission->blood_burst(blow_pos, blood * dmg);
        }
    }

    if (f10 == 1) {
        float plen;
        if (hp < 1.0f) {
            hp = 0.0f;
            f14 = 5;
            f5830 = 3;
            tick_stamp = g_ticks[tick_idx];
            body->skel->pad68 = 1;
            fn_4552c0();
            m = g_mission;
            if (rider_id == m->local_id && id == m->mount_id && (g_rec58[m->cur_rec].flags >> 1 & 1) &&
                base_item.item >= 0 && !(g_items[base_item.item].type_flags >> 12 & 1)) {
                GameData* g = g_game;
                ItemRef* horse = &g->chars[g->player].items[8];
                if ((horse->m4 & 0xff000000) == (unsigned)imod_lame << 24) {
                    if (frand() < 0.2f) {
                        tstring k("ui_horse_fell_dead");
                        g_msglog.add(*g_strtable.lookup(k), 0xffdd6655);
                        horse->item = -1;
                    }
                } else {
                    if (frand() < 0.2f) {
                        {
                            tstring k("ui_horse_crippled");
                            g_msglog.add(*g_strtable.lookup(k), 0xffeebbbb);
                        }
                        horse->b.mod = 0;
                        horse->m4 |= (unsigned)imod_lame << 24;
                        horse->reduce(0);
                    }
                }
            }
            plen = 1.0f;
            if (ac > 2.5f) {
                set_order(0, 0x1a4, 0x2000080, 1.0f);
                int kind = 4;
                play_hit_sound(&kind);
                timer.set_delay(-2.4f);
                if (rider_id >= 0) {
                    g_mission->tbl.get(rider_id)->set_order(0, 0x123, 0x1000081, 1.0f);
                    vector4 z;
                    z.x = 0.0f;
                    z.y = 0.0f;
                    z.z = 0.0f;
                    g_mission->tbl.get(rider_id)->impulse = z;
                    g_mission->tbl.get(rider_id)->timer.set_delay(-2.4f);
                }
            } else {
                if (frand() <= 1.0f) {
                    set_order(0, 0x1a3, 0x2000080, 1.0f);
                    int kind = 4;
                    play_hit_sound(&kind);
                    plen = g_anims[0x1a3].info->length;
                    if (rider_id >= 0)
                        g_mission->tbl.get(rider_id)->set_order(0, 0x122, 0x1000081, 1.0f);
                } else {
                    set_order(0, 0x1a2, 0x2000080, 1.0f);
                    plen = g_anims[0x1a2].info->length;
                    int kind = 4;
                    play_hit_sound(&kind);
                    if (rider_id >= 0)
                        g_mission->tbl.get(rider_id)->set_order(0, 0x121, 0x1000081, 1.0f);
                }
                if (plen < 0.01f)
                    plen = 1.0f;
            }
            timer5918.set_delay(-plen);
            if (rider_id >= 0) {
                g_mission->tbl.get(rider_id)->mount_id = -1;
                rider_id = -1;
            }
            return;
        }
        // the horse survived: a lance couched into it may stagger it
        // the horse survived: a lance couched into it may stagger it
        if (blow_item < 0) return;
        if ((g_items[blow_item].type_flags & 0xff) != itp_type_polearm) return;
        if (blow_dmg_type != 1) return;
        if (!(blow_damage > 8.0f)) return;
        if (!(ac > 0.1f)) return;
        if (blow_attacker < 0) return;
        if (!(rgl::dot(pos - m->tbl.get(blow_attacker)->pos, body->m.f) < -0.5f)) return;
        if ((int)(fe8 & 0x1ffff) >= 0x4a) return;
        float a = 5.0f;
        float x = 5.0f / ((float)base_item.stat_214() + 2.0f);
        float y = fmax_ref(&ac, &a);
        x = (x / y) * 5.0f;
        if (frand() < x) {
            int kind = 4;
            play_hit_sound(&kind);
            set_order(0, 0x19c, 0x100004a, 1.0f);
            return;
        }
        a = 0.95f;
        float z = (float)base_item.stat_214() * 0.01f + 0.7f;
        ac = fmin_ref(&z, &a) * ac;
        return;
    }

    // ---- a human ----
    stunned = false;
    hit_part = 0x80;
    if (hp < 1.0f) {
        vector4 apos;
        apos.x = 0.0f;
        apos.y = 0.0f;
        apos.z = 0.0f;
        if (blow_attacker >= 0)
            apos = g_mission->tbl.get(blow_attacker)->pos;
        if (mount_id != -1) {
            anim_a = 0x120;
        } else {
            float vx = apos.x - pos.x;
            float vy = apos.y - pos.y;
            float vz = apos.z - pos.z;
            if (vx * dir.x + (vy * dir.y + vz * dir.z) > 0.0f) {
                if (blow_bone == 9 && (blow_weapon != 0 || frand() < 0.7f)) {
                    anim_a = 0x11a;
                } else if (blow_bone == 9) {
                    anim_a = 0x11d;
                } else if (blow_bone == 0 || blow_bone == 7) {
                    anim_a = 0x11c;
                } else if (blow_weapon == 0 && rgl::dot(blow_vec, body->m.s) > 0.3f) {
                    anim_a = 0x11e;
                } else {
                    anim_a = 0x11b;
                }
            } else {
                anim_a = 0x11f;
            }
        }
        goto common;
    }
    if (troop >= 0) {
        if (dmg < 1.0f) {
            float r = (frand() + 1.0f) * (float)(g_game->chars[troop].strength + 15) * 0.2f;
            if (!(blow_force < r)) goto bee;
        } else {
        bee:
            if (!(dmg < g_k_a18178)) goto c25;
        }
        if (blow_attacker >= 0 && g_mission->tbl.get(blow_attacker)->f10 == 0)
            stunned = true;
    }
c25:
    if (mount_id == -1 && blow_item >= 0 &&
        (ik = &g_items[blow_item], ik->is_ranged() || (ik->type_flags & 0xff) == itp_type_polearm)) {
        if (stunned)
            goto common;
        if (((ik->type_flags & 0xff) == itp_type_polearm && frand() < 0.5f && blow_force > 15.0f) ||
            ((g_items[blow_item].type_flags & 0xff) == itp_type_thrown && frand() < 0.4f && blow_force > 15.0f) ||
            ((g_items[blow_item].type_flags & 0xff) == itp_type_bow && frand() < 0.3f && blow_force > 25.0f) ||
            ((g_items[blow_item].type_flags & 0xff) == itp_type_crossbow && frand() < 0.3f && blow_force > 25.0f)) {
            anim_a = move_anim(2);
            float k = frand() * 0.2f + 2.1f;
            hit_part = 0x82;
            imp.x = k * blow_dir.x;
            imp.y = k * blow_dir.y;
            imp.z = k * blow_dir.z;
            goto impulse;
        }
        anim_a = move_anim(1);
        goto common;
    }
    if (mount_id != -1)
        goto h_mounted;
ded:
    {
        float d = rgl::dot(dir, blow_dir);
        if (!(d < 0.0f))
            goto dfail;
    }
    if (!(((blow_flags & 0x10) && blow_force > 7.0f) || (blow_force > 24.0f && frand() < 0.4f)))
        goto dfail;
    if (blow_dmg_type != 2)
        goto dfail;
    if (action == 0x125)
        goto dfail;
    anim_a = 0x125;
    imp.x = dir.x * -0.6f;
    anim_b = 0x126;
    imp.y = dir.y * -0.6f;
    imp.z = dir.z * -0.6f;
    if (blow_flags & 0x10) {
        float rr = frand() * 0.4f + 1.5f;
        imp.x = rr * blow_dir.x + imp.x;
        imp.y = rr * blow_dir.y + imp.y;
        imp.z = rr * blow_dir.z + imp.z;
    }
ded_tail:
    if (stunned)
        goto common;
impulse:
    if (flags3c & 1) {
        flags3c &= ~1u;
        timer.set_delay(-0.6f);
        impulse.x = imp.x + impulse.x;
        impulse.y = imp.y + impulse.y;
        impulse.z = imp.z + impulse.z;
        if (blow_attacker >= 0 && g_mission->tbl.get(blow_attacker)->f10 == 1) {
            impulse.x = impulse.x;
            impulse.y = impulse.y;
            impulse.z = impulse.z + 1.0f;
        }
    }

common:
    cA = false;
    cB = false;
    if (hp < 1.0f) {
        hp = 0.0f;
        m = g_mission;
        float surg = 0.0f;
        if (m->local_id >= 0) {
            if (m->tbl.get(m->local_id)->is_friend(this)) {
                surg = (float)g_game->parties.get(g_game->main_party)->skill(10) * 0.04f + 0.25f;
                m = g_mission;
            }
        }
        if (m->flags & 1)
            goto set_cB;
        if ((g_game->chars[troop].flags & tf_allways_fall_dead) == 0) {
            if (blow_dmg_type == 2)
                goto set_cB;
            if (troop < 0)
                goto set_cB;
            if (g_game->chars[troop].has_flag10())
                goto set_cB;
            if (!(frand() > surg))
                goto set_cB;
        }
        cA = true;
        goto ab_done;
    dfail:
        anim_b = move_anim(1);
        if (!(blow_flags & 0x10)) {
            if (!(blow_force > 15.0f))
                goto common;
            if (!(frand() < 0.3f))
                goto common;
        }
        {
            float e = (frand() - 0.5f) * 0.5f;
            float f = (frand() + 0.6f) * (blow_force + 30.0f) * 0.02f;
            vector4* sv = &body->m.s;
            imp.x = f * blow_dir.x + e * sv->x;
            imp.y = f * blow_dir.y + e * sv->y;
            imp.z = f * blow_dir.z + e * sv->z;
        }
        goto ded_tail;
    h_mounted:
        anim_b = move_anim(0);
        goto common;
    set_cB:
        cB = true;
    ab_done:
        f5830 = 3;
        tick_stamp = g_ticks[tick_idx];
        f5838 = mount_id;
        body->skel->pad68 = 1;
        float len = 0.5f;
        if (anim_a >= 0) {
            len = g_anims[anim_a].info->length;
            if (len < 0.01f)
                len = 0.5f;
        }
        timer5918.set_delay(-len);
        hit_bone = blow_bone;
        hit_vec = blow_vec;
        hit_vec.normalize();
        float k = (blow_force + 5.0f) * 0.2f;
        hit_vec.x = k * hit_vec.x;
        hit_vec.y = k * hit_vec.y;
        hit_vec.z = k * hit_vec.z;
        hit_pos = blow_pos;
        if (blow_flags & 0x10) {
            hit_vec.x = hit_vec.x + hit_vec.x;
            hit_vec.y = hit_vec.y + hit_vec.y;
            hit_vec.z = hit_vec.z + hit_vec.z;
        }
        if (blow_dmg_type == 2) {
            hit_vec.x = hit_vec.x + hit_vec.x;
            hit_vec.y = hit_vec.y + hit_vec.y;
            hit_vec.z = hit_vec.z + hit_vec.z;
        }
        fn_4552c0();
        m = g_mission;
        if (mount_id >= 0) {
            if (m->local_id < 0 || m->tbl.get(m->local_id)->mount_id != -1 ||
                ((blow_attacker != m->local_id || m->dismount_count >= 4) && !(m->flags & 1))) {
                m->tbl.get(mount_id)->f5844 = 1;
                Agent* h = g_mission->tbl.get(mount_id);
                h->tick_stamp = g_ticks[h->tick_idx];
            } else {
                Agent* h = m->tbl.get(mount_id);
                vector4 bf;
                bf.x = h->ac * body->m.f.x;
                bf.y = body->m.f.y * h->ac;
                bf.z = body->m.f.z * h->ac;
                timer.set_delay(-1.4f);
                impulse = bf;
                g_mission->tbl.get(mount_id)->rider_id = -1;
                mount_id = -1;
                g_mission->dismount_count++;
            }
        }
    }

    // ---- experience and weapon proficiency for the attacker ----
    if (atk_troop >= 0 && this->is_enemy(PAGE_AGENT(g_mission, atk_rider)) && dmg > 0.01f && (cA || cB)) {
        CharRec* chars = g_game->chars;
        float xpf = (float)chars[troop].scaled_b0();
        if (g_mission->flags & 1)
            xpf = xpf * 0.1f;
        int xp = (int)xpf;
        if (xp != 0) {
            if (chars[g_mission->tbl.get(atk_rider)->troop].has_flag10()) {
                GameData* g = g_game;
                if (atk_troop == g->player)
                    g->add_experience(atk_troop, (int)((float)xp * g_k_a1812c));
                else
                    g->add_experience(atk_troop, (int)((float)xp * g_k_a18130));
            } else {
                g_mission->tbl.get(atk_rider)->xp_pool += (int)((float)xp * g_k_a18134);
            }
        }
    }
    if (blow_attacker >= 0 && dmg > 0.01f) {
        AgentTable* t = &g_mission->tbl;
        if (this->is_enemy(t->get(blow_attacker)) && t->get(blow_attacker)->troop >= 0 &&
            g_game->chars[t->get(blow_attacker)->troop].has_flag10() && blow_item >= 0) {
            ItemRef sp = t->get(blow_attacker)->slot_pair(1);
            int cls = slot_class_impl(blow_item, sp.item >= 0);
            int old = (int)g_game->chars[atk_troop].prof[cls];
            float dm = dmg;
            if (g_mission->flags & 1)
                dm = dm * 0.33f;
            float amount = (((float)g_game->chars[troop].level + 10.0f) * dm) * 0.05f;
            vector4 vA = v56b4 - t->get(blow_attacker)->v56b4;
            vector4 vB = pos - t->get(blow_attacker)->pos;
            float lenB = vB.normalize();
            float lenA = vA.normalize();
            float cl = rgl::cross(vA, vB).out_length();
            char msgbuf[512];
            char namebuf[256];
            float w;
            if (cls == 3 || cls == 5 || cls == 4 || cls == 6) {
                float tt = ((cl * lenA + 4.0f) * 0.25f) * ((lenB + 4.0f) * 0.25f) * 0.3f;
                if (tt < 1.0f)
                    tt = 1.0f;
                else if (tt > 12.0f)
                    tt = 12.0f;
                if (blow_attacker == g_mission->local_id && g_opt_shot_difficulty != 0) {
                    float sd = tt;
                    if (blow_bone == 9)
                        sd = tt * 1.2f;
                    {
                        tstring k("ui_shot_difficulty");
                        sprintf(msgbuf, g_strtable.lookup(k)->c_str(), (double)sd);
                    }
                    {
                        g_msglog.add(tstring(msgbuf), 0xffaaaaaa);
                    }
                }
                w = tt * amount;
            } else {
                w = amount;
            }
            g_game->chars[atk_troop].adjust(cls, w * 0.02f);
            int nw = (int)g_game->chars[atk_troop].prof[cls];
            if (atk_troop == 0 && nw > old) {
                ui_weapon_class_name(cls, namebuf);
                int diff = nw - old;
                if (diff == 1) {
                    {
                        tstring k("ui_you_have_improved_your_proficiency_in_value_to_number");
                        sprintf(msgbuf, g_strtable.lookup(k)->c_str(), namebuf, nw);
                    }
                } else {
                    tstring k("ui_your_proficiency_in_value_has_improved_by_number_to_number");
                    sprintf(msgbuf, g_strtable.lookup(k)->c_str(), namebuf, diff, nw);
                }
                g_msglog.add(tstring(msgbuf), 0xffffdf44);
            }
        }
    }

    // ---- the impact particles ----
    if (dmg > 1.0f && !(g_mission->flags & 1) && g_opt_hit_fx != 0) {
        float hi = 50.0f;
        float lo = 1.0f;
        float k = ((dmg + 5.0f) * g_k_a1811c) * 0.1f + 1.0f;
        float ef = clamp_float(&k, &lo, &hi);
        matrix m2;
        m2.o = blow_pos;
        m2.f = vector4(-blow_dir.x, -blow_dir.y, -blow_dir.z + 0.5f);
        m2.u = vector4(0.0f, 0.0f, 1.0f);
        m2.orthonormalize();
        int vi = (int)(g_game->chars[troop].flags & 0xf);
        int e1 = g_voices.get_f1f0(vi);
        int e2 = g_voices.get_f1f4(vi);
        if (e1 == 0)
            e1 = g_voice_default_a;
        if (e2 == 0)
            e2 = g_voice_default_b;
        if (e1 >= 0) {
            Emit* em = g_mission->spawn_effect(e1, (const mat4*)&m2, (int)(ef + ef));
            float s = (ef + 10.0f) * 0.1f;
            vector4 v;
            v.x = s * g_emit_defs.defs[e1].a4;
            v.y = s * g_emit_defs.defs[e1].a8;
            v.z = s * g_emit_defs.defs[e1].ac;
            em->set_v184(&v);
            em->f198 = ef * g_emit_defs.defs[e1].f24;
        }
        if (e2 >= 0) {
            Emit* em = g_mission->spawn_effect(e2, (const mat4*)&m2, (int)(ef * 4.0f));
            float s = (ef + 10.0f) * 0.1f;
            vector4 v;
            v.x = s * g_emit_defs.defs[e2].a4;
            v.y = s * g_emit_defs.defs[e2].a8;
            v.z = s * g_emit_defs.defs[e2].ac;
            em->set_v184(&v);
            em->f198 = ef * g_emit_defs.defs[e2].f24;
        }
    }

    // ---- the outcome ----
    if (cA) {
        if (!(blow_flags & 0x20)) {
            int kind = 0;
            play_class_sound(&kind);
        }
        set_order(0, anim_a, 0x28000c8, 1.0f);
        f14 = 5;
        m = g_mission;
        if (blow_attacker == m->local_id) {
            g_game->cnt_killed++;
            if (this->is_friend(m->tbl.get(g_mission->local_id)))
                g_game->cnt_killed_friend++;
            m = g_mission;
        }
        if (blow_attacker >= 0)
            m->tbl.get(blow_attacker)->cnt_killed++;
        if (g_opt_kill_msgs != 0) {
            char msgbuf[512];
            tstring unused_s;
            if (atk_troop >= 0) {
                tstring k("ui_value_killed_by_value");
                const char* n1 = g_game->chars[atk_troop].display_name()->c_str();
                const char* n2 = g_game->chars[troop].display_name()->c_str();
                sprintf(msgbuf, g_strtable.lookup(k)->c_str(), n2, n1);
            } else {
                tstring k("ui_value_fell_dead");
                const char* n2 = g_game->chars[troop].display_name()->c_str();
                sprintf(msgbuf, g_strtable.lookup(k)->c_str(), n2);
            }
            m = g_mission;
            int lid = m->local_id;
            int color = 0xff44ddaa;
            if (lid >= 0) {
                if (m->tbl.get(lid)->team == team)
                    color = 0xffcc4411;
                else if (lid >= 0 && m->tbl.get(lid)->is_friend(this))
                    color = 0xffce1180;
            }
            g_msglog.add(tstring(msgbuf), color);
        }
        fn_4552c0();
        return;
    }
    if (cB) {
        if (!(blow_flags & 0x20)) {
            int kind = 0;
            play_class_sound(&kind);
        }
        set_order(0, anim_a, 0x28000c8, 1.0f);
        f14 = 4;
        m = g_mission;
        if (blow_attacker == m->local_id) {
            g_game->cnt_ko++;
            if (this->is_friend(m->tbl.get(g_mission->local_id)))
                g_game->cnt_ko_friend++;
            m = g_mission;
        }
        if (blow_attacker >= 0)
            m->tbl.get(blow_attacker)->cnt_ko++;
        if (g_opt_kill_msgs != 0) {
            char namebuf[256];
            if (atk_troop >= 0) {
                tstring k("ui_value_knocked_unconscious_by_value");
                const char* n1 = g_game->chars[atk_troop].display_name()->c_str();
                const char* n2 = g_game->chars[troop].display_name()->c_str();
                sprintf(namebuf, g_strtable.lookup(k)->c_str(), n2, n1);
            } else {
                tstring k("ui_value_fell_unconscious");
                const char* n2 = g_game->chars[troop].display_name()->c_str();
                sprintf(namebuf, g_strtable.lookup(k)->c_str(), n2);
            }
            m = g_mission;
            int lid = m->local_id;
            int color = 0xff44ddaa;
            if (lid >= 0) {
                if (m->tbl.get(lid)->team == team)
                    color = 0xffcc9911;
                else if (lid >= 0 && m->tbl.get(lid)->is_friend(this))
                    color = 0xffb26fdd;
            }
            g_msglog.add(tstring(namebuf), color);
        }
        fn_4552c0();
        return;
    }
    if (stunned)
        return;
    if (!(blow_flags & 0x20)) {
        int kind = 1;
        play_class_sound(&kind);
    }
    if (anim_a >= 0)
        set_order(0, anim_a, hit_part | 0x5800000, 1.0f);
    if (anim_b >= 0)
        set_order(1, anim_b, hit_part | 0x5800000, 1.0f);
}
