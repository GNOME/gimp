## Process this file with automake to produce Makefile.in

libgimpbase = $(top_builddir)/libgimpbase/libgimpbase-$(GIMP_API_VERSION).la
libgimpconfig = $(top_builddir)/libgimpconfig/libgimpconfig-$(GIMP_API_VERSION).la
libgimpcolor = $(top_builddir)/libgimpcolor/libgimpcolor-$(GIMP_API_VERSION).la
libgimpmath = $(top_builddir)/libgimpmath/libgimpmath-$(GIMP_API_VERSION).la
libgimpmodule = $(top_builddir)/libgimpmodule/libgimpmodule-$(GIMP_API_VERSION).la
libgimpwidgets = $(top_builddir)/libgimpwidgets/libgimpwidgets-$(GIMP_API_VERSION).la
libgimpthumb = $(top_builddir)/libgimpthumb/libgimpthumb-$(GIMP_API_VERSION).la


SUBDIRS = \
	paint-funcs	\
	composite	\
	base 		\
	config		\
	core		\
	paint		\
	text		\
	vectors		\
	plug-in		\
	file		\
	xcf		\
	pdb		\
	widgets		\
	display		\
	tools		\
	dialogs		\
	actions		\
	menus		\
	gui


if ENABLE_GIMP_CONSOLE
bin_PROGRAMS = gimp-2.4 gimp-console-2.4
else
bin_PROGRAMS = gimp-2.4
endif

app_sources = \
	about.h		\
	app.c		\
	app.h		\
	batch.c		\
	batch.h		\
	errors.c	\
	errors.h	\
	main.c		\
	sanity.c	\
	sanity.h	\
	units.c		\
	units.h		\
	gimp-intl.h

gimp_2_4_SOURCES = $(app_sources)


EXTRA_DIST = \
	makefile.msc	\
	gimp.rc		\
	fileicon.ico	\
	wilber.ico


if HAVE_GLIBC_REGEX
REGEXREPL =
else
REGEXREPL = $(top_builddir)/regexrepl/libregex.a
endif

if OS_WIN32
mwindows = -mwindows
endif

if USE_BINRELOC
munix = -Wl,-rpath '-Wl,$$ORIGIN/../lib'
endif

if HAVE_WINDRES
GIMPICONRC = gimprc.o
endif

AM_CPPFLAGS = \
	-DG_LOG_DOMAIN=\"Gimp\"		\
	-DGIMP_APP_GLUE_COMPILATION

INCLUDES = \
	-I$(top_srcdir)		\
	$(GTK_CFLAGS)		\
	$(PANGOFT2_CFLAGS)	\
	$(DBUS_GLIB_CFLAGS)	\
	-I$(includedir)

AM_LDFLAGS = $(mwindows) $(munix) \
	$(CARBON_LDFLAGS)			\
	-u $(SYMPREFIX)xcf_init			\
	-u $(SYMPREFIX)internal_procs_init	\
	-u $(SYMPREFIX)gimp_coords_mix		\
	-u $(SYMPREFIX)gimp_plug_in_manager_restore

gimp_2_4_LDADD = \
	gui/libappgui.a			\
	actions/libappactions.a		\
	dialogs/libappdialogs.a		\
	menus/libappmenus.a		\
	display/libappdisplay.a		\
	tools/libapptools.a		\
	widgets/libappwidgets.a		\
	pdb/libappinternal-procs.a	\
	xcf/libappxcf.a			\
	file/libappfile.a		\
	plug-in/libappplug-in.a		\
	pdb/libapppdb.a			\
	core/libappcore.a		\
	vectors/libappvectors.a		\
	paint/libapppaint.a		\
	text/libapptext.a		\
	config/libappconfig.a		\
	paint-funcs/libapppaint-funcs.a	\
	composite/libappcomposite.a	\
	base/libappbase.a		\
	$(libgimpwidgets)		\
	$(libgimpmodule)		\
	$(libgimpcolor)			\
	$(libgimpthumb)			\
	$(libgimpmath)			\
	$(libgimpconfig)		\
	$(libgimpbase)			\
	$(REGEXREPL)			\
	$(GTK_LIBS)			\
	$(LIBART_LIBS)			\
	$(PANGOFT2_LIBS)		\
	$(FONTCONFIG_LIBS)		\
	$(FREETYPE_LIBS)		\
	$(DBUS_GLIB_LIBS)		\
	$(RT_LIBS)			\
	$(INTLLIBS)			\
	$(GIMPICONRC)


