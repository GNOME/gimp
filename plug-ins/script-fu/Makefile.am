## Process this file with automake to produce Makefile.in

if OS_WIN32
mwindows = -mwindows
endif

LDFLAGS = @LDFLAGS@ $(mwindows)

SUBDIRS = scripts

libexecdir = $(gimpplugindir)/plug-ins

libexec_PROGRAMS = script-fu

script_fu_SOURCES = \
	interp_md5.c		\
	interp_md5.h		\
	interp_regex.c		\
	interp_slib.c		\
	interp_sliba.c		\
	interp_trace.c		\
	regex.c			\
	regex.h			\
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
	siodp.h

EXTRA_DIST = \
	script-fu.ico		\
	script-fu.rc

INCLUDES = \
	-I$(top_srcdir)			\
	-I$(top_srcdir)/intl		\
	$(GTK_CFLAGS)			\
	-I$(includedir)

LDADD = \
	$(top_builddir)/plug-ins/dbbrowser/dbbrowser_utils.o	\
	$(top_builddir)/libgimp/libgimpui.la			\
	$(top_builddir)/libgimp/libgimp.la			\
	$(GTK_LIBS)						\
	$(INTLLIBS)

AM_CPPFLAGS = \
	-DREGEX_MALLOC			\
	-DLOCALEDIR=\""$(localedir)"\"

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
