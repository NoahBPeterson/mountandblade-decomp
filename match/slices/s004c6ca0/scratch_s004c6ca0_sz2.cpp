#include <windows.h>

struct T { char s[128]; };

void f(HWND w, int mode, int g, int *out)
{
    char buf[128];
    LRESULT cur = SendMessage(GetDlgItem(w, 0x45f), CB_GETCURSEL, 0, 0);
    if (cur == CB_ERR) {
        *out = (mode == 0x17 || mode == 0x19 || mode == 0x1a || mode == 0x18 || mode == 0x1e) ? 0x10 : 0x20;
    } else {
        SendMessage(GetDlgItem(w, 0x45f), CB_GETLBTEXT, cur, (LPARAM)buf);
        *out = 1;
    }
}
