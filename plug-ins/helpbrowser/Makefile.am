## Process this file with automake to produce Makefile.in

libexecdir = $(gimpplugindir)/plug-ins

libexec_PROGRAMS = helpbrowser

helpbrowser_SOURCES = \
	back.xpm		\
	forward.xpm		\
	helpbrowser.c		\
	queue.c			\
	queue.h

INCLUDES = \
	-I$(top_srcdir)		\
	$(GTK_CFLAGS)		\
	$(GTKXMHTML_CFLAGS)	\
	-I$(includedir)

LDADD = \
	$(top_builddir)/libgimp/libgimpui-$(LT_RELEASE).la		\
	$(top_builddir)/libgimpwidgets/libgimpwidgets-$(LT_RELEASE).la	\
	$(top_builddir)/libgimp/libgimp-$(LT_RELEASE).la		\
	$(top_builddir)/libgimpcolor/libgimpcolor-$(LT_RELEASE).la	\
	$(GTK_LIBS)							\
	$(GTKXMHTML_LIBS)						\
	$(INTLLIBS)

AM_CPPFLAGS = \
	-DLOCALEDIR=\""$(localedir)"\"

.PHONY: files

files:
	@files=`ls $(DISTFILES) 2> /dev/null`; for p in $$files; do \
	  echo $$p; \
	done
