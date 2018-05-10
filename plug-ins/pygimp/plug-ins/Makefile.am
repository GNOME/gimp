## Process this file with automake to produce Makefile.in

pluginexecdir = $(gimpplugindir)/plug-ins

scripts = \
	colorxhtml.py			\
	file-openraster.py		\
	foggify.py			\
	gradients-save-as-css.py	\
	histogram-export.py     	\
	palette-offset.py		\
	palette-sort.py			\
	palette-to-gradient.py		\
	py-slice.py			\
	python-eval.py

test_scripts = \
	benchmark-foreground-extract.py	\
	clothify.py			\
	shadow_bevel.py			\
	sphere.py			\
	whirlpinch.py

pluginexec_SCRIPTS = $(scripts)

if GIMP_UNSTABLE
pluginexec_SCRIPTS += $(test_scripts)
endif

# python-console has a data file.
# Therefore let's move it to its own sub-directory.
consoleexecdir = $(gimpplugindir)/plug-ins/python-console
console_scripts = python-console.py
consoleexec_SCRIPTS = $(console_scripts)
dist_consoleexec_DATA = pyconsole.py

EXTRA_DIST = $(scripts) $(test_scripts) $(console_scripts)
