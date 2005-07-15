pluginexecdir = $(gimpplugindir)/plug-ins

scripts = \
	colorxhtml.py	\
	clothify.py	\
	foggify.py	\
	gimpcons.py	\
	pdbbrowse.py	\
	py-slice.py	\
	shadow_bevel.py	\
	whirlpinch.py

test_scripts = \
	benchmark-foreground-extract.py	\
	sphere.py

if GIMP_UNSTABLE
pluginexec_SCRIPTS = $(scripts) $(test_scripts)
else
pluginexec_SCRIPTS = $(scripts)
endif

pluginexec_DATA = gtkcons.py

EXTRA_DIST = $(pluginexec_DATA) $(pluginexec_SCRIPTS) $(test_scripts)
