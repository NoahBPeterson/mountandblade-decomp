#include "./save_state.h"
#include "../../include/wl_marker.h"   // WL_MARKER: the original's inline WinLicense region markers
#include "../skill_cap.h"      // g_skill_tbl (0x8ee3100)
#include "../tstring.h"      // (listed directly: the resolution audit reads only the source's own includes)
#include "../../include/game_data.h"      // (the shared class header named here too: the resolution audit reads the annotations of directly included headers)
#include "../out_stream.h"      // (listed directly: the resolution audit reads only the source's own includes)
#include "../slot_array.h"
#include "../track_rec.h"      // (listed directly: the resolution audit reads only the source's own includes)

#include "../game_globals.h"      // canonical g_party_template_count (listed directly: the audit reads only the source's own includes)
#include "../../include/item_kind.h"   // g_item_kind_count 0x5e29d4
#include "../../include/script_state.h"   // ScriptState g_script (0xa70440): the one object
#include "../terrain_b/scene_file.h"   // SceneFile: the scene slots write_list_party saves
#include "../../rgl/stream_inline.h"   // the stream constructors / destructors inline (the scene record's embedded streams)

// @ 0x4e8f10    int GameData::save_state(OutStream* out)   (thiscall, ret 4)
int GameData::save_state(OutStream* out)
{
    WL_MARKER(12)
    __int64 counter = g_ticks[1];
    out->w_int8((int*)&counter);
    out->w_raw(&f4);
    out->w_raw(&fb4);
    WL_MARKER(13)
    write_clock(out);
    g_script.vars.write(out);
    triggers.write(out);
    simple.write(out);
    write_list138(out);
    write_list_party(out);
    write_list300(out);
    write_vec5c(out);
    out->w_raw(&g_party_template_count);
    for (int i = 0; i < g_party_template_count; i++)
        g_party_templates[i].write(out);
    write_missions(out);
    WL_MARKER(12)
    events.write_table(out);                 // the event pool (save_records.cpp views it as Table)
    WL_MARKER(13)
    write_records(out);
    write_items(out);
    out->w_int8((int*)&records[player].face_keys[0]);
    out->w_int8((int*)&records[player].face_keys[1]);
    out->w_raw(&cnt_killed);
    out->w_raw(&cnt_ko);
    out->w_raw(&cnt_killed_friend);
    out->w_raw(&cnt_ko_friend);
    return 0;
}

// @ 0x4e90d0    void TriggerVec::write(OutStream* out)   (thiscall, ret 4)
void TriggerVec::write(OutStream* out)
{
    out->w_raw(this);
    for (int i = 0; i < count; i++)
        data[i].write(out);
}

// @ 0x4e9110    void TriggerRec::write(OutStream* out)   (thiscall, ret 4)
void TriggerRec::write(OutStream* out)
{
    out->w_raw(&state);
    out->w_i64(&check_timer.start);
    out->w_i64(&delay_timer.start);
    out->w_i64(&rearm_timer.start);
}

// @ 0x4e9150    void SimpleTriggerVec::write(OutStream* out)   (thiscall, ret 4)
void SimpleTriggerVec::write(OutStream* out)
{
    out->w_raw(&count);
    for (int i = 0; i < count; i++)
        out->w_i64(&data[i].timer.start);
}

// @ 0x4e9190    int GameData::write_clock(OutStream* out)   (thiscall, ret 4)
int GameData::write_clock(OutStream* out)
{
    WL_MARKER(12)
    out->w_i64(&game_clock.start);
    out->w_raw(&current_hours);
    out->w_raw(&string_count);
    out->w_raw(&f13b30);
    out->w_raw(&f13b34);
    out->w_raw(&f13b38);
    out->w_raw(&f13b60);
    out->w_int(&i13b3c);
    out->w_int(&i13b40);
    out->w_int(&f13b44);
    out->w_int(&f13b48);
    out->w_struct(&t13a70);
    out->w_int4((int*)&f13aa0);                     // the flag byte and its three pad bytes as one word
    out->w_raw(&f13ae8);
    out->w_raw(&f13a20);
    out->w_raw(&encountered_party);
    out->w_raw(&encountered_party2);
    out->w_raw(&current_menu);
    out->w_raw(&jump_scene);
    out->w_raw(&jump_entry);
    out->w_raw(&jump_mission);
    out->w_raw(&range_lo);
    out->w_raw(&range_hi);
    tstring tmp = message_log;
    tmp.spaces_to_underscores();
    if (!tmp.len) {
        out->w_cstr("-1");
    } else {
        out->w_struct(&tmp);
    }
    out->w_raw(&f1a1d8);
    out->w_raw(&f1a1dc);
    out->w_raw(&f1a1e0);
    out->w_raw(&f1a1e4);
    out->w_raw(&f1a1e8);
    out->w_raw(&f1a1ec);
    WL_MARKER(13)
    out->w_i64(&timer13a80.start);
    out->w_int(&f13ad0);
    out->w_raw(&rest_time_speed);
    out->w_raw(&rest_interactive);
    out->w_raw(&rest_attackable);
    return 0;
}

