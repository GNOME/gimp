## Process this file with automake to produce Makefile.in

if HAVE_GLIBC_REGEX
regex_sources =
else
regex_sources = regexrepl.c regexrepl.h
endif

SUBDIRS = scripts

libexecdir = $(gimpplugindir)/plug-ins

libexec_PROGRAMS = script-fu

script_fu_SOURCES = @STRIP_BEGIN@ \
	interp_regex.c		\
	interp_slib.c		\
	interp_sliba.c		\
	interp_trace.c		\
	script-fu.c		\
	script-fu-console.c	\
	script-fu-console.h	\
	script-fu-constants.c	\
	script-fu-constants.h	\
	script-fu-intl.h	\
	script-fu-scripts.c	\
	script-fu-scripts.h	\
	script-fu-server.c	\
	script-fu-server.h	\
	script-fu-enums.h	\
	siod.h			\
	siodp.h			\
	$(regex_sources)	\
@STRIP_END@

EXTRA_DIST = @STRIP_BEGIN@ \
	regexrepl.c		\
	regexrepl.h		\
@STRIP_END@

AM_CPPFLAGS = @STRIP_BEGIN@ \
	-DREGEX_MALLOC			\
	-DLOCALEDIR=\""$(localedir)"\"	\
@STRIP_END@

INCLUDES = @STRIP_BEGIN@ \
	-I$(top_srcdir)			\
	$(GTK_CFLAGS)			\
	-I$(includedir)			\
@STRIP_END@

LDADD = @STRIP_BEGIN@ \
	$(top_builddir)/plug-ins/dbbrowser/dbbrowser_utils.o		\
	$(top_builddir)/libgimp/libgimpui-$(LT_RELEASE).la		\
	$(top_builddir)/libgimpwidgets/libgimpwidgets-$(LT_RELEASE).la	\
	$(top_builddir)/libgimp/libgimp-$(LT_RELEASE).la		\
	$(top_builddir)/libgimpcolor/libgimpcolor-$(LT_RELEASE).la	\
	$(top_builddir)/libgimpbase/libgimpbase-$(LT_RELEASE).la	\
	$(GTK_LIBS)							\
	$(INTLLIBS)							\
@STRIP_END@

.PHONY: files

files:
	@files=`ls $(DISTFILES) 2> /dev/null`; for p in $$files; do \
	  echo $$p; \
	done
	@for subdir in $(SUBDIRS); do \
	  files=`cd $$subdir; $(MAKE) files | grep -v "make\[[1-9]\]"`; \
	  for file in $$files; do \
	    echo $$subdir/$$file; \
	  done; \
	done
