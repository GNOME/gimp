## Process this file with automake to produce Makefile.in

gckincludedir = $(includedir)/gck

lib_LTLIBRARIES = libgck.la

libgck_la_SOURCES = \
	gckcolor.c	\
	gcklistbox.c	\
	gckmath.c	\
	gckui.c		\
	gckvector.c

gckinclude_HEADERS = \
	gck.h		\
	gckcolor.h	\
	gckcommon.h	\
	gckimage.h	\
	gcklistbox.h	\
	gckmath.h	\
	gcktypes.h	\
	gckui.h		\
	gckvector.h

libgck_la_LDFLAGS = \
	-version-info $(LT_CURRENT):$(LT_REVISION):$(LT_AGE) -release $(LT_RELEASE)

INCLUDES = \
	-I..			\
	-I$(srcdir)/..		\
	-I$(top_srcdir)		\
	$(GTK_CFLAGS)		\
	-I$(includedir)

libgck_la_LIBADD = \
	$(GTK_LIBS)

.PHONY: files

files:
	@files=`ls $(DISTFILES) 2> /dev/null `; for p in $$files; do \
	  echo $$p; \
	done
