// patches/perf/resmgr_add_index.cpp: hashed duplicate check in resmgr::add (patch set "perf").
//
// What it changes. resmgr::add (0x41ba90) registers each loaded record and first scans the whole vector of its kind for
// a record of the same name ("attempt to reregister mesh %s"). The 146 .brf files register 4740 meshes, so the mesh scan
// alone is 4740*4739/2 = 11.2 million rgl::string::equals calls (a strcmp each, chasing mesh -> name buffer pointers over
// a few MB of heap). This replacement keeps the scan for the small kinds (textures, shaders, materials, bodies,
// skeletons, animations: 337/82/408/600/2/118 records) and answers the mesh check from a hash index of the names.
//
// Why it is equivalent. The faithful version (0x41ba90) is byte-exact. For meshes it errors exactly when some registered
// mesh has a name equal to r->name under rgl::string::equals (both empty, or both non-empty and strcmp-equal). The index
// is built from the same `meshes` vector, lazily, up to its current size, so it holds exactly the vector's entries; a
// hit is confirmed with the same equals() call before the same mb_error. It is rebuilt from scratch when the vector
// shrank or its first element changed (clear, erase). The one assumption is that a registered mesh's name is not changed
// after registration: the index keys are hashed when the mesh is first seen by add(). Mesh names are set by read_mesh
// (before add) or by mesh construction. A mesh pushed into `meshes` without going through add is picked up on the next
// call. The error message names the matching mesh, which has the same name.
#include <windows.h>
#include <d3d9.h>
#include <d3dx9.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <time.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <new>
#include <vector>
#include <list>
#include <memory>
#include <algorithm>
#include "../../src/rgl/rgl_string.h"
#include "../../src/rgl/rgl_math.h"
#include "../../src/rgl/stream.h"
#include "../../src/rgl/resource.h"
#include "../../src/rgl/resource_manager.h"
#include "../../src/rgl/particles/mesh_obj.h"
#include "../../src/rgl/mem_stream.h"
#include "../../src/rgl/text_stream.h"
#include "../../src/rgl/core/resource_io.h"
#include "../../src/rgl/core/resmgr.h"
#include "../../src/include/keyframes.h"
#include "../../src/include/mat4.h"
#include "../../src/include/mesh_prims.h"
#include "../../src/include/vec.h"
#include "../../src/include/mesh_parts.h"
#include "../../src/rgl/mesh_x.h"
#include "../../src/rgl/mesh.h"

using namespace rgl;

namespace {

// name -> chain of mesh pointers (open hashing over a power-of-two bucket array; chains through `next`)
struct MeshIndex {
    std::vector<mesh*> items;          // the meshes indexed so far, in vector order
    std::vector<unsigned> hash;        // hash[i] of items[i]
    std::vector<int> next;             // next item of the same bucket, -1 = end
    std::vector<int> head;             // bucket -> first item, -1 = empty
    mesh* first;                       // meshes[0] when the index was built (detects clear/erase)

    MeshIndex() : first(0) {}

    static unsigned name_hash(const rgl::string& s)
    {
        if (s.len == 0)
            return 0;
        unsigned h = 2166136261u;
        for (const unsigned char* p = (const unsigned char*)s.buf; *p; ++p)
            h = (h ^ *p) * 16777619u;           // FNV-1a over the C string (equals() compares with strcmp)
        return h | 1u;                          // never 0: an empty name hashes to 0 and compares by its own rule
    }

    void clear()
    {
        items.clear();
        hash.clear();
        next.clear();
        head.assign(1024, -1);
        first = 0;
    }

    void insert(mesh* m)
    {
        if (items.size() * 2 >= head.size()) {          // grow and rehash
            std::vector<int> nh(head.size() * 2, -1);
            for (size_t i = 0; i < items.size(); i++) {
                unsigned b = hash[i] & (nh.size() - 1);
                next[i] = nh[b];
                nh[b] = (int)i;
            }
            head.swap(nh);
        }
        unsigned h = name_hash(m->name);
        unsigned b = h & (head.size() - 1);
        items.push_back(m);
        hash.push_back(h);
        next.push_back(head[b]);
        head[b] = (int)items.size() - 1;
    }

    // brings the index up to date with the registered meshes
    void sync(const std::vector<mesh*>& v)
    {
        if (head.empty() || v.size() < items.size() || (!v.empty() && v[0] != first))
            clear();
        if (items.empty() && !v.empty())
            first = v[0];
        for (size_t i = items.size(); i < v.size(); i++)
            insert(v[i]);
    }

    mesh* find(const rgl::string& name) const
    {
        unsigned h = name_hash(name);
        for (int i = head[h & (head.size() - 1)]; i >= 0; i = next[i])
            if (hash[i] == h && items[i]->name.equals(name))
                return items[i];
        return 0;
    }
};

MeshIndex g_mesh_index;

}   // namespace

// @ replaces 0x41ba90
// @ 0x41ba90
void resmgr::add(named_object* r)
{
    int t = r->type();
    if (t == 1) {
        g_mesh_index.sync(meshes);
        mesh* dup = g_mesh_index.find(r->name);
        if (dup)
            mb_error("attempt to reregister mesh %s", dup->name.c_str());
        meshes.push_back((mesh*)r);
    } else if (r->type() == 2) {
        for (std::vector<texture*>::iterator it = textures.begin(); it != textures.end(); ++it)
            if (r->name.len && (*it)->name.len && (*it)->name.equals(r->name))
                mb_error("attempt to reregister texture %s", (*it)->name.c_str());
        ((texture*)r)->f1c = c4;
        if (++c4 > 1000)
            c4 = 1;
        textures.push_back((texture*)r);
    } else if (r->type() == 4) {
        for (std::vector<material*>::iterator it = shaders.begin(); it != shaders.end(); ++it)
            if (r->name.len && (*it)->name.len && (*it)->name.equals(r->name))
                mb_error("attempt to reregister shader %s", (*it)->name.c_str());
        ((int*)r)[6] = cc;
        if (++cc > 1000)
            cc = 1;
        shaders.push_back((material*)r);
    } else if (r->type() == 3) {
        for (std::vector<skin*>::iterator it = materials.begin(); it != materials.end(); ++it)
            if (r->name.len && (*it)->name.len && (*it)->name.equals(r->name))
                mb_error("attempt to reregister material %s", (*it)->name.c_str());
        ((int*)r)[6] = c8;
        if (++c8 > 1000)
            c8 = 1;
        materials.push_back((skin*)r);
    } else if (r->type() == 6) {
        for (std::vector<body_part*>::iterator it = bodies.begin(); it != bodies.end(); ++it)
            if ((*it)->name.equals(r->name))
                mb_error("attempt to reregister body %s", (*it)->name.c_str());
        bodies.push_back((body_part*)r);
    } else if (r->type() == 7) {
        for (std::vector<frame_set*>::iterator it = skeletons.begin(); it != skeletons.end(); ++it)
            if ((*it)->name.equals(r->name))
                mb_error("attempt to reregister skeleton model %s", (*it)->name.c_str());
        skeletons.push_back((frame_set*)r);
    } else if (r->type() == 8) {
        for (std::vector<skel_anim*>::iterator it = anims.begin(); it != anims.end(); ++it)
            if ((*it)->name.equals(r->name))
                mb_error("attempt to reregister skeleton anim %s", (*it)->name.c_str());
        anims.push_back((skel_anim*)r);
    } else {
        mb_error("Unrecognized resource type.");
    }
    all.push_back(r);
}
