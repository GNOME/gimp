## Process this file with automake to produce Makefile.in

libexecdir = $(gimpplugindir)/plug-ins

libexec_PROGRAMS = gimp_ace

gimp_ace_SOURCES = \
	gimp_ace.c		\
	gimp_ace.h		\
	glace.c			\
	glace.h			\
	glaceG.c		\
	glaceInt.h		\
	dialog.c		\
	color.h			\
	acconfig.h

AM_CPPFLAGS = \
	-DGLACE_GIMP		\
	-DVERSION=\"0.5.0\"

INCLUDES = \
	-I$(top_srcdir)			\
	$(GTK_CFLAGS)			\
	-I$(includedir)

LDADD = \
	$(top_builddir)/libgimp/libgimp.la	\
	$(GTK_LIBS)				\
	$(INTLLIBS)

.PHONY: files

files:
	@files=`ls $(DISTFILES) 2> /dev/null`; for p in $$files; do \
	  echo $$p; \
	done
