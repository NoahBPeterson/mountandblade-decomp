// key_groups.h: the key-code group tables of key_groups.cpp (0x4320b0..0x432350). Key codes are in the Module System's
// header_triggers numbering; ids come in triples (0x97.., 0xc7..0xed).
#ifndef RGL_XMLCFG_KEY_GROUPS_H
#define RGL_XMLCFG_KEY_GROUPS_H

int __cdecl not_group_base(int x);          // 0x4320b0 false for the first code of each triple group_base() returns
int __cdecl group_base(int id);             // 0x432130 first id of the triple, -1 otherwise

#endif
