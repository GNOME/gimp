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

if GIMP_UNSTABLE
pluginexec_SCRIPTS = $(scripts) $(test_scripts)
else
pluginexec_SCRIPTS = $(scripts)
endif

pluginexec_DATA = pyconsole.py

EXTRA_DIST = $(pluginexec_DATA) $(pluginexec_SCRIPTS) $(test_scripts)
