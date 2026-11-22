// patches/gfx/gfx_teardown.cpp: GameApp::DeleteDeviceObjects with the gfx set's own device objects released (patch set
// "gfx", opt-in).
//
// The DX9 sample framework releases the device in CD3DApplication::Cleanup3DEnvironment (src/platform/dxframework/d3dapp.cpp,
// 0x4026e0) right after DeleteDeviceObjects, and shows "A D3D object has a non-zero reference count" when that Release
// does not reach 0. Every object of ours holds a device reference (gfx_scale.cpp: the vertex and pixel shaders, the
// vertex declaration, the scaled colour / depth targets, the EASU and copy textures), so they go here, after the game's
// own. The function is virtual (GameApp's vtable slot 10), so every caller of the framework, the device change of
// d3dapp.cpp's Change3DEnvironment included, reaches this copy. A device created after it makes our objects again
// (gfx_scale_device_created). The body before the call is the faithful one of src/game/app/options_dialog_and_module_tables.cpp.
#include "../../src/game/config/core.h"     // GameApp (font +0x348)
#include "../../src/engine/resource/resource.h"         // g_resmgr
#include "../../src/engine/render/render_batch.h"    // g_batch
#include "./gfx.h"

// @ replaces 0x4c1ad0
HRESULT GameApp::DeleteDeviceObjects()
{
    font->DeleteDeviceObjects();
    g_resmgr.invalidate();
    g_batch.release_decls();
    gfx_scale_device_deleted();
    return 0;
}
