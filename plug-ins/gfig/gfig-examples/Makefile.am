## Process this file with automake to produce Makefile.in

gfigdatadir = $(gimpdatadir)/gfig

gfigdata_DATA = \
	A_star			\
	curves			\
	polys			\
	ring			\
	ring+star		\
	simily			\
	spirals_and_stars	\
	sprial			\
	star2			\
	stars

EXTRA_DIST = $(gfigdata_DATA)

files:
	@files=`ls $(DISTFILES) 2> /dev/null`; for p in $$files; do \
	  echo $$p; \
	done
