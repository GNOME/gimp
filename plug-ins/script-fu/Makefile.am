## Process this file with automake to produce Makefile.in

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
	script-fu-text-console.h	\
	script-fu-text-console.c	\
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
	siod-wrapper.c		\
	siod-wrapper.h		\
@STRIP_END@

AM_CPPFLAGS = -DLOCALEDIR=\""$(localedir)"\"

INCLUDES = @STRIP_BEGIN@ \
	-I$(top_srcdir)			\
	$(GTK_CFLAGS)			\
	-I$(includedir)			\
@STRIP_END@

if HAVE_GLIBC_REGEX
REGEXREPL =
else
REGEXREPL = $(top_builddir)/regexrepl/libregex.a
endif

LDADD = @STRIP_BEGIN@ \
	$(top_builddir)/plug-ins/dbbrowser/dbbrowser_utils.o		\
	$(top_builddir)/libgimp/libgimpui-$(LT_RELEASE).la		\
	$(top_builddir)/libgimpwidgets/libgimpwidgets-$(LT_RELEASE).la	\
	$(top_builddir)/libgimp/libgimp-$(LT_RELEASE).la		\
	$(top_builddir)/libgimpcolor/libgimpcolor-$(LT_RELEASE).la	\
	$(top_builddir)/libgimpbase/libgimpbase-$(LT_RELEASE).la	\
	$(GTK_LIBS)							\
	$(INTLLIBS)							\
	$(REGEXREPL)							\
@STRIP_END@
