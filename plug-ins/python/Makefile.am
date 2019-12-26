## Process this file with automake to produce Makefile.in

SUBDIRS = \
	python-console

colorxhtmldir = $(gimpplugindir)/plug-ins/colorxhtml
colorxhtml_SCRIPTS = colorxhtml.py

file_openrasterdir = $(gimpplugindir)/plug-ins/file-openraster
file_openraster_SCRIPTS = file-openraster.py

foggifydir = $(gimpplugindir)/plug-ins/foggify
foggify_SCRIPTS = foggify.py

gradients_save_as_cssdir = $(gimpplugindir)/plug-ins/gradients-save-as-css
gradients_save_as_css_SCRIPTS = gradients-save-as-css.py

histogram_exportdir = $(gimpplugindir)/plug-ins/histogram-export
histogram_export_SCRIPTS = histogram-export.py

palette_offsetdir = $(gimpplugindir)/plug-ins/palette-offset
palette_offset_SCRIPTS = palette-offset.py

palette_sortdir = $(gimpplugindir)/plug-ins/palette-sort
palette_sort_SCRIPTS = palette-sort.py

palette_to_gradientdir = $(gimpplugindir)/plug-ins/palette-to-gradient
palette_to_gradient_SCRIPTS = palette-to-gradient.py

py_slicedir = $(gimpplugindir)/plug-ins/py-slice
py_slice_SCRIPTS = py-slice.py

python_evaldir = $(gimpplugindir)/plug-ins/python-eval
python_eval_SCRIPTS = python-eval.py

spyro_plusdir = $(gimpplugindir)/plug-ins/spyro-plus
spyro_plus_SCRIPTS = spyro-plus.py

if GIMP_UNSTABLE
benchmark_foreground_extractdir = $(gimpplugindir)/plug-ins/benchmark-foreground-extract
benchmark_foreground_extract_SCRIPTS = benchmark-foreground-extract.py
##	clothify.py
##	shadow_bevel.py
##	sphere.py
##	whirlpinch.py
endif

EXTRA_DIST = \
	colorxhtml.py			\
	file-openraster.py		\
	foggify.py			\
	gradients-save-as-css.py	\
	histogram-export.py		\
	palette-offset.py		\
	palette-sort.py		\
	palette-to-gradient.py		\
	py-slice.py			\
	python-eval.py			\
	spyro-plus.py

if GIMP_UNSTABLE
EXTRA_DIST += benchmark-foreground-extract.py
endif

# Python interpreter file.

pyinterpdir = $(gimpplugindir)/interpreters
pyinterpfile = $(pyinterpdir)/pygimp.interp

if HAS_PYTHON_INTERP
install-interp-file:
	$(mkinstalldirs) '$(DESTDIR)$(pyinterpdir)'
	echo 'python=$(PYBIN_PATH)' > '$(DESTDIR)$(pyinterpfile)'
	echo 'python3=$(PYBIN_PATH)' >> '$(DESTDIR)$(pyinterpfile)'
	echo '/usr/bin/python=$(PYBIN_PATH)' >> '$(DESTDIR)$(pyinterpfile)'
	echo ":Python:E::py::`basename $(PYTHON)`:" >> '$(DESTDIR)$(pyinterpfile)'

install-data-local: install-interp-file
endif

uninstall-local:
	rm -f '$(DESTDIR)$(pyinterpfile)'
