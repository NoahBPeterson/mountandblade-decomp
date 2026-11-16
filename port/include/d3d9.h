// port/include/d3d9.h: portable-build shim (ticket 004) for the Direct3D 9 types src/ names. Struct layouts and enum values
// are the D3D9 API's (so the renderer's records keep their 32-bit sizes on i686); the interfaces list every method in the
// API's vtable order, so a D3D9 implementation (dxvk-native as the ticket 008 B0 bring-up crutch) could sit behind them.
// Written for the port from the API's documented interface; values only where src/ uses a name.
#pragma once
#ifndef MB_PORT_D3D9_H
#define MB_PORT_D3D9_H
#include <windows.h>

#define DIRECT3D_VERSION 0x0900
#define D3D_SDK_VERSION (32 | 0x80000000)
#define D3DADAPTER_DEFAULT 0
#define MAX_DEVICE_IDENTIFIER_STRING 512
#define MAKEFOURCC(a, b, c, d) ((DWORD)(BYTE)(a) | ((DWORD)(BYTE)(b) << 8) | ((DWORD)(BYTE)(c) << 16) | ((DWORD)(BYTE)(d) << 24))
#define _FACD3D 0x876
#define MAKE_D3DHRESULT(code) MAKE_HRESULT(1, _FACD3D, code)
#define MAKE_D3DSTATUS(code) MAKE_HRESULT(0, _FACD3D, code)

typedef DWORD D3DCOLOR;
#define D3DCOLOR_ARGB(a, r, g, b) ((D3DCOLOR)((((a) & 0xff) << 24) | (((r) & 0xff) << 16) | (((g) & 0xff) << 8) | ((b) & 0xff)))
#define D3DCOLOR_RGBA(r, g, b, a) D3DCOLOR_ARGB(a, r, g, b)
#define D3DCOLOR_XRGB(r, g, b) D3DCOLOR_ARGB(0xff, r, g, b)
#define D3DCOLOR_COLORVALUE(r, g, b, a) D3DCOLOR_RGBA((DWORD)((r) * 255.f), (DWORD)((g) * 255.f), (DWORD)((b) * 255.f), (DWORD)((a) * 255.f))

// ---- enums ----
typedef enum _D3DFORMAT {
    D3DFMT_UNKNOWN = 0,
    D3DFMT_R8G8B8 = 20, D3DFMT_A8R8G8B8 = 21, D3DFMT_X8R8G8B8 = 22, D3DFMT_R5G6B5 = 23, D3DFMT_X1R5G5B5 = 24,
    D3DFMT_A1R5G5B5 = 25, D3DFMT_A4R4G4B4 = 26, D3DFMT_R3G3B2 = 27, D3DFMT_A8 = 28, D3DFMT_A8R3G3B2 = 29,
    D3DFMT_X4R4G4B4 = 30, D3DFMT_A2B10G10R10 = 31, D3DFMT_A8B8G8R8 = 32, D3DFMT_X8B8G8R8 = 33, D3DFMT_G16R16 = 34,
    D3DFMT_A2R10G10B10 = 35, D3DFMT_A16B16G16R16 = 36, D3DFMT_A8P8 = 40, D3DFMT_P8 = 41, D3DFMT_L8 = 50,
    D3DFMT_A8L8 = 51, D3DFMT_A4L4 = 52, D3DFMT_V8U8 = 60, D3DFMT_L6V5U5 = 61, D3DFMT_X8L8V8U8 = 62,
    D3DFMT_Q8W8V8U8 = 63, D3DFMT_V16U16 = 64, D3DFMT_A2W10V10U10 = 67,
    D3DFMT_UYVY = MAKEFOURCC('U', 'Y', 'V', 'Y'), D3DFMT_R8G8_B8G8 = MAKEFOURCC('R', 'G', 'B', 'G'),
    D3DFMT_YUY2 = MAKEFOURCC('Y', 'U', 'Y', '2'), D3DFMT_G8R8_G8B8 = MAKEFOURCC('G', 'R', 'G', 'B'),
    D3DFMT_DXT1 = MAKEFOURCC('D', 'X', 'T', '1'), D3DFMT_DXT2 = MAKEFOURCC('D', 'X', 'T', '2'),
    D3DFMT_DXT3 = MAKEFOURCC('D', 'X', 'T', '3'), D3DFMT_DXT4 = MAKEFOURCC('D', 'X', 'T', '4'),
    D3DFMT_DXT5 = MAKEFOURCC('D', 'X', 'T', '5'),
    D3DFMT_D16_LOCKABLE = 70, D3DFMT_D32 = 71, D3DFMT_D15S1 = 73, D3DFMT_D24S8 = 75, D3DFMT_D24X8 = 77,
    D3DFMT_D24X4S4 = 79, D3DFMT_D16 = 80, D3DFMT_D32F_LOCKABLE = 82, D3DFMT_D24FS8 = 83, D3DFMT_L16 = 81,
    D3DFMT_VERTEXDATA = 100, D3DFMT_INDEX16 = 101, D3DFMT_INDEX32 = 102, D3DFMT_Q16W16V16U16 = 110,
    D3DFMT_MULTI2_ARGB8 = MAKEFOURCC('M', 'E', 'T', '1'),
    D3DFMT_R16F = 111, D3DFMT_G16R16F = 112, D3DFMT_A16B16G16R16F = 113, D3DFMT_R32F = 114, D3DFMT_G32R32F = 115,
    D3DFMT_A32B32G32R32F = 116, D3DFMT_CxV8U8 = 117,
    D3DFMT_FORCE_DWORD = 0x7fffffff
} D3DFORMAT;

typedef enum _D3DDEVTYPE { D3DDEVTYPE_HAL = 1, D3DDEVTYPE_REF = 2, D3DDEVTYPE_SW = 3, D3DDEVTYPE_NULLREF = 4, D3DDEVTYPE_FORCE_DWORD = 0x7fffffff } D3DDEVTYPE;
typedef enum _D3DMULTISAMPLE_TYPE {
    D3DMULTISAMPLE_NONE = 0, D3DMULTISAMPLE_NONMASKABLE = 1, D3DMULTISAMPLE_2_SAMPLES = 2, D3DMULTISAMPLE_3_SAMPLES = 3,
    D3DMULTISAMPLE_4_SAMPLES = 4, D3DMULTISAMPLE_5_SAMPLES = 5, D3DMULTISAMPLE_6_SAMPLES = 6, D3DMULTISAMPLE_7_SAMPLES = 7,
    D3DMULTISAMPLE_8_SAMPLES = 8, D3DMULTISAMPLE_9_SAMPLES = 9, D3DMULTISAMPLE_10_SAMPLES = 10, D3DMULTISAMPLE_11_SAMPLES = 11,
    D3DMULTISAMPLE_12_SAMPLES = 12, D3DMULTISAMPLE_13_SAMPLES = 13, D3DMULTISAMPLE_14_SAMPLES = 14, D3DMULTISAMPLE_15_SAMPLES = 15,
    D3DMULTISAMPLE_16_SAMPLES = 16, D3DMULTISAMPLE_FORCE_DWORD = 0x7fffffff
} D3DMULTISAMPLE_TYPE;
typedef enum _D3DSWAPEFFECT { D3DSWAPEFFECT_DISCARD = 1, D3DSWAPEFFECT_FLIP = 2, D3DSWAPEFFECT_COPY = 3, D3DSWAPEFFECT_FORCE_DWORD = 0x7fffffff } D3DSWAPEFFECT;
typedef enum _D3DPOOL { D3DPOOL_DEFAULT = 0, D3DPOOL_MANAGED = 1, D3DPOOL_SYSTEMMEM = 2, D3DPOOL_SCRATCH = 3, D3DPOOL_FORCE_DWORD = 0x7fffffff } D3DPOOL;
typedef enum _D3DRESOURCETYPE {
    D3DRTYPE_SURFACE = 1, D3DRTYPE_VOLUME = 2, D3DRTYPE_TEXTURE = 3, D3DRTYPE_VOLUMETEXTURE = 4, D3DRTYPE_CUBETEXTURE = 5,
    D3DRTYPE_VERTEXBUFFER = 6, D3DRTYPE_INDEXBUFFER = 7, D3DRTYPE_FORCE_DWORD = 0x7fffffff
} D3DRESOURCETYPE;
typedef enum _D3DBACKBUFFER_TYPE { D3DBACKBUFFER_TYPE_MONO = 0, D3DBACKBUFFER_TYPE_LEFT = 1, D3DBACKBUFFER_TYPE_RIGHT = 2, D3DBACKBUFFER_TYPE_FORCE_DWORD = 0x7fffffff } D3DBACKBUFFER_TYPE;
typedef enum _D3DPRIMITIVETYPE {
    D3DPT_POINTLIST = 1, D3DPT_LINELIST = 2, D3DPT_LINESTRIP = 3, D3DPT_TRIANGLELIST = 4, D3DPT_TRIANGLESTRIP = 5,
    D3DPT_TRIANGLEFAN = 6, D3DPT_FORCE_DWORD = 0x7fffffff
} D3DPRIMITIVETYPE;
typedef enum _D3DTRANSFORMSTATETYPE {
    D3DTS_VIEW = 2, D3DTS_PROJECTION = 3, D3DTS_TEXTURE0 = 16, D3DTS_TEXTURE1 = 17, D3DTS_TEXTURE2 = 18, D3DTS_TEXTURE3 = 19,
    D3DTS_TEXTURE4 = 20, D3DTS_TEXTURE5 = 21, D3DTS_TEXTURE6 = 22, D3DTS_TEXTURE7 = 23, D3DTS_FORCE_DWORD = 0x7fffffff
} D3DTRANSFORMSTATETYPE;
#define D3DTS_WORLDMATRIX(index) (D3DTRANSFORMSTATETYPE)((index) + 256)
#define D3DTS_WORLD D3DTS_WORLDMATRIX(0)
#define D3DTS_WORLD1 D3DTS_WORLDMATRIX(1)

