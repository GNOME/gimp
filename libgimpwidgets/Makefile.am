## Process this file with automake to produce Makefile.in

libgimpwidgetsincludedir = $(includedir)/gimp-$(LT_RELEASE)/libgimpwidgets

AM_CPPFLAGS = @STRIP_BEGIN@ \
	-DGIMPDIR=\""$(gimpdir)"\"		\
	-DDATADIR=\""$(gimpdatadir)"\"		\
	-DSYSCONFDIR=\""$(gimpsysconfdir)"\"	\
	-DG_LOG_DOMAIN=\"LibGimpWidgets\"	\
	@GIMP_THREAD_FLAGS@			\
@STRIP_END@

INCLUDES = @STRIP_BEGIN@ \
	-I$(top_srcdir)		\
	$(GTK_CFLAGS)		\
	-I$(includedir)		\
@STRIP_END@

EXTRA_DIST = \
	makefile.mingw		\
	makefile.mingw.in	\
	makefile.msc		\
	gimpwidgets.def

lib_LTLIBRARIES = libgimpwidgets-1.3.la

libgimpwidgets_1_3_la_SOURCES = @STRIP_BEGIN@ \
	gimpwidgets.c		\
	gimpwidgets.h		\
	gimpwidgetstypes.h	\
	gimpbutton.c		\
	gimpbutton.h		\
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
	gimpoffsetarea.c	\
	gimpoffsetarea.h	\
	gimppatheditor.c	\
	gimppatheditor.h	\
	gimppixmap.c		\
	gimppixmap.h		\
	gimpquerybox.c		\
	gimpquerybox.h		\
	gimpsizeentry.c		\
	gimpsizeentry.h		\
	gimpstock.c		\
	gimpstock.h		\
	gimpunitmenu.c		\
	gimpunitmenu.h		\
@STRIP_END@

libgimpwidgetsinclude_HEADERS = @STRIP_BEGIN@ \
	gimpwidgets.h		\
	gimpwidgetstypes.h	\
	gimpbutton.h		\
	gimpchainbutton.h	\
	gimpcolorarea.h		\
	gimpcolorbutton.h	\
	gimpdialog.h		\
	gimpfileselection.h	\
	gimphelpui.h		\
	gimpoffsetarea.h	\
	gimppatheditor.h	\
	gimppixmap.h		\
	gimpquerybox.h		\
	gimpsizeentry.h		\
	gimpstock.h		\
	gimpunitmenu.h		\
@STRIP_END@

EXTRA_HEADERS +=

libgimpwidgets_1_3_la_LDFLAGS = @STRIP_BEGIN@ \
	-version-info $(LT_CURRENT):$(LT_REVISION):$(LT_AGE)	\
@STRIP_END@

libgimpwidgets_1_3_la_LIBADD = $(GLIB_LIBS) -lm

## this is probably cruft, but may not be.  -klm
##makefile.mingw: $(top_builddir)/config.status $(top_srcdir)/libgimpwidgets/makefile.mingw.in
##	cd $(top_builddir) && CONFIG_FILES=libgimpwidgets/$@ CONFIG_HEADERS= $(SHELL) ./config.status
