## Process this file with automake to produce Makefile.in

pluginexecdir = $(gimpplugindir)/plug-ins

scripts = \
	colorxhtml/colorxhtml.py			\
	file-openraster/file-openraster.py		\
	foggify/foggify.py				\
	gradients-save-as-css/gradients-save-as-css.py	\
	histogram-export/histogram-export.py     	\
	palette-offset/palette-offset.py		\
	palette-sort/palette-sort.py			\
	palette-to-gradient/palette-to-gradient.py	\
	py-slice/py-slice.py				\
	python-eval/python-eval.py

test_scripts = \
	benchmark-foreground-extract/benchmark-foreground-extract.py	\
	clothify/clothify.py						\
	shadow_bevel/shadow_bevel.py					\
	sphere/sphere.py						\
	whirlpinch/whirlpinch.py

%.py:
	mkdir -p $(@D) && cp -f $(srcdir)/$(@F) $@

nobase_pluginexec_SCRIPTS = $(scripts)

if GIMP_UNSTABLE
nobase_pluginexec_SCRIPTS += $(test_scripts)
endif

# python-console has a data file.
# Therefore let's move it to its own sub-directory.
consoleexecdir = $(gimpplugindir)/plug-ins/python-console
console_scripts = python-console.py
consoleexec_SCRIPTS = $(console_scripts)
dist_consoleexec_DATA = pyconsole.py

EXTRA_DIST = $(scripts) $(test_scripts) $(console_scripts)
