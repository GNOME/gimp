## Process this file with automake to produce Makefile.in

paperdatadir = $(gimpdatadir)/gimpressionist/Paper

paperdata_DATA = \
	bricks.pgm		\
	bricks2.pgm		\
	burlap.pgm		\
	canvas2.pgm		\
	defaultpaper.pgm	\
	marble.pgm		\
	marble2.pgm		\
	stone.pgm

EXTRA_DIST = $(paperdata_DATA)

files:
	@files=`ls $(DISTFILES) 2> /dev/null`; for p in $$files; do \
	  echo $$p; \
	done
