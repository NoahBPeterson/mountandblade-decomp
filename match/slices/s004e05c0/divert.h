// Temporary redirection helper used while iterating in this slice only.
// Localized text of a key: the callee stores the text into the 16 bytes at (key - 8), i.e. the slot just
// above the caller's key object, which is how the original keeps several texts alive at once.
struct text_ret { tstring* t; operator tstring*() { return t; } };
static inline text_ret tex(const char* key) { tstring k(key); text_ret r = { g_text.get(k) }; return r; }
