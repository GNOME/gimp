## Process this file with automake to produce Makefile.in

gckincludedir = $(includedir)/gck

lib_LTLIBRARIES = libgck.la

libgck_la_SOURCES =	gckcolor.c	\
			gcklistbox.c	\
			gckmath.c	\
			gcknotebook.c	\
			gckui.c		\
			gckvector.c

gckinclude_HEADERS =	gck.h		\
			gckcolor.h	\
			gckcommon.h	\
			gckimage.h	\
			gcklistbox.h	\
			gckmath.h	\
			gcktypes.h	\
			gckui.h		\
			gcknotebook.h	\
			gckvector.h

libgck_la_LDFLAGS = -version-info 0:1:0 -rpath $(libdir) 

INCLUDES = \
	-I$(srcdir)/..			\
	-I$(top_srcdir)		\
	-I$(includedir)

.PHONY: files

files:
	@files=`ls $(DISTFILES) 2> /dev/null `; for p in $$files; do \
	  echo $$p; \
	done
