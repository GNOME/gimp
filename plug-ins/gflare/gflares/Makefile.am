## Process this file with automake to produce Makefile.in

gflaresdatadir = $(gimpdatadir)/gflares

gflaresdata_DATA = \
	Bright_Star		\
	Classic			\
	Default			\
	Distant_Sun		\
	GFlare_101		\
	GFlare_102		\
	Hidden_Planet




EXTRA_DATA =
EXTRA_DIST = $(gflaresdata_DATA)

files:
	@files=`ls $(DISTFILES) 2> /dev/null`; for p in $$files; do \
	  echo $$p; \
	done