// @ 0x4e93f0    void GameData::write_vec5c(OutStream* out)   (thiscall, ret 4)   vector at +0x13a64, stride 0x5c
void GameData::write_vec5c(OutStream* out)
{
    int n;
    if (!fleet)
        n = 0;
    else
        n = (int)(fleet_end - fleet) / 0x5c;
    out->w_count(&n);
    for (char* r = fleet; r < fleet_end; r += 0x5c)
        ((TrackRec*)r)->write(out);
}

// @ 0x4e9460    void TrackRec::write(OutStream* s)   (thiscall, ret 4)
void TrackRec::write(OutStream* s)
{
    char* p = (char*)this;
    s->w_int((int*)(p + 0)).w_int((int*)(p + 4)).w_int((int*)(p + 8));
    s->w_int((int*)(p + 0x10));
    s->w_int((int*)(p + 0x14)).w_raw(p + 0x18);
}

// @ 0x4e94b0    void SlotArray::write(OutStream* out)   (thiscall, ret 4)
void SlotArray::write(OutStream* out)
{
    int n = (int)size();
    out->w_count(&n);
    for (unsigned i = 0; i < size(); i++)
        out->w_i64(&(*this)[i]);     // an 8-byte element (OutStream's +0x18 slot)
}

// @ 0x4e9510    void PartyTemplate::write(OutStream* out)   (thiscall, ret 4)
void PartyTemplate::write(OutStream* out)
{
    out->w_raw(&num_created);
    out->w_raw(&num_destroyed);
    out->w_raw(&num_destroyed_by_player);
    slots.write(out);
}

// @ 0x4e9550    void GameData::write_list138(OutStream* out)   (thiscall, ret 4)
void GameData::write_list138(OutStream* out)
{
    out->w_raw(&n13a5c);
    for (int i = 0; i < n13a5c; i++)
        quests[i].write(out);
}

// @ 0x4e95a0    void QuestRec::write(OutStream* out)   (thiscall, ret 4)
void QuestRec::write(OutStream* out)
{
    out->w_raw(&f64);
    out->w_raw(&giver_troop);
    out->w_raw(&number);
    out->w_int((int*)&f11c);
    out->w_struct(&text);
    out->w_struct(&title);
    out->w_struct(&giver);
    write_soa(out, &notes);
    slots.write(out);
}

// @ 0x4e9620    OutStream* write_soa(OutStream* out, SoaBlock* b)   (free function, ret 0)
// The notes block (party/game.h SoaBlock, the reader's read_soa 0x51e1f0): per note its text, its time, then the tableau word.
OutStream* write_soa(OutStream* out, SoaBlock* b)
{
    for (int i = 0; i < 8; i++) {
        int* blk = &b->tableau;
        out->w_struct(&b->texts[i]);
        out->w_raw(&b->times[i]);
        out->w_raw(blk);
    }
    return out;
}

// @ 0x4e9670    void GameData::write_list_party(OutStream* out)   (thiscall, ret 4)
void GameData::write_list_party(OutStream* out)
{
    out->w_raw(&nparties);
    for (int i = 0; i < nparties; i++)
        scenes[i].slots.write(out);
}

// @ 0x4e96c0    void GameData::write_list300(OutStream* out)   (thiscall, ret 4)
void GameData::write_list300(OutStream* out)
{
    out->w_raw(&n13a4c);
    for (int i = 0; i < n13a4c; i++)
        factions[i].write300(out, n13a4c);
}

// @ 0x4e9710    void FactionRec::write300(OutStream* out, int n)   (thiscall, ret 8)
void FactionRec::write300(OutStream* out, int n)
{
    slots.write(out);
    for (int i = 0; i < n; i++)
        out->w_int((int*)&relations[i]);
    out->w_struct(&name);
    out->w_int4((int*)&f10);                         // the flag byte and its three pad bytes as one word
    out->w_count((int*)&f28);
    out->w_raw(&f230);
    write_soa(out, &notes);
}

// @ 0x4e9790    void GameData::write_items(OutStream* out)   (thiscall, ret 4)   item kinds at 0x5e29d0
void GameData::write_items(OutStream* out)
{
    out->w_raw(&g_item_kind_count);
    for (int i = 0; i < g_item_kind_count; i++)
        g_items[i].slots.write(out);
}

// @ 0x4e97e0    void GameData::write_records(OutStream* out)   (thiscall, ret 4)
void GameData::write_records(OutStream* out)
{
    out->w_raw(&nrecords);
    for (int i = 0; i < nrecords; i++)
        records[i].write_save(out);
    for (int i = 0; i < 42; i++)
        out->w_raw(&g_skill_tbl[i].cap);
}