@echo off
if [%1]==[] goto help
if [%2]==[] goto help
if [%3]==[] goto help
if [%4]==[] goto help
set VER=%~1
set GIMPDIR=%~2
set DIR32=%~3
set DIR64=%~4

FOR /F "usebackq tokens=5,* skip=2" %%A IN (`REG QUERY "HKLM\Software\Microsoft\Windows\CurrentVersion\Uninstall\Inno Setup 5_is1" /v "Inno Setup: App Path" /reg:32`) DO set INNOPATH=%%B
if not exist "%INNOPATH%\iscc.exe" goto noinno

::i'd use %*, but shift has no effect on it
shift
shift
shift
shift
set PARAMS=
:doparams
if "%1"=="" goto paramsdone
set PARAMS=%PARAMS% %1
shift
goto doparams
:paramsdone

"%INNOPATH%\iscc.exe" -DVERSION="%VER%" -DGIMP_DIR="%GIMPDIR%" -DDIR32="%DIR32%" -DDIR64="%DIR64%" %PARAMS% gimp3264.iss
goto :eof

:help
echo Usage: %~n0%~x0 ver.si.on base_dir gimp_x86_dir gimp_x64_dir
echo Example: %~n0%~x0 2.9.4 X:\gimp-output\2.9-dev gimp-dev-i686-2016-10-08 gimp-dev-x86_64-2016-10-08 
goto :eof
:noinno
echo Inno Setup path could not be read from Registry
goto :eof
