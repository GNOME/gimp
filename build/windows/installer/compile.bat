@echo off
if [%1]==[] goto help
if [%2]==[] goto help
if [%3]==[] goto help
if [%4]==[] goto help
set VER=%~1
set GIMP_BASE=%~2
set GIMP32=%~3
set GIMP64=%~4
set DEPS_BASE=%~5
set DEPS32=%~6
set DEPS64=%~7

if [%INNOPATH%]==[] (
FOR /F "usebackq tokens=5,* skip=2" %%A IN (`REG QUERY "HKLM\Software\Microsoft\Windows\CurrentVersion\Uninstall\Inno Setup 5_is1" /v "Inno Setup: App Path" /reg:32`) DO set INNOPATH=%%B
)
if not exist "%INNOPATH%\iscc.exe" goto noinno

::i'd use %*, but shift has no effect on it
shift
shift
shift
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

"%INNOPATH%\iscc.exe" -DVERSION="%VER%" -DGIMP_DIR="%GIMP_BASE%" -DDIR32="%GIMP32%" -DDIR64="%GIMP64%" -DDEPS_DIR="%DEPS_BASE%" -DDDIR32="%DEPS32%" -DDDIR64="%DEPS64%" %PARAMS% gimp3264.iss
goto :eof

:help
echo Usage: %~n0%~x0 ver.si.on gimp_base_dir gimp_x86_dir gimp_x64_dir deps_base_dir deps_x86_dir deps_x64_dir [iscc_parameters]
echo Example: %~n0%~x0 2.9.4 X:\gimp-output\2.9-dev x86 x64 x:\gimp-deps x86 x64 -DPYTHON -DDEBUG_SYMBOLS
goto :eof
:noinno
echo Inno Setup path could not be read from Registry - install Inno Setup or set INNOPATH environment variable pointing at it's
echo install directory
goto :eof
