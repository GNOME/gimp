@REM This file is executed by the GIMP. Do NOT run this yourself.
@echo off
mkdir %2
copy %1\gimprc_user %2\gimprc
copy %1\unitrc %2\unitrc
copy %1\gtkrc_user %2\gtkrc
mkdir %2\brushes
mkdir %2\generated_brushes
mkdir %2\gradients
mkdir %2\palettes
mkdir %2\patterns
mkdir %2\plug-ins
mkdir %2\modules
mkdir %2\scripts
mkdir %2\gfig
mkdir %2\gflare
mkdir %2\fractalexplorer
mkdir %2\gimpressionist
mkdir %2\gimpressionist\Brushes
mkdir %2\gimpressionist\Paper
mkdir %2\gimpressionist\Presets
mkdir %2\levels
mkdir %2\curves

pause
