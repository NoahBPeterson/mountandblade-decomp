struct MapScreen;
struct MapObj { int pad[0x50]; };
extern void funct_4c3d90(int, int, int, MapScreen*, int);    // 0x4c3d90

struct MapScreen {
    char pad0[0x6690];
    MapObj* objects;                                    // +0x6690
    MapObj* add_object2(int kind, int x, int y, int z); // 0x4c6ee0
    MapObj* add_object(int kind, int x, int y, int z);  // 0x4c6eb0
};

// @ 0x4c6ee0
MapObj* MapScreen::add_object2(int kind, int x, int y, int z)
{
    funct_4c3d90(kind, x, z, this, z);
    return &objects[x];
}
