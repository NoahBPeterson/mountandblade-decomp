// id_stack.h: the screen id stack at 0xa739f8 (a std::deque<int>: a ring of 4-int blocks) and its top, as
// GameApp::Update (0x50ec40) and VideoPage::update (0x4d63b0) read it inline.
#ifndef GAME_ID_STACK_H
#define GAME_ID_STACK_H

struct IdStack {                    // 0xa739f8
    int f0;                         // 0xa739f8
    int** blocks;                   // 0xa739fc
    unsigned block_count;           // 0xa73a00
    unsigned a;                     // 0xa73a04 (offset)
    unsigned b;                     // 0xa73a08 (size)
};
extern IdStack g_id_stack;              // 0xa739f8

static __forceinline int current_screen_id()
{
    unsigned i = g_id_stack.b + g_id_stack.a - 1;
    unsigned q = i >> 2;
    unsigned r = i - q * 4;
    if (q >= g_id_stack.block_count) q -= g_id_stack.block_count;
    return g_id_stack.blocks[q][r];
}
#endif
