## Process this file with automake to produce Makefile.in

libgimpbase = $(top_builddir)/libgimpbase/libgimpbase-$(GIMP_API_VERSION).la
libgimpcolor = $(top_builddir)/libgimpcolor/libgimpcolor-$(GIMP_API_VERSION).la

if PLATFORM_WIN32
no_undefined = -no-undefined
endif

if PLATFORM_WIN32
else
libm = -lm
endif

if OS_WIN32
gimpwidgets_def = gimpwidgets.def
libgimpwidgets_export_symbols = -export-symbols gimpwidgets.def

install-libtool-import-lib:
	$(INSTALL) .libs/libgimpwidgets-$(GIMP_API_VERSION).dll.a $(DESTDIR)$(libdir)
	$(INSTALL) gimpwidgets.def $(DESTDIR)$(libdir)

uninstall-libtool-import-lib:
	-rm $(DESTDIR)$(libdir)/libgimpwidgets-$(GIMP_API_VERSION).dll.a
	-rm $(DESTDIR)$(libdir)/gimpwidgets.def
else
install-libtool-import-lib:
uninstall-libtool-import-lib:
endif

if MS_LIB_AVAILABLE
noinst_DATA = gimpwidgets-$(GIMP_API_VERSION).lib

install-ms-lib:
	$(INSTALL) gimpwidgets-$(GIMP_API_VERSION).lib $(DESTDIR)$(libdir)

uninstall-ms-lib:
	-rm $(DESTDIR)$(libdir)/gimpwidgets-$(GIMP_API_VERSION).lib

gimpwidgets-@GIMP_API_VERSION@.lib: gimpwidgets.def
	lib -name:libgimpwidgets-$(GIMP_API_VERSION)-@LT_CURRENT_MINUS_AGE@.dll -def:gimpwidgets.def -out:$@

else
install-ms-lib:
uninstall-ms-lib:
endif

libgimpwidgetsincludedir = $(includedir)/gimp-$(GIMP_API_VERSION)/libgimpwidgets

AM_CPPFLAGS = \
	-DG_LOG_DOMAIN=\"LibGimpWidgets\"			\
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
	gimpcolordisplaystack.c	\
	gimpcolordisplaystack.h	\
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
	gimpcolorselection.c	\
	gimpcolorselection.h	\
	gimpdialog.c		\
	gimpdialog.h		\
	gimpfileentry.c		\
	gimpfileentry.h		\
	gimphelpui.c		\
	gimphelpui.h		\
	gimpmemsizeentry.c	\
	gimpmemsizeentry.h	\
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
	gimpunitmenu.h		\
	gimpwidgets-private.c	\
	gimpwidgets-private.h

libgimpwidgets_1_3_la_built_sources = \
	gimpwidgetsmarshal.c	\
	gimpwidgetsmarshal.h

libgimpwidgets_1_3_la_extra_sources = gimpwidgetsmarshal.list

EXTRA_DIST = \
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
	gimpcolordisplaystack.h	\
	gimpcolornotebook.h	\
	gimpcolorscale.h	\
	gimpcolorscales.h	\
	gimpcolorselector.h	\
	gimpcolorselect.h	\
	gimpcolorselection.h	\
	gimpdialog.h		\
	gimpfileentry.h		\
	gimphelpui.h		\
	gimpmemsizeentry.h	\
	gimpoffsetarea.h	\
	gimppatheditor.h	\
	gimppickbutton.h	\
	gimppixmap.h		\
	gimpquerybox.h		\
	gimpsizeentry.h		\
	gimpstock.h		\
	gimpunitmenu.h

libgimpwidgets_1_3_la_LDFLAGS = \
	-version-info $(LT_VERSION_INFO)	\
	$(no_undefined) 			\
	$(libgimpwidgets_export_symbols)

libgimpwidgets_1_3_la_LIBADD = \
	$(libgimpbase)	\
	$(libgimpcolor) \
	$(GTK_LIBS) 	\
	$(libm)

libgimpwidgets_1_3_la_DEPENDENCIES = $(gimpwidgets_def)

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

install-data-local: install-ms-lib install-libtool-import-lib

uninstall-local: uninstall-ms-lib uninstall-libtool-import-lib
