@ECHO off
SETLOCAL ENABLEEXTENSIONS

SetEnv /Release

SET objfile="./smr.obj"
SET expfile="./libsmr.exp"
SET libfile="./libsmr.lib"

SET target=./lib/windows/libsmr.dll
SET julia_lib=./julia/lib/libsmr.dll

cl /Ox /Fe:%target% /LD smr.c /link /DEF:smr.def

copy %target% %julia_lib%

cl /Ox /Fe:./smr2mda.exe ./smr2mda.c %libfile%

del /F %objfile% %expfile% %libfile%
