## Process this file with automake to produce Makefile.in

gflaredir = $(gimpdatadir)/gflare

gflare_DATA = \
	Bright_Star		\
	Classic			\
	Distant_Sun		\
	Default			\
	GFlare_101		\
	GFlare_102		\
	Hidden_Planet

EXTRA_DIST = $(gflare_DATA)

files:
	@files=`ls $(DISTFILES) 2> /dev/null`; for p in $$files; do \
	  echo $$p; \
	done