typedef enum _D3DRENDERSTATETYPE {
    D3DRS_ZENABLE = 7, D3DRS_FILLMODE = 8, D3DRS_SHADEMODE = 9, D3DRS_ZWRITEENABLE = 14, D3DRS_ALPHATESTENABLE = 15,
    D3DRS_LASTPIXEL = 16, D3DRS_SRCBLEND = 19, D3DRS_DESTBLEND = 20, D3DRS_CULLMODE = 22, D3DRS_ZFUNC = 23,
    D3DRS_ALPHAREF = 24, D3DRS_ALPHAFUNC = 25, D3DRS_DITHERENABLE = 26, D3DRS_ALPHABLENDENABLE = 27, D3DRS_FOGENABLE = 28,
    D3DRS_SPECULARENABLE = 29, D3DRS_FOGCOLOR = 34, D3DRS_FOGTABLEMODE = 35, D3DRS_FOGSTART = 36, D3DRS_FOGEND = 37,
    D3DRS_FOGDENSITY = 38, D3DRS_RANGEFOGENABLE = 48, D3DRS_STENCILENABLE = 52, D3DRS_STENCILFAIL = 53,
    D3DRS_STENCILZFAIL = 54, D3DRS_STENCILPASS = 55, D3DRS_STENCILFUNC = 56, D3DRS_STENCILREF = 57, D3DRS_STENCILMASK = 58,
    D3DRS_STENCILWRITEMASK = 59, D3DRS_TEXTUREFACTOR = 60, D3DRS_WRAP0 = 128, D3DRS_CLIPPING = 136, D3DRS_LIGHTING = 137,
    D3DRS_AMBIENT = 139, D3DRS_FOGVERTEXMODE = 140, D3DRS_COLORVERTEX = 141, D3DRS_LOCALVIEWER = 142,
    D3DRS_NORMALIZENORMALS = 143, D3DRS_DIFFUSEMATERIALSOURCE = 145, D3DRS_SPECULARMATERIALSOURCE = 146,
    D3DRS_AMBIENTMATERIALSOURCE = 147, D3DRS_EMISSIVEMATERIALSOURCE = 148, D3DRS_VERTEXBLEND = 151,
    D3DRS_CLIPPLANEENABLE = 152, D3DRS_POINTSIZE = 154, D3DRS_POINTSIZE_MIN = 155, D3DRS_POINTSPRITEENABLE = 156,
    D3DRS_POINTSCALEENABLE = 157, D3DRS_MULTISAMPLEANTIALIAS = 161, D3DRS_MULTISAMPLEMASK = 162,
    D3DRS_INDEXEDVERTEXBLENDENABLE = 167, D3DRS_COLORWRITEENABLE = 168, D3DRS_TWEENFACTOR = 170, D3DRS_BLENDOP = 171,
    D3DRS_SCISSORTESTENABLE = 174, D3DRS_SLOPESCALEDEPTHBIAS = 175, D3DRS_DEPTHBIAS = 195, D3DRS_SRGBWRITEENABLE = 194,
    D3DRS_SEPARATEALPHABLENDENABLE = 206, D3DRS_SRCBLENDALPHA = 207, D3DRS_DESTBLENDALPHA = 208, D3DRS_BLENDOPALPHA = 209,
    D3DRS_FORCE_DWORD = 0x7fffffff
} D3DRENDERSTATETYPE;
typedef enum _D3DTEXTURESTAGESTATETYPE {
    D3DTSS_COLOROP = 1, D3DTSS_COLORARG1 = 2, D3DTSS_COLORARG2 = 3, D3DTSS_ALPHAOP = 4, D3DTSS_ALPHAARG1 = 5,
    D3DTSS_ALPHAARG2 = 6, D3DTSS_BUMPENVMAT00 = 7, D3DTSS_BUMPENVMAT01 = 8, D3DTSS_BUMPENVMAT10 = 9, D3DTSS_BUMPENVMAT11 = 10,
    D3DTSS_TEXCOORDINDEX = 11, D3DTSS_BUMPENVLSCALE = 22, D3DTSS_BUMPENVLOFFSET = 23, D3DTSS_TEXTURETRANSFORMFLAGS = 24,
    D3DTSS_COLORARG0 = 26, D3DTSS_ALPHAARG0 = 27, D3DTSS_RESULTARG = 28, D3DTSS_CONSTANT = 32,
    D3DTSS_FORCE_DWORD = 0x7fffffff
} D3DTEXTURESTAGESTATETYPE;
typedef enum _D3DSAMPLERSTATETYPE {
    D3DSAMP_ADDRESSU = 1, D3DSAMP_ADDRESSV = 2, D3DSAMP_ADDRESSW = 3, D3DSAMP_BORDERCOLOR = 4, D3DSAMP_MAGFILTER = 5,
    D3DSAMP_MINFILTER = 6, D3DSAMP_MIPFILTER = 7, D3DSAMP_MIPMAPLODBIAS = 8, D3DSAMP_MAXMIPLEVEL = 9, D3DSAMP_MAXANISOTROPY = 10,
    D3DSAMP_SRGBTEXTURE = 11, D3DSAMP_ELEMENTINDEX = 12, D3DSAMP_DMAPOFFSET = 13, D3DSAMP_FORCE_DWORD = 0x7fffffff
} D3DSAMPLERSTATETYPE;
typedef enum _D3DTEXTUREOP {
    D3DTOP_DISABLE = 1, D3DTOP_SELECTARG1 = 2, D3DTOP_SELECTARG2 = 3, D3DTOP_MODULATE = 4, D3DTOP_MODULATE2X = 5,
    D3DTOP_MODULATE4X = 6, D3DTOP_ADD = 7, D3DTOP_ADDSIGNED = 8, D3DTOP_SUBTRACT = 10, D3DTOP_BLENDDIFFUSEALPHA = 12,
    D3DTOP_BLENDTEXTUREALPHA = 13, D3DTOP_BLENDFACTORALPHA = 14, D3DTOP_BLENDCURRENTALPHA = 16, D3DTOP_DOTPRODUCT3 = 24,
    D3DTOP_MULTIPLYADD = 25, D3DTOP_LERP = 26, D3DTOP_FORCE_DWORD = 0x7fffffff
} D3DTEXTUREOP;
#define D3DTA_SELECTMASK 0x0000000f
#define D3DTA_DIFFUSE 0x00000000
#define D3DTA_CURRENT 0x00000001
#define D3DTA_TEXTURE 0x00000002
#define D3DTA_TFACTOR 0x00000003
#define D3DTA_SPECULAR 0x00000004
#define D3DTA_TEMP 0x00000005
#define D3DTA_CONSTANT 0x00000006
#define D3DTA_COMPLEMENT 0x00000010
#define D3DTA_ALPHAREPLICATE 0x00000020
typedef enum _D3DTEXTUREFILTERTYPE {
    D3DTEXF_NONE = 0, D3DTEXF_POINT = 1, D3DTEXF_LINEAR = 2, D3DTEXF_ANISOTROPIC = 3, D3DTEXF_PYRAMIDALQUAD = 6,
    D3DTEXF_GAUSSIANQUAD = 7, D3DTEXF_FORCE_DWORD = 0x7fffffff
} D3DTEXTUREFILTERTYPE;
typedef enum _D3DTEXTUREADDRESS {
    D3DTADDRESS_WRAP = 1, D3DTADDRESS_MIRROR = 2, D3DTADDRESS_CLAMP = 3, D3DTADDRESS_BORDER = 4, D3DTADDRESS_MIRRORONCE = 5,
    D3DTADDRESS_FORCE_DWORD = 0x7fffffff
} D3DTEXTUREADDRESS;
typedef enum _D3DTEXTURETRANSFORMFLAGS {
    D3DTTFF_DISABLE = 0, D3DTTFF_COUNT1 = 1, D3DTTFF_COUNT2 = 2, D3DTTFF_COUNT3 = 3, D3DTTFF_COUNT4 = 4,
    D3DTTFF_PROJECTED = 256, D3DTTFF_FORCE_DWORD = 0x7fffffff
} D3DTEXTURETRANSFORMFLAGS;
#define D3DTSS_TCI_PASSTHRU 0x00000000
#define D3DTSS_TCI_CAMERASPACENORMAL 0x00010000
#define D3DTSS_TCI_CAMERASPACEPOSITION 0x00020000
#define D3DTSS_TCI_CAMERASPACEREFLECTIONVECTOR 0x00030000
#define D3DTSS_TCI_SPHEREMAP 0x00040000
typedef enum _D3DBLEND {
    D3DBLEND_ZERO = 1, D3DBLEND_ONE = 2, D3DBLEND_SRCCOLOR = 3, D3DBLEND_INVSRCCOLOR = 4, D3DBLEND_SRCALPHA = 5,
    D3DBLEND_INVSRCALPHA = 6, D3DBLEND_DESTALPHA = 7, D3DBLEND_INVDESTALPHA = 8, D3DBLEND_DESTCOLOR = 9,
    D3DBLEND_INVDESTCOLOR = 10, D3DBLEND_SRCALPHASAT = 11, D3DBLEND_BLENDFACTOR = 14, D3DBLEND_FORCE_DWORD = 0x7fffffff
} D3DBLEND;
typedef enum _D3DBLENDOP { D3DBLENDOP_ADD = 1, D3DBLENDOP_SUBTRACT = 2, D3DBLENDOP_REVSUBTRACT = 3, D3DBLENDOP_MIN = 4, D3DBLENDOP_MAX = 5, D3DBLENDOP_FORCE_DWORD = 0x7fffffff } D3DBLENDOP;
typedef enum _D3DCMPFUNC {
    D3DCMP_NEVER = 1, D3DCMP_LESS = 2, D3DCMP_EQUAL = 3, D3DCMP_LESSEQUAL = 4, D3DCMP_GREATER = 5, D3DCMP_NOTEQUAL = 6,
    D3DCMP_GREATEREQUAL = 7, D3DCMP_ALWAYS = 8, D3DCMP_FORCE_DWORD = 0x7fffffff
} D3DCMPFUNC;
typedef enum _D3DCULL { D3DCULL_NONE = 1, D3DCULL_CW = 2, D3DCULL_CCW = 3, D3DCULL_FORCE_DWORD = 0x7fffffff } D3DCULL;
typedef enum _D3DFILLMODE { D3DFILL_POINT = 1, D3DFILL_WIREFRAME = 2, D3DFILL_SOLID = 3, D3DFILL_FORCE_DWORD = 0x7fffffff } D3DFILLMODE;
typedef enum _D3DSHADEMODE { D3DSHADE_FLAT = 1, D3DSHADE_GOURAUD = 2, D3DSHADE_PHONG = 3, D3DSHADE_FORCE_DWORD = 0x7fffffff } D3DSHADEMODE;
typedef enum _D3DFOGMODE { D3DFOG_NONE = 0, D3DFOG_EXP = 1, D3DFOG_EXP2 = 2, D3DFOG_LINEAR = 3, D3DFOG_FORCE_DWORD = 0x7fffffff } D3DFOGMODE;
typedef enum _D3DZBUFFERTYPE { D3DZB_FALSE = 0, D3DZB_TRUE = 1, D3DZB_USEW = 2, D3DZB_FORCE_DWORD = 0x7fffffff } D3DZBUFFERTYPE;
typedef enum _D3DSTENCILOP {
    D3DSTENCILOP_KEEP = 1, D3DSTENCILOP_ZERO = 2, D3DSTENCILOP_REPLACE = 3, D3DSTENCILOP_INCRSAT = 4, D3DSTENCILOP_DECRSAT = 5,
    D3DSTENCILOP_INVERT = 6, D3DSTENCILOP_INCR = 7, D3DSTENCILOP_DECR = 8, D3DSTENCILOP_FORCE_DWORD = 0x7fffffff
} D3DSTENCILOP;
typedef enum _D3DMATERIALCOLORSOURCE { D3DMCS_MATERIAL = 0, D3DMCS_COLOR1 = 1, D3DMCS_COLOR2 = 2, D3DMCS_FORCE_DWORD = 0x7fffffff } D3DMATERIALCOLORSOURCE;
typedef enum _D3DVERTEXBLENDFLAGS {
    D3DVBF_DISABLE = 0, D3DVBF_1WEIGHTS = 1, D3DVBF_2WEIGHTS = 2, D3DVBF_3WEIGHTS = 3, D3DVBF_TWEENING = 255, D3DVBF_0WEIGHTS = 256,
    D3DVBF_FORCE_DWORD = 0x7fffffff
} D3DVERTEXBLENDFLAGS;
typedef enum _D3DLIGHTTYPE { D3DLIGHT_POINT = 1, D3DLIGHT_SPOT = 2, D3DLIGHT_DIRECTIONAL = 3, D3DLIGHT_FORCE_DWORD = 0x7fffffff } D3DLIGHTTYPE;
typedef enum _D3DDECLTYPE {
    D3DDECLTYPE_FLOAT1 = 0, D3DDECLTYPE_FLOAT2 = 1, D3DDECLTYPE_FLOAT3 = 2, D3DDECLTYPE_FLOAT4 = 3, D3DDECLTYPE_D3DCOLOR = 4,
    D3DDECLTYPE_UBYTE4 = 5, D3DDECLTYPE_SHORT2 = 6, D3DDECLTYPE_SHORT4 = 7, D3DDECLTYPE_UBYTE4N = 8, D3DDECLTYPE_SHORT2N = 9,
    D3DDECLTYPE_SHORT4N = 10, D3DDECLTYPE_USHORT2N = 11, D3DDECLTYPE_USHORT4N = 12, D3DDECLTYPE_UDEC3 = 13,
    D3DDECLTYPE_DEC3N = 14, D3DDECLTYPE_FLOAT16_2 = 15, D3DDECLTYPE_FLOAT16_4 = 16, D3DDECLTYPE_UNUSED = 17
} D3DDECLTYPE;
typedef enum _D3DDECLMETHOD {
    D3DDECLMETHOD_DEFAULT = 0, D3DDECLMETHOD_PARTIALU, D3DDECLMETHOD_PARTIALV, D3DDECLMETHOD_CROSSUV, D3DDECLMETHOD_UV,
    D3DDECLMETHOD_LOOKUP, D3DDECLMETHOD_LOOKUPPRESAMPLED
} D3DDECLMETHOD;
typedef enum _D3DDECLUSAGE {
    D3DDECLUSAGE_POSITION = 0, D3DDECLUSAGE_BLENDWEIGHT, D3DDECLUSAGE_BLENDINDICES, D3DDECLUSAGE_NORMAL, D3DDECLUSAGE_PSIZE,
    D3DDECLUSAGE_TEXCOORD, D3DDECLUSAGE_TANGENT, D3DDECLUSAGE_BINORMAL, D3DDECLUSAGE_TESSFACTOR, D3DDECLUSAGE_POSITIONT,
    D3DDECLUSAGE_COLOR, D3DDECLUSAGE_FOG, D3DDECLUSAGE_DEPTH, D3DDECLUSAGE_SAMPLE
} D3DDECLUSAGE;
#define D3DDECL_END() { 0xFF, 0, D3DDECLTYPE_UNUSED, 0, 0, 0 }
typedef enum _D3DSTATEBLOCKTYPE { D3DSBT_ALL = 1, D3DSBT_PIXELSTATE = 2, D3DSBT_VERTEXSTATE = 3, D3DSBT_FORCE_DWORD = 0x7fffffff } D3DSTATEBLOCKTYPE;
typedef enum _D3DCUBEMAP_FACES {
    D3DCUBEMAP_FACE_POSITIVE_X = 0, D3DCUBEMAP_FACE_NEGATIVE_X = 1, D3DCUBEMAP_FACE_POSITIVE_Y = 2,
    D3DCUBEMAP_FACE_NEGATIVE_Y = 3, D3DCUBEMAP_FACE_POSITIVE_Z = 4, D3DCUBEMAP_FACE_NEGATIVE_Z = 5,
    D3DCUBEMAP_FACE_FORCE_DWORD = 0x7fffffff
} D3DCUBEMAP_FACES;
typedef enum _D3DQUERYTYPE {
    D3DQUERYTYPE_VCACHE = 4, D3DQUERYTYPE_RESOURCEMANAGER = 5, D3DQUERYTYPE_VERTEXSTATS = 6, D3DQUERYTYPE_EVENT = 8,
    D3DQUERYTYPE_OCCLUSION = 9, D3DQUERYTYPE_TIMESTAMP = 10, D3DQUERYTYPE_TIMESTAMPDISJOINT = 11, D3DQUERYTYPE_TIMESTAMPFREQ = 12
} D3DQUERYTYPE;
#define D3DISSUE_END (1 << 0)
#define D3DISSUE_BEGIN (1 << 1)
#define D3DGETDATA_FLUSH (1 << 0)

