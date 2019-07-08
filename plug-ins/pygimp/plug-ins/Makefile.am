## Process this file with automake to produce Makefile.in

pluginexecdir = $(gimpplugindir)/plug-ins

source_scripts = \
	colorxhtml.py			\
	file-openraster.py		\
	foggify.py			\
	gradients-save-as-css.py	\
	histogram-export.py     	\
	palette-offset.py		\
	palette-sort.py			\
	palette-to-gradient.py		\
	py-slice.py			\
	python-eval.py			\
	spyro_plus.py			\
					\
	benchmark-foreground-extract.py	\
	clothify.py			\
	shadow_bevel.py			\
	sphere.py			\
	whirlpinch.py

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
	python-eval/python-eval.py			\
	spyro_plus/spyro_plus.py

test_scripts = \
	benchmark-foreground-extract/benchmark-foreground-extract.py	\
	clothify/clothify.py						\
	shadow_bevel/shadow_bevel.py					\
	sphere/sphere.py						\
	whirlpinch/whirlpinch.py

$(scripts) $(test_scripts): $(source_scripts)
	$(AM_V_GEN) mkdir -p $(@D) && cp -f "$(srcdir)/$(@F)" $@

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

EXTRA_DIST = \
	$(source_scripts)	\
	$(console_scripts)

CLEANFILES = $(scripts) $(test_scripts)
