## Process this file with automake to produce Makefile.in

noinst_LIBRARIES = libappxcf.a

libappxcf_a_SOURCES = @STRIP_BEGIN@ \
	xcf.c		\
	xcf.h		\
	xcf-load.c	\
	xcf-load.h	\
	xcf-read.c	\
	xcf-read.h	\
	xcf-save.c	\
	xcf-save.h	\
	xcf-seek.c	\
	xcf-seek.h	\
	xcf-write.c	\
	xcf-write.h	\
@STRIP_END@

AM_CPPFLAGS = @STRIP_BEGIN@ \
	-DG_LOG_DOMAIN=\"Gimp-XCF\"	\
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
