## Process this file with automake to produce Makefile.in

##SUBDIRS = common			

libexecdir = $(gimpplugindir)/plug-ins

libexec_PROGRAMS = tool-safe-mode 

tool_safe_mode_SOURCES = \
	tool-safe-mode.c		\
	tool-safe-mode.h		\
	tool-safe-mode-plug-in.c	\
	tool-safe-mode-plug-in.h	\
	gimptool.c			\
	gimptoolcontrol.c		\
	gimptoolcontrol.h

AM_CPPFLAGS = \
	-DLOCALEDIR=\""$(localedir)"\"

INCLUDES = \
	-I$(top_srcdir)			\
	-I$(top_srcdir)/app		\
	-I$(top_srcdir)/app/core	\
	$(GTK_CFLAGS)			\
	-I$(includedir)

LDADD = \
	$(top_builddir)/libgimp/libgimpui-$(LT_RELEASE).la		\
	$(top_builddir)/libgimpwidgets/libgimpwidgets-$(LT_RELEASE).la	\
	$(top_builddir)/libgimp/libgimp-$(LT_RELEASE).la		\
	$(top_builddir)/libgimpcolor/libgimpcolor-$(LT_RELEASE).la	\
	$(top_builddir)/libgimpproxy/libgimpproxy-$(LT_RELEASE).la	\
	$(top_builddir)/libgimptool/libgimptool-$(LT_RELEASE).la	\
	$(top_builddir)/libgimpbase/libgimpbase-$(LT_RELEASE).la	\
	$(GTK_LIBS)							\
	$(INTLLIBS)
