## Process this file with automake to produce Makefile.in

libgimpbase = $(top_builddir)/libgimpbase/libgimpbase-$(GIMP_API_VERSION).la
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
	xcf		\
	file		\
	plug-in		\
	tools		\
	widgets		\
	display		\
	actions		\
	menus		\
	gui		\
	pdb

scriptdata =

if ENABLE_GIMP_CONSOLE
bin_PROGRAMS = gimp-2.1 gimp-console-2.1
else
bin_PROGRAMS = gimp-2.1
endif

gimp_2_1_SOURCES = \
	app_procs.c	\
	app_procs.h	\
	main.c		\
	batch.c		\
	batch.h		\
	errors.c	\
	errors.h	\
	sanity.c	\
	sanity.h	\
	units.c		\
	units.h		\
	gimp-intl.h

EXTRA_DIST = \
	makefile.msc	\
	gimp.rc		\
	wilber.ico

if HAVE_GLIBC_REGEX
REGEXREPL =
else
REGEXREPL = $(top_builddir)/regexrepl/libregex.a
endif

if OS_WIN32
mwindows = -mwindows
endif

AM_CPPFLAGS = \
	-DG_LOG_DOMAIN=\"Gimp\"		\
	-DGIMP_APP_GLUE_COMPILATION	\
	@GIMP_THREAD_FLAGS@ 		\
	@GIMP_MP_FLAGS@

INCLUDES = \
	-I$(top_srcdir)	\
	$(GTK_CFLAGS)	\
	$(PANGOFT2_CFLAGS) \
	-I$(includedir)

gimp_2_1_LDFLAGS = \
	$(mwindows)					\
	-u $(SYMPREFIX)gimp_container_filter		\
	-u $(SYMPREFIX)gimp_xml_parser_new		\
	-u $(SYMPREFIX)gimp_drawable_stroke_vectors	\
	\
	-u $(SYMPREFIX)convert_dialog_new		\
	-u $(SYMPREFIX)grid_dialog_new			\
	-u $(SYMPREFIX)info_dialog_new			\
	-u $(SYMPREFIX)info_window_create		\
	-u $(SYMPREFIX)offset_dialog_new		\
	-u $(SYMPREFIX)palette_import_dialog_show	\
	-u $(SYMPREFIX)resize_dialog_new		\
	-u $(SYMPREFIX)stroke_dialog_new		\
	-u $(SYMPREFIX)file_open_dialog_show		\
	-u $(SYMPREFIX)file_save_dialog_show		\
	-u $(SYMPREFIX)color_notebook_new

gimp_2_1_LDADD = \
	config/libappconfig.a		\
	gui/libappgui.a			\
	menus/libappmenus.a		\
	actions/libappactions.a		\
	display/libappdisplay.a		\
	tools/libapptools.a		\
	widgets/libappwidgets.a		\
	core/libappcore.a		\
	pdb/libapppdb.a			\
	paint/libapppaint.a		\
	xcf/libappxcf.a			\
	text/libapptext.a		\
	vectors/libappvectors.a		\
	file/libappfile.a		\
	plug-in/libappplug-in.a		\
	paint-funcs/libapppaint-funcs.a	\
	composite/libappcomposite.a	\
	base/libappbase.a		\
	$(libgimpwidgets)		\
	$(libgimpmodule)		\
	$(libgimpcolor)			\
	$(libgimpthumb)			\
	$(libgimpmath)			\
	$(libgimpbase)			\
	$(GTK_LIBS)			\
	$(LIBART_LIBS)			\
	$(PANGOFT2_LIBS)		\
	$(FONTCONFIG_LIBS)		\
	$(FREETYPE_LIBS)		\
	$(GIMP_THREAD_LIBS)		\
	$(GIMP_MP_LIBS)			\
	$(RT_LIBS)			\
	$(INTLLIBS)			\
	$(REGEXREPL)

if ENABLE_GIMP_CONSOLE
gimp_console_2_1_SOURCES = $(gimp_2_1_SOURCES)
gimp_console_2_1_LDFLAGS = $(gimp_2_1_LDFLAGS)

gimp_console_2_1_LDADD = \
	config/libappconfig.a		\
	pdb/libapppdb.a			\
	core/libappcore.a		\
	paint/libapppaint.a		\
	xcf/libappxcf.a			\
	text/libapptext.a		\
	vectors/libappvectors.a		\
	file/libappfile.a		\
	plug-in/libappplug-in.a		\
	paint-funcs/libapppaint-funcs.a	\
	composite/libappcomposite.a	\
	base/libappbase.a		\
	$(libgimpmodule)		\
	$(libgimpcolor)			\
	$(libgimpthumb)			\
	$(libgimpmath)			\
	$(libgimpbase)			\
	$(GDK_PIXBUF_LIBS)		\
	$(LIBART_LIBS)			\
	$(PANGOFT2_LIBS)		\
	$(FONTCONFIG_LIBS)		\
	$(FREETYPE_LIBS)		\
	$(GIMP_THREAD_LIBS)		\
	$(GIMP_MP_LIBS)			\
	$(RT_LIBS)			\
	$(INTLLIBS)			\
	$(REGEXREPL)
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


# hooks to assure that the system gimprc and the gimprc manpage are
# uptodate when a release is made

dist-hook: gimp-$(GIMP_APP_VERSION)$(EXEEXT)
	./$< --dump-gimprc-system > $(top_srcdir)/etc/gimprc
	./$< --dump-gimprc-manpage > $(top_srcdir)/docs/gimprc.5.in
