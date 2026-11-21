#include "./save_state.h"
#include "../../include/wl_marker.h"   // WL_MARKER: the original's inline WinLicense region markers
#include "../skill_cap.h"      // g_skill_tbl (0x8ee3100)
#include "../tstring.h"      // (listed directly: the resolution audit reads only the source's own includes)
#include "../../include/game_data.h"      // (the shared class header named here too: the resolution audit reads the annotations of directly included headers)
#include "../../rgl/stream.h"
#include "../slot_array.h"
#include "../track_rec.h"      // (listed directly: the resolution audit reads only the source's own includes)

#include "../game_globals.h"      // canonical g_party_template_count (listed directly: the audit reads only the source's own includes)
#include "../../include/item_kind.h"   // g_item_kind_count 0x5e29d4
#include "../../include/script_state.h"   // ScriptState g_script (0xa70440): the one object
#include "../terrain_b/scene_file.h"   // SceneFile: the scene slots write_list_party saves
#include "../../rgl/stream_inline.h"   // the stream constructors / destructors inline (the scene record's embedded streams)

// @ 0x4e8f10    int GameData::save_state(stream_base* out)   (thiscall, ret 4)
int GameData::save_state(stream_base* out)
{
    WL_MARKER(12)
    unsigned __int64 counter = g_ticks[1];
    out->w_u64(&counter);
    out->w_int(&f4);
    out->w_int(&fb4);
    WL_MARKER(13)
    write_clock(out);
    g_script.vars.write(out);
    triggers.write(out);
    simple.write(out);
    write_list138(out);
    write_list_party(out);
    write_list300(out);
    write_vec5c(out);
    out->w_int(&g_party_template_count);
    for (int i = 0; i < g_party_template_count; i++)
        g_party_templates[i].write(out);
    write_missions(out);
    WL_MARKER(12)
    events.write_table(out);                 // the event pool (save_records.cpp views it as Table)
    WL_MARKER(13)
    write_records(out);
    write_items(out);
    out->w_u64(&records[player].face_keys[0]);
    out->w_u64(&records[player].face_keys[1]);
    out->w_int(&cnt_killed);
    out->w_int(&cnt_ko);
    out->w_int(&cnt_killed_friend);
    out->w_int(&cnt_ko_friend);
    return 0;
}

// @ 0x4e90d0    void TriggerVec::write(stream_base* out)   (thiscall, ret 4)
void TriggerVec::write(stream_base* out)
{
    out->w_int(&count);
    for (int i = 0; i < count; i++)
        data[i].write(out);
}

// @ 0x4e9110    void TriggerRec::write(stream_base* out)   (thiscall, ret 4)
void TriggerRec::write(stream_base* out)
{
    out->w_int(&state);
    out->w_i64(&check_timer.start);
    out->w_i64(&delay_timer.start);
    out->w_i64(&rearm_timer.start);
}

// @ 0x4e9150    void SimpleTriggerVec::write(stream_base* out)   (thiscall, ret 4)
void SimpleTriggerVec::write(stream_base* out)
{
    out->w_int(&count);
    for (int i = 0; i < count; i++)
        out->w_i64(&data[i].timer.start);
}

// @ 0x4e9190    int GameData::write_clock(stream_base* out)   (thiscall, ret 4)
int GameData::write_clock(stream_base* out)
{
    WL_MARKER(12)
    out->w_i64(&game_clock.start);
    out->w_int(&current_hours);
    out->w_int(&string_count);
    out->w_int(&f13b30);
    out->w_int(&f13b34);
    out->w_int(&f13b38);
    out->w_int(&f13b60);
    out->w_float(&cloud_amount);
    out->w_float(&haze_amount);
    out->w_float(&avg_value);
    out->w_float(&avg_weight);
    out->w_tstr(&t13a70);
    out->w_bool(&f13aa0);                     // the flag byte and its three pad bytes as one word
    out->w_int(&f13ae8);
    out->w_int(&f13a20);
    out->w_int(&encountered_party);
    out->w_int(&encountered_party2);
    out->w_int(&current_menu);
    out->w_int(&jump_scene);
    out->w_int(&jump_entry);
    out->w_int(&jump_mission);
    out->w_int(&range_lo);
    out->w_int(&range_hi);
    tstring tmp = message_log;
    tmp.spaces_to_underscores();
    if (!tmp.len) {
        out->w_cstr("-1");
    } else {
        out->w_tstr(&tmp);
    }
    out->w_int(&f1a1d8);
    out->w_int(&f1a1dc);
    out->w_int(&f1a1e0);
    out->w_int(&f1a1e4);
    out->w_int(&f1a1e8);
    out->w_int(&f1a1ec);
    WL_MARKER(13)
    out->w_i64(&timer13a80.start);
    out->w_float(&rest_period);
    out->w_int(&rest_time_speed);
    out->w_int(&rest_interactive);
    out->w_int(&rest_attackable);
    return 0;
}

