## Process this file with automake to produce Makefile.in

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
	gui		\
	pdb

scriptdata =

if ENABLE_GIMP_CONSOLE
bin_PROGRAMS = gimp-1.3 gimp-console-1.3
else
bin_PROGRAMS = gimp-1.3
endif

gimp_1_3_SOURCES = \
	app_procs.c	\
	app_procs.h	\
	main.c		\
	batch.c		\
	batch.h		\
	errors.c	\
	errors.h	\
	gimp-intl.h	\
	libgimp_glue.c	\
	libgimp_glue.h

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

gimp_1_3_LDFLAGS = \
	$(mwindows)				\
	-u $(SYMPREFIX)gimp_container_filter	\
	-u $(SYMPREFIX)gimp_xml_parser_new	\
	-u $(SYMPREFIX)gimp_drawable_stroke_vectors

AM_CPPFLAGS = \
	-DG_LOG_DOMAIN=\"Gimp\"		\
	-DGIMP_APP_GLUE_COMPILATION	\
	@GIMP_THREAD_FLAGS@ 		\
	@GIMP_MP_FLAGS@

INCLUDES = \
	-I$(top_srcdir)	\
	$(GTK_CFLAGS)	\
	-I$(includedir)

gimp_1_3_LDADD = \
	config/libappconfig.a						\
	gui/libappgui.a							\
	display/libappdisplay.a						\
	tools/libapptools.a						\
	widgets/libappwidgets.a						\
	core/libappcore.a						\
	pdb/libapppdb.a							\
	paint/libapppaint.a						\
	xcf/libappxcf.a							\
	text/libapptext.a						\
	vectors/libappvectors.a						\
	file/libappfile.a						\
	plug-in/libappplug-in.a						\
	paint-funcs/libapppaint-funcs.a					\
	composite/libappcomposite.a					\
	base/libappbase.a						\
	$(top_builddir)/libgimpcolor/libgimpcolor-$(LT_RELEASE).la	\
	$(top_builddir)/libgimpmath/libgimpmath-$(LT_RELEASE).la	\
	$(top_builddir)/libgimpbase/libgimpbase-$(LT_RELEASE).la	\
	$(top_builddir)/libgimpmodule/libgimpmodule-$(LT_RELEASE).la	\
	$(top_builddir)/libgimpwidgets/libgimpwidgets-$(LT_RELEASE).la	\
	$(GTK_LIBS)							\
	$(LIBART_LIBS)							\
	$(PANGOFT2_LIBS)						\
	$(FONTCONFIG_LIBS)						\
	$(FREETYPE_LIBS)						\
	$(GIMP_THREAD_LIBS)						\
	$(GIMP_MP_LIBS)							\
	$(INTLLIBS)							\
	$(REGEXREPL)

if ENABLE_GIMP_CONSOLE
gimp_console_1_3_SOURCES = $(gimp_1_3_SOURCES)

gimp_console_1_3_LDADD = \
	config/libappconfig.a						\
	core/libappcore.a						\
	pdb/libapppdb.a							\
	paint/libapppaint.a						\
	xcf/libappxcf.a							\
	text/libapptext.a						\
	vectors/libappvectors.a						\
	file/libappfile.a						\
	plug-in/libappplug-in.a						\
	paint-funcs/libapppaint-funcs.a					\
	base/libappbase.a						\
	composite/libappcomposite.a					\
	$(top_builddir)/libgimpcolor/libgimpcolor-$(LT_RELEASE).la	\
	$(top_builddir)/libgimpmath/libgimpmath-$(LT_RELEASE).la	\
	$(top_builddir)/libgimpbase/libgimpbase-$(LT_RELEASE).la	\
	$(top_builddir)/libgimpmodule/libgimpmodule-$(LT_RELEASE).la	\
	$(GDK_PIXBUF_LIBS)						\
	$(LIBART_LIBS)							\
	$(PANGOFT2_LIBS)						\
	$(FONTCONFIG_LIBS)						\
	$(FREETYPE_LIBS)						\
	$(GIMP_THREAD_LIBS)						\
	$(GIMP_MP_LIBS)							\
	$(INTLLIBS)							\
	$(REGEXREPL)
endif

install-exec-hook:
if DEFAULT_BINARY
	cd $(DESTDIR)$(bindir) \
	&& rm -f gimp$(EXEEXT) \
	&& $(LN_S) gimp-$(LT_RELEASE)$(EXEEXT) gimp$(EXEEXT)
if ENABLE_GIMP_CONSOLE
	cd $(DESTDIR)$(bindir) \
	&& rm -f gimp-console$(EXEEXT) \
	&& $(LN_S) gimp-console-$(LT_RELEASE)$(EXEEXT) gimp-console$(EXEEXT)
endif
endif

uninstall-local:
if DEFAULT_BINARY
	rm -f $(DESTDIR)$(bindir)/gimp$(EXEEXT)
if ENABLE_GIMP_CONSOLE
	rm -f $(DESTDIR)$(bindir)/gimp-console-$(EXEEXT)
endif
endif


# hooks to assure that the system gimprc and the gimprc manpage are
# uptodate when a release is made

dist-hook: gimp-1.3
	./$< --dump-gimprc-system > $(top_srcdir)/etc/gimprc
	./$< --dump-gimprc-manpage > $(top_srcdir)/docs/gimprc-$(LT_RELEASE).5.in
