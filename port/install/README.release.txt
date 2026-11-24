Mount & Blade 1.011 -- native port (@TARGET@, @VERSION@)

This is a build of https://github.com/NoahBPeterson/mountandblade-decomp . It contains NO game files: you need your
own copy of Mount & Blade 1.011 (Steam). The port reads the game's Data/, CommonRes/, Modules/, mb.fx ... folder.

Run
  @RUN@

Game folder: set MB_GAME_DIR to the folder that contains Data/ and Modules/, or leave it unset and the launcher looks
in the usual Steam library locations.
@NOTES@
Saves and settings: ~/Documents/Mount&Blade (Windows: %USERPROFILE%\Documents\Mount&Blade). MB_USER_DIR=<dir> moves them.

Licence: GPL version 2 (LICENSE). Third-party components statically linked into the binary and their licences:
THIRD_PARTY_LICENSES.txt. To build from source: docs/BUILDING_PORT.md in the repository.
