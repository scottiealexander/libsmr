@ECHO off
SETLOCAL ENABLEEXTENSIONS

SetEnv /Release

cl /LD smr.c /link /DEF:smr.def

IF /I "%1" EQU "32" (
    SET target_dir=cl_32
) ELSE (
    SET target_dir=cl_64
)

SET julia_base_dir=C:\Users\scottie\Documents\code\julia\libsmr\windows
SET julia_dir=%julia_base_dir%\%target_dir%

:: if this is 64bit, copy the dll to the location expected by SMR.jl
IF /I "%target_dir%"=="cl_64" (
    copy smr.dll %julia_base_dir%\libsmr.dll
)

copy smr.dll %julia_dir%\libsmr.dll

SET out_dir=C:\Users\scottie\Documents\code\c\libsmr\lib\windows\%target_dir%

move /Y smr.dll %out_dir%\libsmr.dll
move /Y smr.lib %out_dir\libsmr.lib
del /F smr.exp smr.obj

ECHO %out_dir%
