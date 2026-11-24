@echo off
rem mountandblade.bat: starts mb.exe with the working directory set to your Mount & Blade 1.011 folder (the game reads
rem Data\, CommonRes\, Modules\ ... relative to it). The port ships no game files; see docs\BUILDING_PORT.md.
rem   set MB_GAME_DIR=C:\path\to\game   then run this file.  Without it the default Steam library is searched.
setlocal
set "game=%MB_GAME_DIR%"
if not defined game (
  for %%N in ("Mount and Blade" "Mount & Blade" "MountBlade") do (
    if exist "%ProgramFiles(x86)%\Steam\steamapps\common\%%~N\Data" set "game=%ProgramFiles(x86)%\Steam\steamapps\common\%%~N"
    if exist "%ProgramFiles%\Steam\steamapps\common\%%~N\Data" set "game=%ProgramFiles%\Steam\steamapps\common\%%~N"
  )
)
if not defined game goto :nogame
if not exist "%game%\Data" goto :nogame
set "exe=%~dp0mb.exe"
cd /d "%game%" || exit /b 1
"%exe%" %*
exit /b %errorlevel%
:nogame
echo mountandblade: cannot find your Mount ^& Blade 1.011 folder (the one containing Data and Modules).
echo Set MB_GAME_DIR=C:\path\to\it and run again.
exit /b 1