// ---- flags ----
#define D3DCREATE_FPU_PRESERVE 0x00000002L
#define D3DCREATE_MULTITHREADED 0x00000004L
#define D3DCREATE_PUREDEVICE 0x00000010L
#define D3DCREATE_SOFTWARE_VERTEXPROCESSING 0x00000020L
#define D3DCREATE_HARDWARE_VERTEXPROCESSING 0x00000040L
#define D3DCREATE_MIXED_VERTEXPROCESSING 0x00000080L
#define D3DUSAGE_RENDERTARGET (0x00000001L)
#define D3DUSAGE_DEPTHSTENCIL (0x00000002L)
#define D3DUSAGE_WRITEONLY (0x00000008L)
#define D3DUSAGE_SOFTWAREPROCESSING (0x00000010L)
#define D3DUSAGE_DONOTCLIP (0x00000020L)
#define D3DUSAGE_POINTS (0x00000040L)
#define D3DUSAGE_DYNAMIC (0x00000200L)
#define D3DUSAGE_AUTOGENMIPMAP (0x00000400L)
#define D3DUSAGE_QUERY_POSTPIXELSHADER_BLENDING (0x00080000L)
#define D3DUSAGE_QUERY_FILTER (0x00020000L)
#define D3DLOCK_READONLY 0x00000010L
#define D3DLOCK_DISCARD 0x00002000L
#define D3DLOCK_NOOVERWRITE 0x00001000L
#define D3DLOCK_NOSYSLOCK 0x00000800L
#define D3DLOCK_DONOTWAIT 0x00004000L
#define D3DLOCK_NO_DIRTY_UPDATE 0x00008000L
#define D3DCLEAR_TARGET 0x00000001l
#define D3DCLEAR_ZBUFFER 0x00000002l
#define D3DCLEAR_STENCIL 0x00000004l
#define D3DPRESENT_INTERVAL_DEFAULT 0x00000000L
#define D3DPRESENT_INTERVAL_ONE 0x00000001L
#define D3DPRESENT_INTERVAL_TWO 0x00000002L
#define D3DPRESENT_INTERVAL_THREE 0x00000004L
#define D3DPRESENT_INTERVAL_FOUR 0x00000008L
#define D3DPRESENT_INTERVAL_IMMEDIATE 0x80000000L
#define D3DPRESENTFLAG_LOCKABLE_BACKBUFFER 0x00000001
#define D3DPRESENTFLAG_DISCARD_DEPTHSTENCIL 0x00000002
#define D3DCOLORWRITEENABLE_RED (1L << 0)
#define D3DCOLORWRITEENABLE_GREEN (1L << 1)
#define D3DCOLORWRITEENABLE_BLUE (1L << 2)
#define D3DCOLORWRITEENABLE_ALPHA (1L << 3)
#define D3DFVF_RESERVED0 0x001
#define D3DFVF_POSITION_MASK 0x400E
#define D3DFVF_XYZ 0x002
#define D3DFVF_XYZRHW 0x004
#define D3DFVF_XYZB1 0x006
#define D3DFVF_NORMAL 0x010
#define D3DFVF_PSIZE 0x020
#define D3DFVF_DIFFUSE 0x040
#define D3DFVF_SPECULAR 0x080
#define D3DFVF_TEXCOUNT_MASK 0xf00
#define D3DFVF_TEXCOUNT_SHIFT 8
#define D3DFVF_TEX0 0x000
#define D3DFVF_TEX1 0x100
#define D3DFVF_TEX2 0x200
#define D3DFVF_TEX3 0x300
#define D3DFVF_TEX4 0x400
#define D3DDEVCAPS_HWTRANSFORMANDLIGHT 0x00010000L
#define D3DDEVCAPS_PUREDEVICE 0x00100000L
#define D3DPMISCCAPS_NULLREFERENCE 0x00001000L
#define D3DPTFILTERCAPS_MINFANISOTROPIC 0x00000400L
#define D3DPTFILTERCAPS_MAGFANISOTROPIC 0x04000000L
#define D3DPTFILTERCAPS_MINFLINEAR 0x00000200L
#define D3DPTFILTERCAPS_MAGFLINEAR 0x02000000L
#define D3DPTFILTERCAPS_MIPFLINEAR 0x00020000L
#define D3DCAPS2_CANAUTOGENMIPMAP 0x40000000L
#define D3DCAPS2_FULLSCREENGAMMA 0x00020000L
#define D3DVS_VERSION(major, minor) (0xFFFE0000 | ((major) << 8) | (minor))
#define D3DPS_VERSION(major, minor) (0xFFFF0000 | ((major) << 8) | (minor))
#define D3DSHADER_VERSION_MAJOR(version) (((version) >> 8) & 0xFF)
#define D3DSHADER_VERSION_MINOR(version) (((version) >> 0) & 0xFF)
#define D3DSGR_NO_CALIBRATION 0x00000000L
#define D3DENUM_WHQL_LEVEL 0x00000002L

