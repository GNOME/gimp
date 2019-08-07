## Process this file with automake to produce Makefile.in

SUBDIRS = \
	python-console

palette_offsetdir = $(gimpplugindir)/plug-ins/palette-offset
palette_offset_SCRIPTS = palette-offset.py

palette_to_gradientdir = $(gimpplugindir)/plug-ins/palette-to-gradient
palette_to_gradient_SCRIPTS = palette-to-gradient.py

# TODO: to be ported:
##	colorxhtml.py
##	file-openraster.py
##	foggify.py
##	gradients-save-as-css.py
##	histogram-export.py
##	palette-sort.py
##	py-slice.py
##	python-eval.py
##	spyro_plus.py

if GIMP_UNSTABLE
##	benchmark-foreground-extract.py
##	clothify.py
##	shadow_bevel.py
##	sphere.py
##	whirlpinch.py
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
