// Slice s004543a0: Agent::ragdoll_update (0x4543a0, 2698 bytes; name provisional): per-frame update of an agent's body.
// While the agent is a Newton ragdoll it reads every bone's global matrix into a local table, rebuilds the bone-local
// frames, plays the blood sound when a hand/foot bone touches the floor, rebuilds the static collision when the body has
// moved more than 3 m, and drags the ragdoll along with a moving mount. Otherwise it orients the body frame from the
// agent's direction, blended towards the floor normal. Finally it sets the camera parameters and the body mode.
#include "agent_life.h"

struct NewtonBody; struct NewtonRagDollBone;
extern "C" {
__declspec(dllimport) void __cdecl NewtonBodyGetMatrix(const NewtonBody* b, float* m);                           // 0x54920c
__declspec(dllimport) void __cdecl NewtonBodyGetVelocity(const NewtonBody* b, float* v);                         // 0x549210
__declspec(dllimport) void __cdecl NewtonBodySetVelocity(const NewtonBody* b, const float* v);                   // 0x549214
__declspec(dllimport) void __cdecl NewtonBodySetMatrix(const NewtonBody* b, const float* m);                     // 0x549278
__declspec(dllimport) void __cdecl NewtonRagDollBoneGetGlobalMatrix(const NewtonRagDollBone* b, float* m);       // 0x549248
__declspec(dllimport) NewtonBody* __cdecl NewtonRagDollBoneGetBody(const NewtonRagDollBone* b);                  // 0x549294
}