// ---- HRESULTs ----
#define D3D_OK S_OK
#define D3DERR_WRONGTEXTUREFORMAT MAKE_D3DHRESULT(2072)
#define D3DERR_UNSUPPORTEDCOLOROPERATION MAKE_D3DHRESULT(2073)
#define D3DERR_DRIVERINTERNALERROR MAKE_D3DHRESULT(2087)
#define D3DERR_NOTFOUND MAKE_D3DHRESULT(2150)
#define D3DERR_MOREDATA MAKE_D3DHRESULT(2151)
#define D3DERR_DEVICELOST MAKE_D3DHRESULT(2152)
#define D3DERR_DEVICENOTRESET MAKE_D3DHRESULT(2153)
#define D3DERR_NOTAVAILABLE MAKE_D3DHRESULT(2154)
#define D3DERR_OUTOFVIDEOMEMORY MAKE_D3DHRESULT(380)
#define D3DERR_INVALIDDEVICE MAKE_D3DHRESULT(2155)
#define D3DERR_INVALIDCALL MAKE_D3DHRESULT(2156)

// ---- structs ----
typedef struct _D3DVECTOR { float x, y, z; } D3DVECTOR;
typedef struct _D3DCOLORVALUE { float r, g, b, a; } D3DCOLORVALUE;
typedef struct _D3DRECT { LONG x1, y1, x2, y2; } D3DRECT;
typedef struct _D3DMATRIX {
    union {
        struct { float _11, _12, _13, _14, _21, _22, _23, _24, _31, _32, _33, _34, _41, _42, _43, _44; };
        float m[4][4];
    };
} D3DMATRIX;
typedef struct _D3DVIEWPORT9 { DWORD X, Y, Width, Height; float MinZ, MaxZ; } D3DVIEWPORT9;
typedef struct _D3DCLIPSTATUS9 { DWORD ClipUnion, ClipIntersection; } D3DCLIPSTATUS9;
typedef struct _D3DMATERIAL9 { D3DCOLORVALUE Diffuse, Ambient, Specular, Emissive; float Power; } D3DMATERIAL9;
typedef struct _D3DLIGHT9 {
    D3DLIGHTTYPE Type; D3DCOLORVALUE Diffuse, Specular, Ambient; D3DVECTOR Position, Direction;
    float Range, Falloff, Attenuation0, Attenuation1, Attenuation2, Theta, Phi;
} D3DLIGHT9;
typedef struct _D3DVERTEXELEMENT9 { WORD Stream; WORD Offset; BYTE Type; BYTE Method; BYTE Usage; BYTE UsageIndex; } D3DVERTEXELEMENT9, *LPD3DVERTEXELEMENT9;
typedef struct _D3DDISPLAYMODE { UINT Width, Height, RefreshRate; D3DFORMAT Format; } D3DDISPLAYMODE;
typedef struct _D3DDEVICE_CREATION_PARAMETERS { UINT AdapterOrdinal; D3DDEVTYPE DeviceType; HWND hFocusWindow; DWORD BehaviorFlags; } D3DDEVICE_CREATION_PARAMETERS;
typedef struct _D3DPRESENT_PARAMETERS_ {
    UINT BackBufferWidth, BackBufferHeight; D3DFORMAT BackBufferFormat; UINT BackBufferCount;
    D3DMULTISAMPLE_TYPE MultiSampleType; DWORD MultiSampleQuality; D3DSWAPEFFECT SwapEffect; HWND hDeviceWindow;
    BOOL Windowed; BOOL EnableAutoDepthStencil; D3DFORMAT AutoDepthStencilFormat; DWORD Flags;
    UINT FullScreen_RefreshRateInHz; UINT PresentationInterval;
} D3DPRESENT_PARAMETERS;
typedef struct _D3DGAMMARAMP { WORD red[256], green[256], blue[256]; } D3DGAMMARAMP;
typedef struct _D3DRASTER_STATUS { BOOL InVBlank; UINT ScanLine; } D3DRASTER_STATUS;
typedef struct _D3DSURFACE_DESC {
    D3DFORMAT Format; D3DRESOURCETYPE Type; DWORD Usage; D3DPOOL Pool; D3DMULTISAMPLE_TYPE MultiSampleType;
    DWORD MultiSampleQuality; UINT Width, Height;
} D3DSURFACE_DESC;
typedef struct _D3DVERTEXBUFFER_DESC { D3DFORMAT Format; D3DRESOURCETYPE Type; DWORD Usage; D3DPOOL Pool; UINT Size; DWORD FVF; } D3DVERTEXBUFFER_DESC;
typedef struct _D3DINDEXBUFFER_DESC { D3DFORMAT Format; D3DRESOURCETYPE Type; DWORD Usage; D3DPOOL Pool; UINT Size; } D3DINDEXBUFFER_DESC;
typedef struct _D3DVOLUME_DESC { D3DFORMAT Format; D3DRESOURCETYPE Type; DWORD Usage; D3DPOOL Pool; UINT Width, Height, Depth; } D3DVOLUME_DESC;
typedef struct _D3DLOCKED_RECT { INT Pitch; void* pBits; } D3DLOCKED_RECT;
typedef struct _D3DBOX { UINT Left, Top, Right, Bottom, Front, Back; } D3DBOX;
typedef struct _D3DLOCKED_BOX { INT RowPitch; INT SlicePitch; void* pBits; } D3DLOCKED_BOX;
typedef struct _D3DADAPTER_IDENTIFIER9 {
    char Driver[MAX_DEVICE_IDENTIFIER_STRING]; char Description[MAX_DEVICE_IDENTIFIER_STRING]; char DeviceName[32];
    LARGE_INTEGER DriverVersion; DWORD VendorId, DeviceId, SubSysId, Revision; GUID DeviceIdentifier; DWORD WHQLLevel;
} D3DADAPTER_IDENTIFIER9;
typedef struct _D3DVSHADERCAPS2_0 { DWORD Caps; INT DynamicFlowControlDepth; INT NumTemps; INT StaticFlowControlDepth; } D3DVSHADERCAPS2_0;
typedef struct _D3DPSHADERCAPS2_0 { DWORD Caps; INT DynamicFlowControlDepth; INT NumTemps; INT StaticFlowControlDepth; INT NumInstructionSlots; } D3DPSHADERCAPS2_0;
typedef struct _D3DCAPS9 {
    D3DDEVTYPE DeviceType; UINT AdapterOrdinal;
    DWORD Caps, Caps2, Caps3, PresentationIntervals, CursorCaps, DevCaps, PrimitiveMiscCaps, RasterCaps, ZCmpCaps;
    DWORD SrcBlendCaps, DestBlendCaps, AlphaCmpCaps, ShadeCaps, TextureCaps, TextureFilterCaps, CubeTextureFilterCaps;
    DWORD VolumeTextureFilterCaps, TextureAddressCaps, VolumeTextureAddressCaps, LineCaps;
    DWORD MaxTextureWidth, MaxTextureHeight, MaxVolumeExtent, MaxTextureRepeat, MaxTextureAspectRatio, MaxAnisotropy;
    float MaxVertexW, GuardBandLeft, GuardBandTop, GuardBandRight, GuardBandBottom, ExtentsAdjust;
    DWORD StencilCaps, FVFCaps, TextureOpCaps, MaxTextureBlendStages, MaxSimultaneousTextures, VertexProcessingCaps;
    DWORD MaxActiveLights, MaxUserClipPlanes, MaxVertexBlendMatrices, MaxVertexBlendMatrixIndex;
    float MaxPointSize;
    DWORD MaxPrimitiveCount, MaxVertexIndex, MaxStreams, MaxStreamStride, VertexShaderVersion, MaxVertexShaderConst;
    DWORD PixelShaderVersion;
    float PixelShader1xMaxValue;
    DWORD DevCaps2;
    float MaxNpatchTessellationLevel;
    DWORD Reserved5;
    UINT MasterAdapterOrdinal, AdapterOrdinalInGroup, NumberOfAdaptersInGroup;
    DWORD DeclTypes, NumSimultaneousRTs, StretchRectFilterCaps;
    D3DVSHADERCAPS2_0 VS20Caps;
    D3DPSHADERCAPS2_0 PS20Caps;
    DWORD VertexTextureFilterCaps, MaxVShaderInstructionsExecuted, MaxPShaderInstructionsExecuted;
    DWORD MaxVertexShader30InstructionSlots, MaxPixelShader30InstructionSlots;
} D3DCAPS9;
struct D3DRECTPATCH_INFO;
struct D3DTRIPATCH_INFO;

