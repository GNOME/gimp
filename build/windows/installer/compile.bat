@echo off
if [%1]==[] goto help
if [%2]==[] goto help
if [%3]==[] goto help
if [%4]==[] goto help
if [%5]==[] goto help
if [%6]==[] goto help
if [%7]==[] goto help
set VER=%~1
set LIGMA_BASE=%~2
set LIGMA32=%~3
set LIGMA64=%~4
set DEPS_BASE=%~5
set DEPS32=%~6
set DEPS64=%~7

if [%INNOPATH%]==[] (
FOR /F "usebackq tokens=5,* skip=2" %%A IN (`REG QUERY "HKEY_CURRENT_USER\Software\Microsoft\Windows\CurrentVersion\Uninstall\Inno Setup 6_is1" /v "Inno Setup: App Path" /reg:32`) DO set INNOPATH=%%B
) else (if [%INNOPATH%]==[] (
FOR /F "usebackq tokens=5,* skip=2" %%A IN (`REG QUERY "HKLM\Software\Microsoft\Windows\CurrentVersion\Uninstall\Inno Setup 6_is1" /v "Inno Setup: App Path" /reg:32`) DO set INNOPATH=%%B
))
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

"%INNOPATH%\iscc.exe" -DVERSION="%VER%" -DLIGMA_DIR="%LIGMA_BASE%" -DDIR32="%LIGMA32%" -DDIR64="%LIGMA64%" -DDEPS_DIR="%DEPS_BASE%" -DDDIR32="%DEPS32%" -DDDIR64="%DEPS64%" -DDEBUG_SYMBOLS -DPYTHON -DLUA %PARAMS% ligma3264.iss
goto :eof

:help
echo Usage: %~n0%~x0 ver.si.on ligma_base_dir ligma_x86_dir ligma_x64_dir deps_base_dir deps_x86_dir deps_x64_dir [iscc_parameters]
echo Example: %~n0%~x0 2.9.4 X:\ligma-output\2.9-dev x86 x64 x:\ligma-deps x86 x64 -DPYTHON -DDEBUG_SYMBOLS
goto :eof
:noinno
echo Inno Setup path could not be read from Registry - install Inno Setup or set INNOPATH environment variable pointing at it's
echo install directory
goto :eof
