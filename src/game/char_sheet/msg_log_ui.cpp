#include "./msg_log_ui.h"
#include "../tstring.h"      // (listed directly: the resolution audit reads only the source's own includes)
#include "../../include/msg_log.h"      // (listed directly: the resolution audit reads only the source's own includes)

// @ 0x4f13f0
__declspec(noinline) void MsgLog::Ring::pop(tstring& out, int& index)
{
    if (count != 0) {
        out = names[head];
        index = vals[head];
        head = (head + 1) % 32;
        count--;
        return;
    }
    out.assign("No messages");
    index = -1;
}

// @ 0x4f1390
void MsgLog::flush()
{
    tstring msg;
    int index;
    Ring* r = &ring;
    while (r->count != 0) {
        r->pop(msg, index);
        add(msg, index);
    }
}