// ---- interfaces (vtable order of the D3D9 API) ----
struct IDirect3D9; struct IDirect3DDevice9; struct IDirect3DStateBlock9; struct IDirect3DSwapChain9; struct IDirect3DResource9;
struct IDirect3DVertexDeclaration9; struct IDirect3DVertexShader9; struct IDirect3DPixelShader9; struct IDirect3DBaseTexture9;
struct IDirect3DTexture9; struct IDirect3DVolumeTexture9; struct IDirect3DCubeTexture9; struct IDirect3DVertexBuffer9;
struct IDirect3DIndexBuffer9; struct IDirect3DSurface9; struct IDirect3DVolume9; struct IDirect3DQuery9;

#define MB_D3DM(ret, name) virtual ret STDMETHODCALLTYPE name

struct IDirect3D9 : public IUnknown {
    MB_D3DM(HRESULT, RegisterSoftwareDevice)(void* pInitializeFunction) = 0;
    MB_D3DM(UINT, GetAdapterCount)() = 0;
    MB_D3DM(HRESULT, GetAdapterIdentifier)(UINT Adapter, DWORD Flags, D3DADAPTER_IDENTIFIER9* pIdentifier) = 0;
    MB_D3DM(UINT, GetAdapterModeCount)(UINT Adapter, D3DFORMAT Format) = 0;
    MB_D3DM(HRESULT, EnumAdapterModes)(UINT Adapter, D3DFORMAT Format, UINT Mode, D3DDISPLAYMODE* pMode) = 0;
    MB_D3DM(HRESULT, GetAdapterDisplayMode)(UINT Adapter, D3DDISPLAYMODE* pMode) = 0;
    MB_D3DM(HRESULT, CheckDeviceType)(UINT Adapter, D3DDEVTYPE DevType, D3DFORMAT AdapterFormat, D3DFORMAT BackBufferFormat, BOOL bWindowed) = 0;
    MB_D3DM(HRESULT, CheckDeviceFormat)(UINT Adapter, D3DDEVTYPE DeviceType, D3DFORMAT AdapterFormat, DWORD Usage, D3DRESOURCETYPE RType, D3DFORMAT CheckFormat) = 0;
    MB_D3DM(HRESULT, CheckDeviceMultiSampleType)(UINT Adapter, D3DDEVTYPE DeviceType, D3DFORMAT SurfaceFormat, BOOL Windowed, D3DMULTISAMPLE_TYPE MultiSampleType, DWORD* pQualityLevels) = 0;
    MB_D3DM(HRESULT, CheckDepthStencilMatch)(UINT Adapter, D3DDEVTYPE DeviceType, D3DFORMAT AdapterFormat, D3DFORMAT RenderTargetFormat, D3DFORMAT DepthStencilFormat) = 0;
    MB_D3DM(HRESULT, CheckDeviceFormatConversion)(UINT Adapter, D3DDEVTYPE DeviceType, D3DFORMAT SourceFormat, D3DFORMAT TargetFormat) = 0;
    MB_D3DM(HRESULT, GetDeviceCaps)(UINT Adapter, D3DDEVTYPE DeviceType, D3DCAPS9* pCaps) = 0;
    MB_D3DM(HMONITOR, GetAdapterMonitor)(UINT Adapter) = 0;
    MB_D3DM(HRESULT, CreateDevice)(UINT Adapter, D3DDEVTYPE DeviceType, HWND hFocusWindow, DWORD BehaviorFlags, D3DPRESENT_PARAMETERS* pPresentationParameters, IDirect3DDevice9** ppReturnedDeviceInterface) = 0;
};

