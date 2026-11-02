// module_name.h: the selected module's folder name (g_module_name, 0x8ee3c38): set by the module chooser (options_panel/world_app.cpp),
// read when the module's files are opened and its save directory is named. A dynamic-initialised global (tstring constructor +
// atexit destructor), defined in triggers/trigger_vec.cpp.
#ifndef GAME_MODULE_NAME_H
#define GAME_MODULE_NAME_H
#include "./tstring.h"
extern tstring g_module_name;                   // 0x8ee3c38 the selected module
#endif // GAME_MODULE_NAME_H
