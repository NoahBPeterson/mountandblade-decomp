// Slice s00422130: the per-frame update of a skeletal "effect" animator (blends weighted animation records
// into per-bone quaternions, builds bone matrices, derives angular and linear velocities).
// Names are provisional. quaternion is the verified layout of s00422db0 (memory order w,x,y,z).
#include <math.h>
#include "rgl.h"

struct quaternion {
    float w, x, y, z;
    quaternion() {}
    quaternion(float x_, float y_, float z_, float w_) { x = x_; y = y_; z = z_; w = w_; }
    quaternion& set(const float& x_, const float& y_, const float& z_, const float& w_);   // 0x422db0
    void add(const quaternion& o);                                                         // 0x422de0
    quaternion operator+(const quaternion& o) const;                                       // 0x422e10
    void normalize();                                                                      // 0x422e40
    quaternion mul(const quaternion& o) const;                                             // 0x422eb0
    quaternion mul_conj(const quaternion& o) const;                                        // 0x422f60
    float dot(const quaternion& o) const;                                                  // 0x423010
    bool dot_negative(const quaternion& o) const;                                          // 0x423040
};
void quat_to_matrix(rgl::matrix* m, const quaternion* q);                                  // 0x423080
void quat_to_axis_angle(rgl::vector4* axis, float* angle, const quaternion* q);            // 0x423160

struct mat4 {
    rgl::vector4 s, f, u, o;
    mat4 operator*(const mat4& b) const;       // 0x40e870 (this = left operand)
};
extern __int64 g_bit[];                        // 0x5a8e68 (g_bit[i] == 1 << i)

// 0xd8-byte animation record (provisional)
struct Rec {
    float weight;                       // +0
    char pad0[4];
    unsigned __int64 active_mask;       // +8   bit per bone
    unsigned __int64 flip_mask;         // +0x10 bit per bone
    float f18;                          // +0x18 shaped phase
    char pad1[0x3c - 0x1c];
    int bone;                           // +0x3c
    float blend_a;                      // +0x40
    char pad2[0x68 - 0x44];
    quaternion* ref;                    // +0x68 per-bone reference quaternions
    float add_pos[3];                   // +0x6c
    char pad3[0xc0 - 0x78];
    float wt[2];                        // +0xc0
    char pad4[0xd8 - 0xc8];
    float phase();                                                                         // 0x4231f0
    float shape(float x);                                                                  // 0x4232b0
};

// 0x1dc-byte bone state (provisional)
struct Bone {
    char pad0[0x44];
    mat4 local;                         // +0x44 (origin at +0x74)
    mat4 world;                         // +0x84
    quaternion rot;                     // +0xc4
    quaternion abs_rot;                 // +0xd4
    quaternion delta;                   // +0xe4
    rgl::vector4 ang_vel;               // +0xf4
    char pad1[0x1c0 - 0x104];
    int parent;                         // +0x1c0
    char pad2[0x1d0 - 0x1c4];
    float rate;                         // +0x1d0
    char pad3[0x1dc - 0x1d4];
};
struct OwnerNode { char pad[0x3c]; rgl::vector4 p; };
struct Owner { char pad[0x1c]; OwnerNode* node; };

struct Animator {
    int nbones;                         // +0
    Bone* bones;                        // +4
    Owner* owner;                       // +8
    rgl::vector4 pos;                   // +0xc
    char pad0[0x40 - 0x1c];
    Rec* recs;                          // +0x40
    int nrecs;                          // +0x44
    char pad1[0x50 - 0x48];
    char done;                          // +0x50
    char pad2[0x54 - 0x51];
    float dt;                           // +0x54
    char pad3[0x68 - 0x58];
    char track_vel;                     // +0x68
    char pad4[0x6c - 0x69];
    rgl::vector4 vel;                   // +0x6c
    void update();                                                                         // 0x422130
    rgl::vector4 key_value(int i);                                                         // 0x423300
    quaternion bone_quat(int rec, int j, int bone);                                        // 0x423540
    quaternion parent_quat(int rec, int parent);                                           // 0x423b60
};

static inline rgl::vector4 vmul(const rgl::vector4& a, const float& s) { return rgl::vector4(a.x * s, a.y * s, a.z * s); }
static inline rgl::vector4 vmulv(const rgl::vector4& a, float s) { return rgl::vector4(a.x * s, a.y * s, a.z * s); }
static inline rgl::vector4 vadd(const rgl::vector4& a, const rgl::vector4& b) { return rgl::vector4(a.x + b.x, a.y + b.y, a.z + b.z); }
static inline rgl::vector4 vsub(const rgl::vector4& a, const rgl::vector4& b) { return rgl::vector4(a.x - b.x, a.y - b.y, a.z - b.z); }

static inline float vnormalize(rgl::vector4& v)
{
    float len = (float)sqrt(v.x * v.x + (v.y * v.y + v.z * v.z));
    if (len > 1e-10f) {
        float inv = 1.0f / len;
        v.x *= inv; v.y *= inv; v.z *= inv;
    } else {
        v.x = 0.0f; v.y = 1.0f; v.z = 0.0f;
    }
    return len;
}

#define BN(i) (*(Bone*)((int)bones + (i) * sizeof(Bone)))