struct IDirect3DDevice9 : public IUnknown {
    MB_D3DM(HRESULT, TestCooperativeLevel)() = 0;
    MB_D3DM(UINT, GetAvailableTextureMem)() = 0;
    MB_D3DM(HRESULT, EvictManagedResources)() = 0;
    MB_D3DM(HRESULT, GetDirect3D)(IDirect3D9** ppD3D9) = 0;
    MB_D3DM(HRESULT, GetDeviceCaps)(D3DCAPS9* pCaps) = 0;
    MB_D3DM(HRESULT, GetDisplayMode)(UINT iSwapChain, D3DDISPLAYMODE* pMode) = 0;
    MB_D3DM(HRESULT, GetCreationParameters)(D3DDEVICE_CREATION_PARAMETERS* pParameters) = 0;
    MB_D3DM(HRESULT, SetCursorProperties)(UINT XHotSpot, UINT YHotSpot, IDirect3DSurface9* pCursorBitmap) = 0;
    MB_D3DM(void, SetCursorPosition)(int X, int Y, DWORD Flags) = 0;
    MB_D3DM(BOOL, ShowCursor)(BOOL bShow) = 0;
    MB_D3DM(HRESULT, CreateAdditionalSwapChain)(D3DPRESENT_PARAMETERS* pPresentationParameters, IDirect3DSwapChain9** pSwapChain) = 0;
    MB_D3DM(HRESULT, GetSwapChain)(UINT iSwapChain, IDirect3DSwapChain9** pSwapChain) = 0;
    MB_D3DM(UINT, GetNumberOfSwapChains)() = 0;
    MB_D3DM(HRESULT, Reset)(D3DPRESENT_PARAMETERS* pPresentationParameters) = 0;
    MB_D3DM(HRESULT, Present)(CONST RECT* pSourceRect, CONST RECT* pDestRect, HWND hDestWindowOverride, CONST RGNDATA* pDirtyRegion) = 0;
    MB_D3DM(HRESULT, GetBackBuffer)(UINT iSwapChain, UINT iBackBuffer, D3DBACKBUFFER_TYPE Type, IDirect3DSurface9** ppBackBuffer) = 0;
    MB_D3DM(HRESULT, GetRasterStatus)(UINT iSwapChain, D3DRASTER_STATUS* pRasterStatus) = 0;
    MB_D3DM(HRESULT, SetDialogBoxMode)(BOOL bEnableDialogs) = 0;
    MB_D3DM(void, SetGammaRamp)(UINT iSwapChain, DWORD Flags, CONST D3DGAMMARAMP* pRamp) = 0;
    MB_D3DM(void, GetGammaRamp)(UINT iSwapChain, D3DGAMMARAMP* pRamp) = 0;
    MB_D3DM(HRESULT, CreateTexture)(UINT Width, UINT Height, UINT Levels, DWORD Usage, D3DFORMAT Format, D3DPOOL Pool, IDirect3DTexture9** ppTexture, HANDLE* pSharedHandle) = 0;
    MB_D3DM(HRESULT, CreateVolumeTexture)(UINT Width, UINT Height, UINT Depth, UINT Levels, DWORD Usage, D3DFORMAT Format, D3DPOOL Pool, IDirect3DVolumeTexture9** ppVolumeTexture, HANDLE* pSharedHandle) = 0;
    MB_D3DM(HRESULT, CreateCubeTexture)(UINT EdgeLength, UINT Levels, DWORD Usage, D3DFORMAT Format, D3DPOOL Pool, IDirect3DCubeTexture9** ppCubeTexture, HANDLE* pSharedHandle) = 0;
    MB_D3DM(HRESULT, CreateVertexBuffer)(UINT Length, DWORD Usage, DWORD FVF, D3DPOOL Pool, IDirect3DVertexBuffer9** ppVertexBuffer, HANDLE* pSharedHandle) = 0;
    MB_D3DM(HRESULT, CreateIndexBuffer)(UINT Length, DWORD Usage, D3DFORMAT Format, D3DPOOL Pool, IDirect3DIndexBuffer9** ppIndexBuffer, HANDLE* pSharedHandle) = 0;
    MB_D3DM(HRESULT, CreateRenderTarget)(UINT Width, UINT Height, D3DFORMAT Format, D3DMULTISAMPLE_TYPE MultiSample, DWORD MultisampleQuality, BOOL Lockable, IDirect3DSurface9** ppSurface, HANDLE* pSharedHandle) = 0;
    MB_D3DM(HRESULT, CreateDepthStencilSurface)(UINT Width, UINT Height, D3DFORMAT Format, D3DMULTISAMPLE_TYPE MultiSample, DWORD MultisampleQuality, BOOL Discard, IDirect3DSurface9** ppSurface, HANDLE* pSharedHandle) = 0;
    MB_D3DM(HRESULT, UpdateSurface)(IDirect3DSurface9* pSourceSurface, CONST RECT* pSourceRect, IDirect3DSurface9* pDestinationSurface, CONST POINT* pDestPoint) = 0;
    MB_D3DM(HRESULT, UpdateTexture)(IDirect3DBaseTexture9* pSourceTexture, IDirect3DBaseTexture9* pDestinationTexture) = 0;
    MB_D3DM(HRESULT, GetRenderTargetData)(IDirect3DSurface9* pRenderTarget, IDirect3DSurface9* pDestSurface) = 0;
    MB_D3DM(HRESULT, GetFrontBufferData)(UINT iSwapChain, IDirect3DSurface9* pDestSurface) = 0;
    MB_D3DM(HRESULT, StretchRect)(IDirect3DSurface9* pSourceSurface, CONST RECT* pSourceRect, IDirect3DSurface9* pDestSurface, CONST RECT* pDestRect, D3DTEXTUREFILTERTYPE Filter) = 0;
    MB_D3DM(HRESULT, ColorFill)(IDirect3DSurface9* pSurface, CONST RECT* pRect, D3DCOLOR color) = 0;
    MB_D3DM(HRESULT, CreateOffscreenPlainSurface)(UINT Width, UINT Height, D3DFORMAT Format, D3DPOOL Pool, IDirect3DSurface9** ppSurface, HANDLE* pSharedHandle) = 0;
    MB_D3DM(HRESULT, SetRenderTarget)(DWORD RenderTargetIndex, IDirect3DSurface9* pRenderTarget) = 0;
    MB_D3DM(HRESULT, GetRenderTarget)(DWORD RenderTargetIndex, IDirect3DSurface9** ppRenderTarget) = 0;
    MB_D3DM(HRESULT, SetDepthStencilSurface)(IDirect3DSurface9* pNewZStencil) = 0;
    MB_D3DM(HRESULT, GetDepthStencilSurface)(IDirect3DSurface9** ppZStencilSurface) = 0;
    MB_D3DM(HRESULT, BeginScene)() = 0;
    MB_D3DM(HRESULT, EndScene)() = 0;
    MB_D3DM(HRESULT, Clear)(DWORD Count, CONST D3DRECT* pRects, DWORD Flags, D3DCOLOR Color, float Z, DWORD Stencil) = 0;
    MB_D3DM(HRESULT, SetTransform)(D3DTRANSFORMSTATETYPE State, CONST D3DMATRIX* pMatrix) = 0;
    MB_D3DM(HRESULT, GetTransform)(D3DTRANSFORMSTATETYPE State, D3DMATRIX* pMatrix) = 0;
    MB_D3DM(HRESULT, MultiplyTransform)(D3DTRANSFORMSTATETYPE, CONST D3DMATRIX*) = 0;
    MB_D3DM(HRESULT, SetViewport)(CONST D3DVIEWPORT9* pViewport) = 0;
    MB_D3DM(HRESULT, GetViewport)(D3DVIEWPORT9* pViewport) = 0;
    MB_D3DM(HRESULT, SetMaterial)(CONST D3DMATERIAL9* pMaterial) = 0;
    MB_D3DM(HRESULT, GetMaterial)(D3DMATERIAL9* pMaterial) = 0;
    MB_D3DM(HRESULT, SetLight)(DWORD Index, CONST D3DLIGHT9*) = 0;
    MB_D3DM(HRESULT, GetLight)(DWORD Index, D3DLIGHT9*) = 0;
    MB_D3DM(HRESULT, LightEnable)(DWORD Index, BOOL Enable) = 0;
    MB_D3DM(HRESULT, GetLightEnable)(DWORD Index, BOOL* pEnable) = 0;
    MB_D3DM(HRESULT, SetClipPlane)(DWORD Index, CONST float* pPlane) = 0;
    MB_D3DM(HRESULT, GetClipPlane)(DWORD Index, float* pPlane) = 0;
    MB_D3DM(HRESULT, SetRenderState)(D3DRENDERSTATETYPE State, DWORD Value) = 0;
    MB_D3DM(HRESULT, GetRenderState)(D3DRENDERSTATETYPE State, DWORD* pValue) = 0;
    MB_D3DM(HRESULT, CreateStateBlock)(D3DSTATEBLOCKTYPE Type, IDirect3DStateBlock9** ppSB) = 0;
    MB_D3DM(HRESULT, BeginStateBlock)() = 0;
    MB_D3DM(HRESULT, EndStateBlock)(IDirect3DStateBlock9** ppSB) = 0;
    MB_D3DM(HRESULT, SetClipStatus)(CONST D3DCLIPSTATUS9* pClipStatus) = 0;
    MB_D3DM(HRESULT, GetClipStatus)(D3DCLIPSTATUS9* pClipStatus) = 0;
    MB_D3DM(HRESULT, GetTexture)(DWORD Stage, IDirect3DBaseTexture9** ppTexture) = 0;
    MB_D3DM(HRESULT, SetTexture)(DWORD Stage, IDirect3DBaseTexture9* pTexture) = 0;
    MB_D3DM(HRESULT, GetTextureStageState)(DWORD Stage, D3DTEXTURESTAGESTATETYPE Type, DWORD* pValue) = 0;
    MB_D3DM(HRESULT, SetTextureStageState)(DWORD Stage, D3DTEXTURESTAGESTATETYPE Type, DWORD Value) = 0;
    MB_D3DM(HRESULT, GetSamplerState)(DWORD Sampler, D3DSAMPLERSTATETYPE Type, DWORD* pValue) = 0;
    MB_D3DM(HRESULT, SetSamplerState)(DWORD Sampler, D3DSAMPLERSTATETYPE Type, DWORD Value) = 0;
    MB_D3DM(HRESULT, ValidateDevice)(DWORD* pNumPasses) = 0;
    MB_D3DM(HRESULT, SetPaletteEntries)(UINT PaletteNumber, CONST PALETTEENTRY* pEntries) = 0;
    MB_D3DM(HRESULT, GetPaletteEntries)(UINT PaletteNumber, PALETTEENTRY* pEntries) = 0;
    MB_D3DM(HRESULT, SetCurrentTexturePalette)(UINT PaletteNumber) = 0;
    MB_D3DM(HRESULT, GetCurrentTexturePalette)(UINT* PaletteNumber) = 0;
    MB_D3DM(HRESULT, SetScissorRect)(CONST RECT* pRect) = 0;
    MB_D3DM(HRESULT, GetScissorRect)(RECT* pRect) = 0;
    MB_D3DM(HRESULT, SetSoftwareVertexProcessing)(BOOL bSoftware) = 0;
    MB_D3DM(BOOL, GetSoftwareVertexProcessing)() = 0;
    MB_D3DM(HRESULT, SetNPatchMode)(float nSegments) = 0;
    MB_D3DM(float, GetNPatchMode)() = 0;
    MB_D3DM(HRESULT, DrawPrimitive)(D3DPRIMITIVETYPE PrimitiveType, UINT StartVertex, UINT PrimitiveCount) = 0;
    MB_D3DM(HRESULT, DrawIndexedPrimitive)(D3DPRIMITIVETYPE, INT BaseVertexIndex, UINT MinVertexIndex, UINT NumVertices, UINT startIndex, UINT primCount) = 0;
    MB_D3DM(HRESULT, DrawPrimitiveUP)(D3DPRIMITIVETYPE PrimitiveType, UINT PrimitiveCount, CONST void* pVertexStreamZeroData, UINT VertexStreamZeroStride) = 0;
    MB_D3DM(HRESULT, DrawIndexedPrimitiveUP)(D3DPRIMITIVETYPE PrimitiveType, UINT MinVertexIndex, UINT NumVertices, UINT PrimitiveCount, CONST void* pIndexData, D3DFORMAT IndexDataFormat, CONST void* pVertexStreamZeroData, UINT VertexStreamZeroStride) = 0;
    MB_D3DM(HRESULT, ProcessVertices)(UINT SrcStartIndex, UINT DestIndex, UINT VertexCount, IDirect3DVertexBuffer9* pDestBuffer, IDirect3DVertexDeclaration9* pVertexDecl, DWORD Flags) = 0;
    MB_D3DM(HRESULT, CreateVertexDeclaration)(CONST D3DVERTEXELEMENT9* pVertexElements, IDirect3DVertexDeclaration9** ppDecl) = 0;
    MB_D3DM(HRESULT, SetVertexDeclaration)(IDirect3DVertexDeclaration9* pDecl) = 0;
    MB_D3DM(HRESULT, GetVertexDeclaration)(IDirect3DVertexDeclaration9** ppDecl) = 0;
    MB_D3DM(HRESULT, SetFVF)(DWORD FVF) = 0;
    MB_D3DM(HRESULT, GetFVF)(DWORD* pFVF) = 0;
    MB_D3DM(HRESULT, CreateVertexShader)(CONST DWORD* pFunction, IDirect3DVertexShader9** ppShader) = 0;
    MB_D3DM(HRESULT, SetVertexShader)(IDirect3DVertexShader9* pShader) = 0;
    MB_D3DM(HRESULT, GetVertexShader)(IDirect3DVertexShader9** ppShader) = 0;
    MB_D3DM(HRESULT, SetVertexShaderConstantF)(UINT StartRegister, CONST float* pConstantData, UINT Vector4fCount) = 0;
    MB_D3DM(HRESULT, GetVertexShaderConstantF)(UINT StartRegister, float* pConstantData, UINT Vector4fCount) = 0;
    MB_D3DM(HRESULT, SetVertexShaderConstantI)(UINT StartRegister, CONST int* pConstantData, UINT Vector4iCount) = 0;
    MB_D3DM(HRESULT, GetVertexShaderConstantI)(UINT StartRegister, int* pConstantData, UINT Vector4iCount) = 0;
    MB_D3DM(HRESULT, SetVertexShaderConstantB)(UINT StartRegister, CONST BOOL* pConstantData, UINT BoolCount) = 0;
    MB_D3DM(HRESULT, GetVertexShaderConstantB)(UINT StartRegister, BOOL* pConstantData, UINT BoolCount) = 0;
    MB_D3DM(HRESULT, SetStreamSource)(UINT StreamNumber, IDirect3DVertexBuffer9* pStreamData, UINT OffsetInBytes, UINT Stride) = 0;
    MB_D3DM(HRESULT, GetStreamSource)(UINT StreamNumber, IDirect3DVertexBuffer9** ppStreamData, UINT* pOffsetInBytes, UINT* pStride) = 0;
    MB_D3DM(HRESULT, SetStreamSourceFreq)(UINT StreamNumber, UINT Setting) = 0;
    MB_D3DM(HRESULT, GetStreamSourceFreq)(UINT StreamNumber, UINT* pSetting) = 0;
    MB_D3DM(HRESULT, SetIndices)(IDirect3DIndexBuffer9* pIndexData) = 0;
    MB_D3DM(HRESULT, GetIndices)(IDirect3DIndexBuffer9** ppIndexData) = 0;
    MB_D3DM(HRESULT, CreatePixelShader)(CONST DWORD* pFunction, IDirect3DPixelShader9** ppShader) = 0;
    MB_D3DM(HRESULT, SetPixelShader)(IDirect3DPixelShader9* pShader) = 0;
    MB_D3DM(HRESULT, GetPixelShader)(IDirect3DPixelShader9** ppShader) = 0;
    MB_D3DM(HRESULT, SetPixelShaderConstantF)(UINT StartRegister, CONST float* pConstantData, UINT Vector4fCount) = 0;
    MB_D3DM(HRESULT, GetPixelShaderConstantF)(UINT StartRegister, float* pConstantData, UINT Vector4fCount) = 0;
    MB_D3DM(HRESULT, SetPixelShaderConstantI)(UINT StartRegister, CONST int* pConstantData, UINT Vector4iCount) = 0;
    MB_D3DM(HRESULT, GetPixelShaderConstantI)(UINT StartRegister, int* pConstantData, UINT Vector4iCount) = 0;
    MB_D3DM(HRESULT, SetPixelShaderConstantB)(UINT StartRegister, CONST BOOL* pConstantData, UINT BoolCount) = 0;
    MB_D3DM(HRESULT, GetPixelShaderConstantB)(UINT StartRegister, BOOL* pConstantData, UINT BoolCount) = 0;
    MB_D3DM(HRESULT, DrawRectPatch)(UINT Handle, CONST float* pNumSegs, CONST D3DRECTPATCH_INFO* pRectPatchInfo) = 0;
    MB_D3DM(HRESULT, DrawTriPatch)(UINT Handle, CONST float* pNumSegs, CONST D3DTRIPATCH_INFO* pTriPatchInfo) = 0;
    MB_D3DM(HRESULT, DeletePatch)(UINT Handle) = 0;
    MB_D3DM(HRESULT, CreateQuery)(D3DQUERYTYPE Type, IDirect3DQuery9** ppQuery) = 0;
};

