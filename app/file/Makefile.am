## Process this file with automake to produce Makefile.in

noinst_LIBRARIES = libappfile.a

libappfile_a_SOURCES = @STRIP_BEGIN@ \
	file-open.c	\
	file-open.h	\
	file-save.c	\
	file-save.h	\
	file-utils.c	\
	file-utils.h	\
@STRIP_END@

AM_CPPFLAGS = @STRIP_BEGIN@ \
	-DG_LOG_DOMAIN=\"Gimp-File\"	\
	@GIMP_THREAD_FLAGS@ 		\
	@GIMP_MP_FLAGS@			\
@STRIP_END@

INCLUDES = @STRIP_BEGIN@ \
	-I$(top_srcdir)			\
	-I$(top_srcdir)/app		\
	$(GTK_CFLAGS)			\
	-I$(includedir)			\
@STRIP_END@

.PHONY: files

files:
	@files=`ls $(DISTFILES) 2> /dev/null`; for p in $$files; do \
	  echo $$p; \
	done
