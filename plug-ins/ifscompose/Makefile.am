## Process this file with automake to produce Makefile.in

libexecdir = $(gimpplugindir)/plug-ins

EXTRA_DIST = README.ifscompose

libexec_PROGRAMS = ifscompose

ifscompose_SOURCES = \
	ifscompose.c		\
	ifscompose.h		\
	ifscompose_utils.c      \
	ifscompose_storage.c

INCLUDES = \
	-I$(top_srcdir)			\
	$(GTK_CFLAGS)			\
	-I$(includedir)

LDADD = \
	$(top_builddir)/libgimp/libgimpui.la	\
	$(top_builddir)/libgimp/libgimp.la	\
	$(GTK_LIBS)				\
	$(INTLLIBS)

.PHONY: files

files:
	@files=`ls $(DISTFILES) 2> /dev/null`; for p in $$files; do \
	  echo $$p; \
	done
