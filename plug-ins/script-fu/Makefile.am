## Process this file with automake to produce Makefile.in

SUBDIRS = scripts

scriptdata =

pluginlibdir = $(gimpplugindir)/plug-ins

pluginlib_PROGRAMS = script-fu

script_fu_SOURCES = \
	interp_md5.c		\
	interp_md5.h		\
	interp_regex.c		\
	interp_slib.c		\
	interp_sliba.c		\
	interp_trace.c		\
	script-fu.c		\
	script-fu-console.c	\
	script-fu-console.h	\
	script-fu-scripts.c	\
	script-fu-scripts.h	\
	script-fu-server.c	\
	script-fu-server.h	\
	script-fu-enums.h	\
	siod.h			\
	siodp.h			\
	regex.h			\
	regex.c

INCLUDES = \
	$(GTK_CFLAGS)			\
	-I$(top_srcdir)			\
	-I$(includedir)

LDADD = \
	$(top_builddir)/plug-ins/dbbrowser/dbbrowser_utils.o	\
	$(top_builddir)/libgimp/libgimpui-@LT_RELEASE@.la	\
	$(top_builddir)/libgimp/libgimp-@LT_RELEASE@.la		\
	$(GTK_LIBS)						\
	-lc

DEPS = \
	$(top_builddir)/libgimp/libgimpui-@LT_RELEASE@.la	\
	$(top_builddir)/libgimp/libgimp-@LT_RELEASE@.la		\
	$(top_builddir)/plug-ins/dbbrowser/dbbrowser_utils.o

script_fu_DEPENDENCIES = $(DEPS)

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