struct IDirect3DStateBlock9 : public IUnknown {
    MB_D3DM(HRESULT, GetDevice)(IDirect3DDevice9** ppDevice) = 0;
    MB_D3DM(HRESULT, Capture)() = 0;
    MB_D3DM(HRESULT, Apply)() = 0;
};
struct IDirect3DSwapChain9 : public IUnknown {
    MB_D3DM(HRESULT, Present)(CONST RECT* pSourceRect, CONST RECT* pDestRect, HWND hDestWindowOverride, CONST RGNDATA* pDirtyRegion, DWORD dwFlags) = 0;
    MB_D3DM(HRESULT, GetFrontBufferData)(IDirect3DSurface9* pDestSurface) = 0;
    MB_D3DM(HRESULT, GetBackBuffer)(UINT iBackBuffer, D3DBACKBUFFER_TYPE Type, IDirect3DSurface9** ppBackBuffer) = 0;
    MB_D3DM(HRESULT, GetRasterStatus)(D3DRASTER_STATUS* pRasterStatus) = 0;
    MB_D3DM(HRESULT, GetDisplayMode)(D3DDISPLAYMODE* pMode) = 0;
    MB_D3DM(HRESULT, GetDevice)(IDirect3DDevice9** ppDevice) = 0;
    MB_D3DM(HRESULT, GetPresentParameters)(D3DPRESENT_PARAMETERS* pPresentationParameters) = 0;
};
struct IDirect3DResource9 : public IUnknown {
    MB_D3DM(HRESULT, GetDevice)(IDirect3DDevice9** ppDevice) = 0;
    MB_D3DM(HRESULT, SetPrivateData)(REFGUID refguid, CONST void* pData, DWORD SizeOfData, DWORD Flags) = 0;
    MB_D3DM(HRESULT, GetPrivateData)(REFGUID refguid, void* pData, DWORD* pSizeOfData) = 0;
    MB_D3DM(HRESULT, FreePrivateData)(REFGUID refguid) = 0;
    MB_D3DM(DWORD, SetPriority)(DWORD PriorityNew) = 0;
    MB_D3DM(DWORD, GetPriority)() = 0;
    MB_D3DM(void, PreLoad)() = 0;
    MB_D3DM(D3DRESOURCETYPE, GetType)() = 0;
};
struct IDirect3DVertexDeclaration9 : public IUnknown {
    MB_D3DM(HRESULT, GetDevice)(IDirect3DDevice9** ppDevice) = 0;
    MB_D3DM(HRESULT, GetDeclaration)(D3DVERTEXELEMENT9* pElement, UINT* pNumElements) = 0;
};
struct IDirect3DVertexShader9 : public IUnknown {
    MB_D3DM(HRESULT, GetDevice)(IDirect3DDevice9** ppDevice) = 0;
    MB_D3DM(HRESULT, GetFunction)(void*, UINT* pSizeOfData) = 0;
};
struct IDirect3DPixelShader9 : public IUnknown {
    MB_D3DM(HRESULT, GetDevice)(IDirect3DDevice9** ppDevice) = 0;
    MB_D3DM(HRESULT, GetFunction)(void*, UINT* pSizeOfData) = 0;
};
struct IDirect3DBaseTexture9 : public IDirect3DResource9 {
    MB_D3DM(DWORD, SetLOD)(DWORD LODNew) = 0;
    MB_D3DM(DWORD, GetLOD)() = 0;
    MB_D3DM(DWORD, GetLevelCount)() = 0;
    MB_D3DM(HRESULT, SetAutoGenFilterType)(D3DTEXTUREFILTERTYPE FilterType) = 0;
    MB_D3DM(D3DTEXTUREFILTERTYPE, GetAutoGenFilterType)() = 0;
    MB_D3DM(void, GenerateMipSubLevels)() = 0;
};
struct IDirect3DTexture9 : public IDirect3DBaseTexture9 {
    MB_D3DM(HRESULT, GetLevelDesc)(UINT Level, D3DSURFACE_DESC* pDesc) = 0;
    MB_D3DM(HRESULT, GetSurfaceLevel)(UINT Level, IDirect3DSurface9** ppSurfaceLevel) = 0;
    MB_D3DM(HRESULT, LockRect)(UINT Level, D3DLOCKED_RECT* pLockedRect, CONST RECT* pRect, DWORD Flags) = 0;
    MB_D3DM(HRESULT, UnlockRect)(UINT Level) = 0;
    MB_D3DM(HRESULT, AddDirtyRect)(CONST RECT* pDirtyRect) = 0;
};
struct IDirect3DVolumeTexture9 : public IDirect3DBaseTexture9 {
    MB_D3DM(HRESULT, GetLevelDesc)(UINT Level, D3DVOLUME_DESC* pDesc) = 0;
    MB_D3DM(HRESULT, GetVolumeLevel)(UINT Level, IDirect3DVolume9** ppVolumeLevel) = 0;
    MB_D3DM(HRESULT, LockBox)(UINT Level, D3DLOCKED_BOX* pLockedVolume, CONST D3DBOX* pBox, DWORD Flags) = 0;
    MB_D3DM(HRESULT, UnlockBox)(UINT Level) = 0;
    MB_D3DM(HRESULT, AddDirtyBox)(CONST D3DBOX* pDirtyBox) = 0;
};
struct IDirect3DCubeTexture9 : public IDirect3DBaseTexture9 {
    MB_D3DM(HRESULT, GetLevelDesc)(UINT Level, D3DSURFACE_DESC* pDesc) = 0;
    MB_D3DM(HRESULT, GetCubeMapSurface)(D3DCUBEMAP_FACES FaceType, UINT Level, IDirect3DSurface9** ppCubeMapSurface) = 0;
    MB_D3DM(HRESULT, LockRect)(D3DCUBEMAP_FACES FaceType, UINT Level, D3DLOCKED_RECT* pLockedRect, CONST RECT* pRect, DWORD Flags) = 0;
    MB_D3DM(HRESULT, UnlockRect)(D3DCUBEMAP_FACES FaceType, UINT Level) = 0;
    MB_D3DM(HRESULT, AddDirtyRect)(D3DCUBEMAP_FACES FaceType, CONST RECT* pDirtyRect) = 0;
};
struct IDirect3DVertexBuffer9 : public IDirect3DResource9 {
    MB_D3DM(HRESULT, Lock)(UINT OffsetToLock, UINT SizeToLock, void** ppbData, DWORD Flags) = 0;
    MB_D3DM(HRESULT, Unlock)() = 0;
    MB_D3DM(HRESULT, GetDesc)(D3DVERTEXBUFFER_DESC* pDesc) = 0;
};
struct IDirect3DIndexBuffer9 : public IDirect3DResource9 {
    MB_D3DM(HRESULT, Lock)(UINT OffsetToLock, UINT SizeToLock, void** ppbData, DWORD Flags) = 0;
    MB_D3DM(HRESULT, Unlock)() = 0;
    MB_D3DM(HRESULT, GetDesc)(D3DINDEXBUFFER_DESC* pDesc) = 0;
};
struct IDirect3DSurface9 : public IDirect3DResource9 {
    MB_D3DM(HRESULT, GetContainer)(REFIID riid, void** ppContainer) = 0;
    MB_D3DM(HRESULT, GetDesc)(D3DSURFACE_DESC* pDesc) = 0;
    MB_D3DM(HRESULT, LockRect)(D3DLOCKED_RECT* pLockedRect, CONST RECT* pRect, DWORD Flags) = 0;
    MB_D3DM(HRESULT, UnlockRect)() = 0;
    MB_D3DM(HRESULT, GetDC)(HDC* phdc) = 0;
    MB_D3DM(HRESULT, ReleaseDC)(HDC hdc) = 0;
};
struct IDirect3DVolume9 : public IUnknown {
    MB_D3DM(HRESULT, GetDevice)(IDirect3DDevice9** ppDevice) = 0;
    MB_D3DM(HRESULT, SetPrivateData)(REFGUID refguid, CONST void* pData, DWORD SizeOfData, DWORD Flags) = 0;
    MB_D3DM(HRESULT, GetPrivateData)(REFGUID refguid, void* pData, DWORD* pSizeOfData) = 0;
    MB_D3DM(HRESULT, FreePrivateData)(REFGUID refguid) = 0;
    MB_D3DM(HRESULT, GetContainer)(REFIID riid, void** ppContainer) = 0;
    MB_D3DM(HRESULT, GetDesc)(D3DVOLUME_DESC* pDesc) = 0;
    MB_D3DM(HRESULT, LockBox)(D3DLOCKED_BOX* pLockedVolume, CONST D3DBOX* pBox, DWORD Flags) = 0;
    MB_D3DM(HRESULT, UnlockBox)() = 0;
};
struct IDirect3DQuery9 : public IUnknown {
    MB_D3DM(HRESULT, GetDevice)(IDirect3DDevice9** ppDevice) = 0;
    MB_D3DM(D3DQUERYTYPE, GetType)() = 0;
    MB_D3DM(DWORD, GetDataSize)() = 0;
    MB_D3DM(HRESULT, Issue)(DWORD dwIssueFlags) = 0;
    MB_D3DM(HRESULT, GetData)(void* pData, DWORD dwSize, DWORD dwGetDataFlags) = 0;
};
#undef MB_D3DM

