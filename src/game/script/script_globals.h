// script_globals.h: the script runner's status globals (defined in script_interpreter.cpp), shared by the interpreter,
// the validators (report 0x49cc60) and the block runner. g_script_error (0x5e259c) is a file static of script_interpreter.cpp. No other includes: game_globals.h and
// script_interpreter.h both declare a g_dialogs of different types, so neither can include the other.
#ifndef GAME_SCRIPT_SCRIPT_GLOBALS_H
#define GAME_SCRIPT_SCRIPT_GLOBALS_H
extern int g_cur_param;         // 0x5e2590
extern int g_lineno;            // 0x5e2598  line of the statement being run (report)
extern int g_cur_opcode;        // 0x5e25a0  opcode word of the statement being run (validators print it)
extern int g_or_latch;          // 0x5e25a4  this_or_next chain already satisfied: skip until the chain ends
#endif