// @ 0x4543a0
void Agent::ragdoll_update()
{
    int i = 0;
    if (body == 0)
        return;
    if (ragdoll != 0) {
        SkelData* data = body->skel->data;
        matrix world[0x20];
        if (0 < body->skel->count) {
            do {
                if (i == 0) {
                    mat4 g;
                    NewtonRagDollBoneGetGlobalMatrix(body->skel->elems[0].bone, (float*)&g);
                    mat4 chk = g;
                    if (!(-1e6f <= chk.o.x) || !(chk.o.x < 1e6f)) {
                        on_removed();
                        body->skel->has_poses = 0;
                        break;
                    }
                    body->m = g;
                    mat4 id0;
                    body->skel->elems[0].m44 = *(mat4*)identity_pos((pos*)&id0);
                    *(mat4*)&world[0] = g;
                } else {
                    SkelElem* e = &body->skel->elems[i];
                    mat4 res;
                    mat4* dst;
                    if (e->bone == 0) {
                        res = *(mat4*)&world[data->items[i].parent] * e->m44;
                        dst = (mat4*)&world[i];
                    } else {
                        mat4 g;
                        NewtonRagDollBoneGetGlobalMatrix(e->bone, (float*)&g);
                        mat4* w = (mat4*)&world[i];
                        *w = g;
                        if ((f10 == 0 && (i == 8 || i == 10 || i == 15)) || (f10 == 1 && i == 2)) {
                            float lim = 0.15f;
                            if (f10 == 1)
                                lim = 0.4f;
                            else if (i == 8)
                                lim = 0.35f;
                            if (!f58c4 && !f58d8 && hold.o.z - w->o.z < 0.0f &&
                                w->o.z - g_mission->scene->floor_z(&w->o.x) < lim) {
                                g_sounds.play(g_sound_blood, 0, (vector4*)&body->m.o, frand() * 0.4f + 0.8f, 0);
                                f58d8 = 1;
                            }
                            hold = *w;
                        }
                        res = *(mat4*)&((pos&)world[data->items[i].parent]).relative(world[i]);
                        dst = &body->skel->elems[i].m44;
                    }
                    *dst = res;
                }
                i++;
            } while (i < body->skel->count);
        }
        float dx = body->m.o.x - anchor[0];
        float dy = body->m.o.y - anchor[1];
        float dz = body->m.o.z - anchor[2];
        if (9.0f < dx * dx + (dy * dy + dz * dz))
            build_static_collision();
        if ((int)mount_id >= 0) {
            unsigned mid = mount_id;
            if (g_mission->pages[mid >> 4][mid & 0xf].body != 0) {
                mat4 m1, m2, mm;
                NewtonBodyGetMatrix(mount_body, (float*)&m1);
                NewtonBodyGetMatrix(NewtonRagDollBoneGetBody(body->skel->elems[0].bone), (float*)&m2);
                AgentBody* mb = g_mission->pages[mount_id >> 4][mount_id & 0xf].body;
                mm = mb->m * mb->skel->elems[0].m84;
                NewtonBodySetMatrix(mount_body, (float*)&mm);
                if (0.0f < m2.o.z - m1.o.z) {
                    int j = 0;
                    if (0 < body->skel->count) {
                        int off = 0;
                        do {
                            NewtonRagDollBone* b = *(NewtonRagDollBone**)((char*)body->skel->elems + 0x1d8 + off);
                            if (b != 0) {
                                NewtonBody* nb = NewtonRagDollBoneGetBody(b);
                                NewtonBodyGetMatrix(nb, (float*)&m2);
                                float z = mm.o.z + m2.o.z;
                                float x = (mm.o.x + m2.o.x) - m1.o.x;
                                float y = (mm.o.y + m2.o.y) - m1.o.y;
                                float zz = z - m1.o.z;
                                *(vector4*)&m2.o = vector4(x, y, zz);
                                NewtonBodySetMatrix(nb, (float*)&m2);
                            }
                            off += 0x1dc;
                            j++;
                        } while (j < body->skel->count);
                    }
                    f58c4 = 0;
                } else if (f58c5 != 0) {
                    f58c5 = 0;
                    float* pv = &g_mission->pages[mount_id >> 4][mount_id & 0xf].vel.x;
                    int j = 0;
                    if (0 < body->skel->count) {
                        int off = 0;
                        do {
                            NewtonRagDollBone* b = *(NewtonRagDollBone**)((char*)body->skel->elems + 0x1d8 + off);
                            if (b != 0) {
                                NewtonBody* nb = NewtonRagDollBoneGetBody(b);
                                float v[4];
                                NewtonBodyGetVelocity(nb, v);
                                v[0] = v[0] + pv[0];
                                v[1] = v[1] + pv[1];
                                NewtonBodySetVelocity(nb, v);
                            }
                            off += 0x1dc;
                            j++;
                        } while (j < body->skel->count);
                    }
                    f58c4 = 0;
                } else
                    f58c4 = 0;
            } else if ((int)mount_id >= 0) {
                int j = 0;
                if (0 < body->skel->count) {
                    int off = 0;
                    do {
                        NewtonRagDollBone* b = *(NewtonRagDollBone**)((char*)body->skel->elems + 0x1d8 + off);
                        if (b != 0) {
                            NewtonBody* nb = NewtonRagDollBoneGetBody(b);
                            float v[4];
                            NewtonBodyGetVelocity(nb, v);
                            v[0] = v[0] + vx;
                            v[1] = v[1] + vy;
                            NewtonBodySetVelocity(nb, v);
                        }
                        off += 0x1dc;
                        j++;
                    } while (j < body->skel->count);
                }
                f58c4 = 0;
            } else
                f58c4 = 0;
        } else
            f58c4 = 0;
    } else if (!queued) {
        matrix* bm = (matrix*)&body->m;
        bm->f = dirv;
        bm->f.z = 0.0f;
        bm->u = vector4(0.0f, 0.0f, 1.0f);
        bm->orthonormalize();
        if (f14 != 1) {
            f84 = g_blend_rate + f84;
            if (f84 < 0.0f)
                f84 = 0.0f;
            else if (1.0f < f84)
                f84 = 1.0f;
            float* d = &floor->normal.x;
            float k = 1.0f - f84;
            float a = d[0] * f84;
            float b = d[1] * f84;
            float c = d[2] * f84;
            float zk = k * 0.0f;
            bm->u = vector4(zk + a, zk + b, k + c);
            bm->u.normalize();
            bm->s = rgl::cross(bm->f, bm->u);
            bm->s.normalize();
            bm->f = rgl::cross(bm->u, bm->s);
        }
        ((pos*)bm)->rotate_su(rot);
        bm->o = vector4(eye.x + apos.x, eye.y + apos.y, eye.z + apos.z);
    }
    camera_params(body->camp);
    if (f2c < 0) {
        if (state == 3 || state == 4) {
            body->set_mode(0);
        } else {
            body->set_mode(2);
            if (f30 == -1)
                body->f298 = -1;
        }
    } else {
        body->set_mode(0);
        AgentBody* mb = g_mission->pages[(unsigned)f2c >> 4][(unsigned)f2c & 0xf].body;
        if (mb != 0)
            mb->f298 = body->f8;
    }
    fx->visible = 0;
    if (g_fx_on != 0)
        fx->visible = 1;
}
