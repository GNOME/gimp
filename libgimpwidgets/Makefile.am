## Process this file with automake to produce Makefile.in

libgimpwidgetsincludedir = $(includedir)/libgimpwidgets

AM_CPPFLAGS = \
	-DGIMPDIR=\""$(gimpdir)"\"		\
	-DDATADIR=\""$(gimpdatadir)"\"		\
	-DSYSCONFDIR=\""$(gimpsysconfdir)"\"	\
	-DG_LOG_DOMAIN=\"LibGimpWidgets\"	\
	@GIMP_THREAD_FLAGS@

INCLUDES = \
	-I$(top_srcdir)		\
	-I$(top_srcdir)/intl	\
	-I../intl		\
	$(GTK_CFLAGS)		\
	-I$(includedir)

EXTRA_DIST = \
	makefile.mingw		\
	makefile.mingw.in	\
	makefile.msc		\
	gimpwidgets.def

lib_LTLIBRARIES = libgimpwidgets.la

libgimpwidgets_la_SOURCES = \
	gimpwidgets.c		\
	gimpwidgets.h		\
	gimpwidgetstypes.h	\
	gimpchainbutton.c	\
	gimpchainbutton.h	\
	gimpcolorarea.c		\
	gimpcolorarea.h		\
	gimpcolorbutton.c	\
	gimpcolorbutton.h	\
	gimpdialog.c		\
	gimpdialog.h		\
	gimpfileselection.c	\
	gimpfileselection.h	\
	gimphelpui.c		\
	gimphelpui.h		\
	gimppatheditor.c	\
	gimppatheditor.h	\
	gimppixmap.c		\
	gimppixmap.h		\
	gimpquerybox.c		\
	gimpquerybox.h		\
	gimpsizeentry.c		\
	gimpsizeentry.h		\
	gimpunitmenu.c		\
	gimpunitmenu.h

libgimpwidgetsinclude_HEADERS = \
	gimpwidgets.h		\
	gimpwidgetstypes.h	\
	gimpchainbutton.h	\
	gimpcolorarea.h		\
	gimpcolorbutton.h	\
	gimpdialog.h		\
	gimpfileselection.h	\
	gimphelpui.h		\
	gimppatheditor.h	\
	gimppixmap.h		\
	gimpquerybox.h		\
	gimpsizeentry.h		\
	gimpunitmenu.h

libgimpwidgets_la_LDFLAGS = \
	-version-info $(LT_CURRENT):$(LT_REVISION):$(LT_AGE) \
	-release $(LT_RELEASE)
libgimpwidgets_la_LIBADD = $(GLIB_LIBS) -lm

.PHONY: files

files:
	@files=`ls $(DISTFILES) 2> /dev/null`; for p in $$files; do \
	  echo $$p; \
	done

makefile.mingw: $(top_builddir)/config.status $(top_srcdir)/libgimpwidgets/makefile.mingw.in
	cd $(top_builddir) && CONFIG_FILES=libgimpwidgets/$@ CONFIG_HEADERS= $(SHELL) ./config.status
