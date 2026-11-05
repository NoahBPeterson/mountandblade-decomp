# Glue aliases (faithful build)

Each row is a name that source declares and calls, bound by the glue to the function our source defines under another name
at the same original address (`glue.json` "aliases"; tools/relink/glue.py). Regenerate with a faithful build.

Total: 22.  ICF 8  TWIN 0  VIEW 10  OTHER 4

- ICF: identical-code folding in the original linker; two different classes / instantiations share one address. Honest, cannot reach zero.
- TWIN: deliberate visibility twin (`_def` / `_x` / `_ext`, MeshExt, TroopExt, AgentX, allocN). Waiting on a separate decision.
- VIEW: a partial view struct or duplicate type re-declaring something the real class has. Removable; needs the real class threaded through its callers.
- OTHER: a second spelling of one function (calling convention, signature, deleting-destructor or static-member spelling).

| alias (name source calls) | defined as | class | reason |
|---|---|---|---|
| `??4ptr_list@rgl@@QAEAAU01@ABU01@@Z` | `??4?$vector@HV?$allocator@H@std@@@std@@QAEAAV01@ABV01@@Z` | ICF | vector<T*>::operator= is one instance for all pointer element types (0x43f0a0); ptr_list derives from vector<void*> |
| `??Hiter32@rgl@@QBE?AU01@H@Z` | `??Hiterator@?$vector@Upoint20@rgl@@V?$allocator@Upoint20@rgl@@@std@@@std@@QBE?AV012@H@Z` | VIEW | iter32 (scene.h) / contact_inl: hand-rolled iterators for std::vector<point20>::iterator::operator+ (0x40f880) |
| `??Hiterator@contact_inl@@QBE?AU01@H@Z` | `??Hiterator@?$vector@Upoint20@rgl@@V?$allocator@Upoint20@rgl@@@std@@@std@@QBE?AV012@H@Z` | VIEW | iter32 (scene.h) / contact_inl: hand-rolled iterators for std::vector<point20>::iterator::operator+ (0x40f880) |
| `?RecD_dtor@@YIXPAX@Z` | `??1RecD@@QAE@XZ` | OTHER | fastcall free-function spelling of RecD::~RecD (0x431440) for a vector-constructor callback |
| `?_Uninit_fill_n@std@@YAXPAUidx_weight@rgl@@IABU23@AAV?$allocator@Uidx_weight@rgl@@@1@U_Nonscalar_ptr_iterator_tag@1@@Z` | `??$_Uninit_fill_n@PAUE8@@IU1@V?$allocator@UE8@@@std@@@std@@YAXPAUE8@@IABU1@AAV?$allocator@UE8@@@0@U_Nonscal...` | ICF | vector<idx_weight>::_Uninit_fill_n and the E8 instance are identical (0x418150) |
| `?buy@?$vec@Umface@@@@QAE_NI@Z` | `?_Buy@?$vector@Upoint20@rgl@@V?$allocator@Upoint20@rgl@@@std@@@std@@IAE_NI@Z` | VIEW | hand-rolled vec<T> (mface / vector4) in place of std::vector, _Buy 0x4105c0 / 0x42d570 |
| `?buy@?$vec@Uvector4@rgl@@@@QAE_NI@Z` | `?_Buy@?$vector@Uvector4@rgl@@V?$allocator@Uvector4@rgl@@@std@@@std@@IAE_NI@Z` | VIEW | hand-rolled vec<T> (mface / vector4) in place of std::vector, _Buy 0x4105c0 / 0x42d570 |
| `?clear@?$vec@Umface@@@@QAEXXZ` | `?clear@?$vec@UNavVert@@@rgl@@QAEXXZ` | VIEW | hand-rolled vec<T> clear (0x41f4b0, shared with vec<NavVert>) |
| `?clear@?$vec@Uvector4@rgl@@@@QAEXXZ` | `?clear@?$vec@UNavVert@@@rgl@@QAEXXZ` | VIEW | hand-rolled vec<T> clear (0x41f4b0, shared with vec<NavVert>) |
| `?clear@Stack@@QAEXXZ` | `?reset@NamedSelRec@@QAEXXZ` | ICF | Stack::clear and NamedSelRec::reset are different classes with identical bytes (0x4893e0) |
| `?destroy@RecI@@QAEPAXI@Z` | `??_ERecI@@QAEPAXI@Z` | OTHER | RecD / RecI scalar-deleting destructor spelled as a member (0x431480 / 0x431810) |
| `?display_name@ItemKind@@QAEPAUtstring@@XZ` | `?display_name@TroopRec@@QAEPAUtstring@@XZ` | ICF | ItemKind::display_name and TroopRec::display_name are identical (0x43a480) |
| `?erase@?$vec@Umface@@@@QAE?AUiter@1@U21@0@Z` | `?erase@?$vector@Upoint20@rgl@@V?$allocator@Upoint20@rgl@@@std@@@std@@QAE?AViterator@12@V312@0@Z` | VIEW | hand-rolled vec<mface>::erase for std::vector<point20>::erase (0x40fd40) |
| `?identity_pos@rgl@@YA?AUframe_value@1@XZ` | `?identity_pos@rgl@@YAPAUmatrix@1@PAU21@@Z` | OTHER | second prototype of rgl::identity_pos (returns frame_value by hidden pointer vs matrix*), 0x40cd00 |
| `?insert@GlyphMap@@QAEXPAXPBH@Z` | `?insert@HashTab@@QAEXPAUHOut@@PAI@Z` | VIEW | GlyphMap (font.h) hand-rolled hash map vs HashTab / _Hash lookups |
| `?insert_n@IntVec@@QAEXPAHIPBH@Z` | `?_Insert_n@?$vector@PAURecJ@@V?$allocator@PAURecJ@@@std@@@std@@IAEXViterator@12@IABQAURecJ@@@Z` | VIEW | IntVec (int_vec.h) is a vector<int> view; the instance is vector<RecJ*>::_Insert_n (0x43e7b0) |
| `?lower_bound@GlyphMap@@QAEXPAPAUGlyphNode@@PBH@Z` | `?lower_bound@?$_Hash@V?$_Hmap_traits@HUHashVal@@V?$hash_compare@HU?$less@H@std@@@stdext@@V?$allocator@U?$pa...` | VIEW | GlyphMap (font.h) hand-rolled hash map vs HashTab / _Hash lookups |
| `?push_back@mesh_ptr_vec@@QAEXABQAUmesh@rgl@@@Z` | `?push_back@?$vector@PAUscene@rgl@@V?$allocator@PAUscene@rgl@@@std@@@std@@QAEXABQAUscene@rgl@@@Z` | ICF | vector<T*>::push_back is one instance; mesh_ptr_vec : vector<mesh*> (0x4119d0) |
| `?reset@Agent@@QAEXXZ` | `?reset@BigObj@@QAEXXZ` | ICF | Agent::reset and BigObj::reset are different classes with identical bytes (0x432080) |
| `?scalar_delete@RecD@@QAEPAXI@Z` | `??_ERecD@@QAEPAXI@Z` | OTHER | RecD / RecI scalar-deleting destructor spelled as a member (0x431480 / 0x431810) |
| `?set_value@Meter@@QAEXPAM@Z` | `?cap_all@node_groups@@QAEXPBM@Z` | ICF | Meter::set_value and node_groups::cap_all are identical (0x439000) |
| `?uninit_fill_ptrs@@YAXPAPAUMissionUi@@IPBQAU1@PAX@Z` | `??$_Uninit_fill_n@HIH@std@@YAXPAHIABHAAV?$allocator@H@0@U_Scalar_ptr_iterator_tag@0@@Z` | ICF | pointer-element _Uninit_fill_n shares the int instance (0x40f8a0) |