if ENABLE_GIMP_CONSOLE

gimp_console_2_4_SOURCES = $(app_sources)

gimp_console_2_4_CPPFLAGS = \
	$(AM_CPPFLAGS)			\
	-DGIMP_CONSOLE_COMPILATION

gimp_console_2_4_LDADD = \
	widgets/widgets-enums.o		\
	display/display-enums.o		\
	display/gimpdisplayoptions.o	\
	pdb/libappinternal-procs.a	\
	xcf/libappxcf.a			\
	file/libappfile.a		\
	plug-in/libappplug-in.a		\
	pdb/libapppdb.a			\
	core/libappcore.a		\
	vectors/libappvectors.a		\
	paint/libapppaint.a		\
	text/libapptext.a		\
	config/libappconfig.a		\
	paint-funcs/libapppaint-funcs.a	\
	composite/libappcomposite.a	\
	base/libappbase.a		\
	$(libgimpmodule)		\
	$(libgimpcolor)			\
	$(libgimpthumb)			\
	$(libgimpmath)			\
	$(libgimpconfig)		\
	$(libgimpbase)			\
	$(REGEXREPL)			\
	$(GDK_PIXBUF_LIBS)		\
	$(LIBART_LIBS)			\
	$(PANGOFT2_LIBS)		\
	$(FONTCONFIG_LIBS)		\
	$(FREETYPE_LIBS)		\
	$(GLIB_LIBS)			\
	$(RT_LIBS)			\
	$(INTLLIBS)			\
	$(GIMPICONRC)

endif


install-exec-hook:
if DEFAULT_BINARY
	cd $(DESTDIR)$(bindir) \
	&& rm -f gimp$(EXEEXT) \
	&& $(LN_S) gimp-$(GIMP_APP_VERSION)$(EXEEXT) gimp$(EXEEXT)
if ENABLE_GIMP_CONSOLE
	cd $(DESTDIR)$(bindir) \
	&& rm -f gimp-console$(EXEEXT) \
	&& $(LN_S) gimp-console-$(GIMP_APP_VERSION)$(EXEEXT) gimp-console$(EXEEXT)
endif
endif

uninstall-local:
if DEFAULT_BINARY
	rm -f $(DESTDIR)$(bindir)/gimp$(EXEEXT)
if ENABLE_GIMP_CONSOLE
	rm -f $(DESTDIR)$(bindir)/gimp-console$(EXEEXT)
endif
endif


# require gimp-console when making dist
#
if ENABLE_GIMP_CONSOLE
dist-check-gimp-console:
else
dist-check-gimp-console:
	@echo "*** gimp-console must be enabled in order to make dist"
	@false
endif


# hook to assure that the system gimprc and the gimprc manpage are
# uptodate when a release is made
#
dist-dump-gimprc: gimp-console-$(GIMP_APP_VERSION)$(EXEEXT)
	./$< --dump-gimprc-system > gimprc.tmp	\
		&& sed -e "s/num-processors [0-9]*/num-processors 1/"	\
		   gimprc.tmp > gimprc.tmp2				\
		&& (cmp -s gimprc.tmp2 $(top_srcdir)/etc/gimprc ||	\
		    cp gimprc.tmp2 $(top_srcdir)/etc/gimprc)		\
		&& rm gimprc.tmp gimprc.tmp2
	./$< --dump-gimprc-manpage > gimprc.tmp	\
		&& sed -e "s/num-processors [0-9]*/num-processors 1/"	\
		   gimprc.tmp > gimprc.tmp2				\
		&& (cmp -s gimprc.tmp2 $(top_srcdir)/docs/gimprc.5.in ||\
		    cp gimprc.tmp2 $(top_srcdir)/docs/gimprc.5.in)	\
		&& rm gimprc.tmp gimprc.tmp2

dist-hook: dist-check-gimp-console dist-dump-gimprc

gimprc.o:
	$(WINDRES) gimp.rc gimprc.o
