## Process this file with automake to produce Makefile.in

pluginlibdir = $(gimpplugindir)/plug-ins

pluginlib_PROGRAMS = faxg3

faxg3_SOURCES = \
	faxg3.c		\
	g3.c		\
	g3.h		\
	run_tbl.c   

INCLUDES = \
	-I$(top_srcdir)			\
	$(GLIB_CFLAGS)			\
	-I$(includedir)

LDADD = \
	$(top_builddir)/libgimp/libgimp.la	\
	$(GLIB_LIBS)				\
	$(INTLLIBS)

.PHONY: files

files:
	@files=`ls $(DISTFILES) 2> /dev/null`; for p in $$files; do \
	  echo $$p; \
	done
