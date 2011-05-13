## Process this file with automake to produce Makefile.in

pluginexecdir = $(gimpplugindir)/plug-ins

scripts = \
	colorxhtml.py		\
	file-openraster.py	\
	foggify.py		\
        gradients-save-as-css.py\
	palette-offset.py	\
	palette-sort.py		\
	palette-to-gradient.py	\
	py-slice.py		\
	python-console.py	\
	python-eval.py		\
	text-brush.py

test_scripts = \
	benchmark-foreground-extract.py	\
	clothify.py		\
	shadow_bevel.py		\
	sphere.py		\
	whirlpinch.py

pluginexec_SCRIPTS = $(scripts)

pluginexec_DATA = pyconsole.py

EXTRA_DIST = $(pluginexec_DATA) $(scripts) $(test_scripts)