// @ 0x4e93f0    void GameData::write_vec5c(stream_base* out)   (thiscall, ret 4)   vector at +0x13a64, stride 0x5c
void GameData::write_vec5c(stream_base* out)
{
    unsigned n;
    if (!fleet)
        n = 0;
    else
        n = (int)(fleet_end - fleet) / 0x5c;
    out->w_uint(&n);
    for (char* r = fleet; r < fleet_end; r += 0x5c)
        ((TrackRec*)r)->write(out);
}

// @ 0x4e9460    void TrackRec::write(stream_base* s)   (thiscall, ret 4)
void TrackRec::write(stream_base* s)
{
    s->w_float(&x).w_float(&y).w_float(&z);
    s->w_float(&f10);
    s->w_float(&age).w_int(&i18);
}

// @ 0x4e94b0    void SlotArray::write(stream_base* out)   (thiscall, ret 4)
void SlotArray::write(stream_base* out)
{
    unsigned n = size();
    out->w_uint(&n);
    for (unsigned i = 0; i < size(); i++)
        out->w_i64(&(*this)[i]);     // an 8-byte element (stream_base's +0x18 slot)
}

// @ 0x4e9510    void PartyTemplate::write(stream_base* out)   (thiscall, ret 4)
void PartyTemplate::write(stream_base* out)
{
    out->w_int(&num_created);
    out->w_int(&num_destroyed);
    out->w_int(&num_destroyed_by_player);
    slots.write(out);
}

// @ 0x4e9550    void GameData::write_list138(stream_base* out)   (thiscall, ret 4)
void GameData::write_list138(stream_base* out)
{
    out->w_int(&n13a5c);
    for (int i = 0; i < n13a5c; i++)
        quests[i].write(out);
}

// @ 0x4e95a0    void QuestRec::write(stream_base* out)   (thiscall, ret 4)
void QuestRec::write(stream_base* out)
{
    out->w_int(&f64);
    out->w_int(&giver_troop);
    out->w_int(&number);
    out->w_float(&f11c);
    out->w_tstr(&text);
    out->w_tstr(&title);
    out->w_tstr(&giver);
    write_soa(out, &notes);
    slots.write(out);
}

// @ 0x4e9620    stream_base* write_soa(stream_base* out, SoaBlock* b)   (free function, ret 0)
// The notes block (party/game.h SoaBlock, the reader's read_soa 0x51e1f0): per note its text, its time, then the tableau word.
stream_base* write_soa(stream_base* out, SoaBlock* b)
{
    for (int i = 0; i < 8; i++) {
        int* blk = &b->tableau;
        out->w_tstr(&b->texts[i]);
        out->w_int(&b->times[i]);
        out->w_int(blk);
    }
    return out;
}

// @ 0x4e9670    void GameData::write_list_party(stream_base* out)   (thiscall, ret 4)
void GameData::write_list_party(stream_base* out)
{
    out->w_int(&nparties);
    for (int i = 0; i < nparties; i++)
        scenes[i].slots.write(out);
}

// @ 0x4e96c0    void GameData::write_list300(stream_base* out)   (thiscall, ret 4)
void GameData::write_list300(stream_base* out)
{
    out->w_int(&n13a4c);
    for (int i = 0; i < n13a4c; i++)
        factions[i].write300(out, n13a4c);
}

// @ 0x4e9710    void FactionRec::write300(stream_base* out, int n)   (thiscall, ret 8)
void FactionRec::write300(stream_base* out, int n)
{
    slots.write(out);
    for (int i = 0; i < n; i++)
        out->w_float(&relations[i]);
    out->w_tstr(&name);
    out->w_bool(&f10);                         // the flag byte and its three pad bytes as one word
    out->w_uint(&f28);
    out->w_int(&f230);
    write_soa(out, &notes);
}

// @ 0x4e9790    void GameData::write_items(stream_base* out)   (thiscall, ret 4)   item kinds at 0x5e29d0
void GameData::write_items(stream_base* out)
{
    out->w_int(&g_item_kind_count);
    for (int i = 0; i < g_item_kind_count; i++)
        g_items[i].slots.write(out);
}

// @ 0x4e97e0    void GameData::write_records(stream_base* out)   (thiscall, ret 4)
void GameData::write_records(stream_base* out)
{
    out->w_int(&nrecords);
    for (int i = 0; i < nrecords; i++)
        records[i].write_save(out);
    for (int i = 0; i < 42; i++)
        out->w_int(&g_skill_tbl[i].cap);
}