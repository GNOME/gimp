## Process this file with automake to produce Makefile.in

libexecdir = $(gimpplugindir)/plug-ins

libexec_PROGRAMS = plugin-helper 

plugin_helper_SOURCES = \
	plugin-helper.c

AM_CPPFLAGS = \
        -DLOCALEDIR=\""$(localedir)"\"

INCLUDES = \
	-I$(top_srcdir)			\
	$(GTK_CFLAGS)			\
	-I$(includedir)

LDADD = \
        $(top_builddir)/libgimp/libgimpui.la                    \
        $(top_builddir)/libgimpwidgets/libgimpwidgets.la        \
        $(top_builddir)/libgimp/libgimp.la                      \
        $(top_builddir)/libgimpcolor/libgimpcolor.la            \
        $(GTK_LIBS)                                             \
        $(INTLLIBS)


.PHONY: files

files:
	@files=`ls $(DISTFILES) 2> /dev/null`; for p in $$files; do \
	  echo $$p; \
	done
