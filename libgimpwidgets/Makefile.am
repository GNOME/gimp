## Process this file with automake to produce Makefile.in

libgimpwidgetsincludedir = $(includedir)/gimp-$(LT_RELEASE)/libgimpwidgets

AM_CPPFLAGS = \
	-DG_LOG_DOMAIN=\"LibGimpWidgets\"	\
	@GIMP_THREAD_FLAGS@

INCLUDES = \
	-I$(top_srcdir)	\
	$(GTK_CFLAGS)	\
	-I$(includedir)

lib_LTLIBRARIES = libgimpwidgets-1.3.la

libgimpwidgets_1_3_la_sources = \
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
	gimpcolordisplay.c	\
	gimpcolordisplay.h	\
	gimpcolornotebook.c	\
	gimpcolornotebook.h	\
	gimpcolorscale.c	\
	gimpcolorscale.h	\
	gimpcolorscales.c	\
	gimpcolorscales.h	\
	gimpcolorselector.c	\
	gimpcolorselector.h	\
	gimpcolorselect.c	\
	gimpcolorselect.h	\
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
	gimppickbutton.c	\
	gimppickbutton.h	\
	gimppixmap.c		\
	gimppixmap.h		\
	gimpquerybox.c		\
	gimpquerybox.h		\
	gimpsizeentry.c		\
	gimpsizeentry.h		\
	gimpstock.c		\
	gimpstock.h		\
	gimpunitmenu.c		\
	gimpunitmenu.h

libgimpwidgets_1_3_la_built_sources = \
	gimpwidgetsmarshal.c	\
	gimpwidgetsmarshal.h

libgimpwidgets_1_3_la_extra_sources = gimpwidgetsmarshal.list

EXTRA_DIST = \
	makefile.mingw				\
	makefile.mingw.in			\
	makefile.msc				\
	gimpwidgets.def				\
	$(libgimpwidgets_1_3_la_extra_sources)

libgimpwidgets_1_3_la_SOURCES = 		\
	$(libgimpwidgets_1_3_la_built_sources)	\
	$(libgimpwidgets_1_3_la_sources)

libgimpwidgetsinclude_HEADERS = \
	gimpwidgets.h		\
	gimpwidgetstypes.h	\
	gimpbutton.h		\
	gimpchainbutton.h	\
	gimpcolorarea.h		\
	gimpcolorbutton.h	\
	gimpcolordisplay.h	\
	gimpcolornotebook.h	\
	gimpcolorscale.h	\
	gimpcolorscales.h	\
	gimpcolorselector.h	\
	gimpcolorselect.h	\
	gimpdialog.h		\
	gimpfileselection.h	\
	gimphelpui.h		\
	gimpoffsetarea.h	\
	gimppatheditor.h	\
	gimppickbutton.h	\
	gimppixmap.h		\
	gimpquerybox.h		\
	gimpsizeentry.h		\
	gimpstock.h		\
	gimpunitmenu.h

libgimpwidgets_1_3_la_LDFLAGS = \
	-version-info $(LT_CURRENT):$(LT_REVISION):$(LT_AGE)

libgimpwidgets_1_3_la_LIBADD = $(GLIB_LIBS) -lm

#
# rules to generate built sources
#
# setup autogeneration dependancies
gen_sources = xgen-gwmh xgen-gwmc
CLEANFILES = $(gen_sources)

$(srcdir)/gimpwidgetsmarshal.h: $(srcdir)/gimpwidgetsmarshal.list
	$(GLIB_GENMARSHAL) --prefix=_gimp_widgets_marshal $(srcdir)/gimpwidgetsmarshal.list --header >> xgen-gwmh \
	&& (cmp -s xgen-gwmh $(@F) || cp xgen-gwmh $(@F)) \
	&& rm -f xgen-gwmh xgen-gwmh~

$(srcdir)/gimpwidgetsmarshal.c: $(srcdir)/gimpwidgetsmarshal.h
	echo "#include \"gimpwidgetsmarshal.h\"" >> xgen-gwmc \
	&& $(GLIB_GENMARSHAL) --prefix=_gimp_widgets_marshal $(srcdir)/gimpwidgetsmarshal.list --body >> xgen-gwmc \
	&& cp xgen-gwmc $(@F) \
	&& rm -f xgen-gwmc xgen-gwmc~

