## Process this file with automake to produce Makefile.in

noinst_LTLIBRARIES = libapppaint-funcs.la

if HAVE_ASM_MMX
mmx_sources = \
	paint-funcs-simd.S
else
mmx_sources =
endif

libapppaint_funcs_la_SOURCES = \
	paint-funcs.c	\
	paint-funcs.h	\
	$(mmx_sources)

AM_CPPFLAGS = \
	-DG_LOG_DOMAIN=\"Gimp-Paint-Funcs\"

INCLUDES = \
	-I$(top_srcdir)			\
	-I$(top_srcdir)/app		\
	$(GTK_CFLAGS)			\
	-I$(includedir)

.PHONY: files

files:
	@files=`ls $(DISTFILES) 2> /dev/null`; for p in $$files; do \
	  echo $$p; \
	done
