## Process this file with automake to produce Makefile.in

helpdatadir = $(gimpdatadir)/help/C/images/glossary

helpdata_DATA = \
		modes-addition.png \
		modes-color.png \
		modes-darken.png \
		modes-difference.png \
		modes-dissolve.png \
		modes-divide.png \
		modes-hue.png \
		modes-lighten.png \
		modes-multiply.png \
		modes-normal2.png \
		modes-normal.png \
		modes-overlay.png \
		modes-saturation.png \
		modes-screen.png \
		modes-subtract.png \
		modes-value.png

EXTRA_DIST = $(helpdata_DATA)

.PHONY: files

files:
	@files=`ls $(DISTFILES) 2> /dev/null`; for p in $$files; do \
	  echo $$p; \
	done

