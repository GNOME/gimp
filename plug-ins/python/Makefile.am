## Process this file with automake to produce Makefile.in

SUBDIRS = \
	python-console

colorxhtmldir = $(ligmaplugindir)/plug-ins/colorxhtml
colorxhtml_SCRIPTS = colorxhtml.py

file_openrasterdir = $(ligmaplugindir)/plug-ins/file-openraster
file_openraster_SCRIPTS = file-openraster.py

foggifydir = $(ligmaplugindir)/plug-ins/foggify
foggify_SCRIPTS = foggify.py

gradients_save_as_cssdir = $(ligmaplugindir)/plug-ins/gradients-save-as-css
gradients_save_as_css_SCRIPTS = gradients-save-as-css.py

histogram_exportdir = $(ligmaplugindir)/plug-ins/histogram-export
histogram_export_SCRIPTS = histogram-export.py

palette_offsetdir = $(ligmaplugindir)/plug-ins/palette-offset
palette_offset_SCRIPTS = palette-offset.py

palette_sortdir = $(ligmaplugindir)/plug-ins/palette-sort
palette_sort_SCRIPTS = palette-sort.py

palette_to_gradientdir = $(ligmaplugindir)/plug-ins/palette-to-gradient
palette_to_gradient_SCRIPTS = palette-to-gradient.py

python_evaldir = $(ligmaplugindir)/plug-ins/python-eval
python_eval_SCRIPTS = python-eval.py

spyro_plusdir = $(ligmaplugindir)/plug-ins/spyro-plus
spyro_plus_SCRIPTS = spyro-plus.py

EXTRA_DIST = \
	colorxhtml.py			\
	file-openraster.py		\
	foggify.py			\
	gradients-save-as-css.py	\
	histogram-export.py		\
	palette-offset.py		\
	palette-sort.py		\
	palette-to-gradient.py		\
	python-eval.py			\
	spyro-plus.py

# Python interpreter file.

pyinterpdir = $(ligmaplugindir)/interpreters
pyinterpfile = $(pyinterpdir)/pyligma.interp

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