typedef IDirect3D9* LPDIRECT3D9, *PDIRECT3D9;
typedef IDirect3DDevice9* LPDIRECT3DDEVICE9, *PDIRECT3DDEVICE9;
typedef IDirect3DStateBlock9* LPDIRECT3DSTATEBLOCK9;
typedef IDirect3DSwapChain9* LPDIRECT3DSWAPCHAIN9;
typedef IDirect3DResource9* LPDIRECT3DRESOURCE9;
typedef IDirect3DVertexDeclaration9* LPDIRECT3DVERTEXDECLARATION9;
typedef IDirect3DVertexShader9* LPDIRECT3DVERTEXSHADER9;
typedef IDirect3DPixelShader9* LPDIRECT3DPIXELSHADER9;
typedef IDirect3DBaseTexture9* LPDIRECT3DBASETEXTURE9;
typedef IDirect3DTexture9* LPDIRECT3DTEXTURE9, *PDIRECT3DTEXTURE9;
typedef IDirect3DVolumeTexture9* LPDIRECT3DVOLUMETEXTURE9;
typedef IDirect3DCubeTexture9* LPDIRECT3DCUBETEXTURE9;
typedef IDirect3DVertexBuffer9* LPDIRECT3DVERTEXBUFFER9, *PDIRECT3DVERTEXBUFFER9;
typedef IDirect3DIndexBuffer9* LPDIRECT3DINDEXBUFFER9, *PDIRECT3DINDEXBUFFER9;
typedef IDirect3DSurface9* LPDIRECT3DSURFACE9, *PDIRECT3DSURFACE9;
typedef IDirect3DVolume9* LPDIRECT3DVOLUME9;
typedef IDirect3DQuery9* LPDIRECT3DQUERY9;

extern "C" IDirect3D9* WINAPI Direct3DCreate9(UINT SDKVersion);

#endif // MB_PORT_D3D9_H
