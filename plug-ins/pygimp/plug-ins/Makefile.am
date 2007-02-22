## Process this file with automake to produce Makefile.in

pluginexecdir = $(gimpplugindir)/plug-ins

scripts = \
	colorxhtml.py		\
	foggify.py		\
	gimpcons.py		\
	happy-valley-relief.py	\
	palette-offset.py	\
	palette-sort.py		\
	palette-to-gradient.py	\
	py-slice.py		\
	shadow_bevel.py

test_scripts = \
	benchmark-foreground-extract.py	\
	clothify.py		\
	sphere.py		\
	whirlpinch.py

pluginexec_SCRIPTS = $(scripts)

pluginexec_DATA = pyconsole.py

EXTRA_DIST = $(pluginexec_DATA) $(scripts) $(test_scripts)
