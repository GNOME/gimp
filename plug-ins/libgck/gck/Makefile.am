## Process this file with automake to produce Makefile.in

gckincludedir = $(includedir)/gimp-$(LT_RELEASE)/gck

lib_LTLIBRARIES = libgck-1.3.la

libgck_1_3_la_SOURCES = \
	gckcolor.c

gckinclude_HEADERS = \
	gck.h

libgck_1_3_la_LDFLAGS = \
	-version-info $(LT_CURRENT):$(LT_REVISION):$(LT_AGE)

INCLUDES = \
	-I..			\
	-I$(srcdir)/..		\
	-I$(top_srcdir)		\
	$(GTK_CFLAGS)		\
	-I$(includedir)

libgck_1_3_la_LIBADD = \
	$(GTK_LIBS)

.PHONY: files

files:
	@files=`ls $(DISTFILES) 2> /dev/null `; for p in $$files; do \
	  echo $$p; \
	done