// @ 0x422130
void Animator::update()
{
    rgl::vector4 old_pos = pos;
    for (int i = 0; i < nrecs; i++) {
        Rec& R = recs[i];
        float p = R.phase();
        R.f18 = R.shape(p);
    }
    for (int b = 0; b < nbones; b++) {
        quaternion saved = BN(b).rot;
        int found = -1;
        rgl::vector4 key0;
        quaternion q;
        for (int r = nrecs - 1; r >= 0; r--) {
            if (found != -1) break;
            if (!(recs[r].weight <= 0.0f) && ((unsigned)(recs[r].active_mask >> (unsigned char)b) & 1)) {
                if (b == 0) key0 = key_value(r);
                if (recs[r].bone == b) {
                    quaternion acc(0.0f, 0.0f, 0.0f, 0.0f);
                    for (int j = 0; j < 2; j++) {
                        if (recs[r].wt[j] > 0.001f) {
                            quaternion c; c = bone_quat(r, j, b);
                            const float* pw = &recs[r].wt[j];
                            quaternion t(c.x * *pw, c.y * *pw, c.z * *pw, c.w * *pw);
                            acc.add(t);
                        }
                    }
                    if (BN(b).parent == -1) {
                        q = acc;
                    } else {
                        quaternion pq = parent_quat(r, BN(b).parent);
                        acc.normalize();
                        quaternion m = pq.mul(acc);
                        quaternion d = BN(BN(b).parent).abs_rot.mul_conj(m);
                        float* pa = &recs[r].blend_a;
                        float s = 1.0f - *pa;
                        quaternion t1(acc.x * s, acc.y * s, acc.z * s, acc.w * s);
                        quaternion t2(d.x * *pa, d.y * *pa, d.z * *pa, d.w * *pa);
                        q = t1 + t2;
                    }
                } else {
                    q = quaternion(0.0f, 0.0f, 0.0f, 0.0f);
                    for (int j = 0; j < 2; j++) {
                        if (recs[r].wt[j] > 0.001f) {
                            quaternion c; c = bone_quat(r, j, b);
                            const float* pw = &recs[r].wt[j];
                            quaternion t(c.x * *pw, c.y * *pw, c.z * *pw, c.w * *pw);
                            q.add(t);
                        }
                    }
                }
                found = r;
            }
        }
        if (found != -1) {
            q.normalize();
            if ((g_bit[b] & recs[found].flip_mask) != 0) {
                quaternion m = recs[found].ref[b].mul(q);
                if (q.dot_negative(m)) {
                    m.x = -m.x; m.y = -m.y; m.z = -m.z; m.w = -m.w;
                }
                q = m;
                if (b == 0) {
                    const float* ap = recs[found].add_pos;
                    key0.x = key0.x + ap[0];
                    key0.y = key0.y + ap[1];
                    key0.z = key0.z + ap[2];
                }
            }
            float wgt = recs[found].weight * BN(b).rate;
            float blend = wgt * dt * 30.0f;
            if (blend < 0.0f) blend = 0.0f;
            else if (blend > 1.0f) blend = 1.0f;
            if (wgt <= 0.99f) {
                quaternion t = BN(b).delta;
                if (t.w > 0.0f) t.w = t.w + 1.0f;
                else t.w = t.w - 1.0f;
                t.normalize();
                quaternion m = saved.mul(t);
                if (q.dot(m) < 0.0f) {
                    m.x = -m.x; m.y = -m.y; m.z = -m.z; m.w = -m.w;
                }
                float s = 1.0f - blend;
                quaternion qa;
                qa.set(m.x * s, m.y * s, m.z * s, m.w * s);
                quaternion qb;
                qb.set(q.x * blend, q.y * blend, q.z * blend, q.w * blend);
                BN(b).rot = qa + qb;
                BN(b).rot.normalize();
            } else {
                BN(b).rot = q;
            }
            if (b == 0) {
                float s = 1.0f - blend;
                pos = vadd(vmul(key0, blend), vmul(old_pos, s));
            }
        }
        BN(b).delta = saved.mul_conj(BN(b).rot);
        if (BN(b).parent == -1)
            BN(b).abs_rot = BN(b).rot;
        else
            BN(b).abs_rot = BN(BN(b).parent).abs_rot.mul(BN(b).rot);
        quat_to_matrix((rgl::matrix*)&BN(b).local, &BN(b).rot);
        if (b == 0) {
            BN(0).local.o = vadd(owner->node->p, pos);
        }
        if (BN(b).parent == -1)
            BN(b).world = BN(b).local;
        else
            BN(b).world = BN(BN(b).parent).world * BN(b).local;
    }
    if (track_vel && dt > 0.001f) {
        for (int b = 0; b < nbones; b++) {
            rgl::vector4 axis;
            float angle;
            quat_to_axis_angle(&axis, &angle, &BN(b).delta);
            vnormalize(axis);
            rgl::vector4 w;
            w = vmul(axis, angle);
            BN(b).ang_vel = vmulv(w, 1.0f / dt);
        }
        rgl::vector4 d = vsub(pos, old_pos);
        vel = vmul(d, 1.0f / dt);
    }
    dt = 0.0f;
    done = 1;
}
