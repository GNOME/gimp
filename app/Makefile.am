## Process this file with automake to produce Makefile.in

if OS_WIN32
mwindows = -mwindows
endif

SUBDIRS = \
	paint-funcs	\
	base 		\
	core		\
	config		\
	paint		\
	text		\
	vectors		\
	xcf		\
	file		\
	plug-in		\
	tools		\
	widgets		\
	gui		\
	display		\
	pdb

scriptdata =

bin_PROGRAMS = gimp-1.3

gimp_1_3_SOURCES = \
	appenv.h	\
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

gimp_1_3_LDFLAGS = -export-dynamic $(mwindows)

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
	base/libappbase.a						\
	$(top_builddir)/libgimpcolor/libgimpcolor-$(LT_RELEASE).la	\
	$(top_builddir)/libgimpmath/libgimpmath-$(LT_RELEASE).la	\
	$(top_builddir)/libgimpbase/libgimpbase-$(LT_RELEASE).la	\
	$(top_builddir)/libgimpmodule/libgimpmodule-$(LT_RELEASE).la	\
	$(top_builddir)/libgimpwidgets/libgimpwidgets-$(LT_RELEASE).la	\
	$(GTK_LIBS)							\
	$(PANGOFT2_LIBS)						\
	$(LIBART_LIBS)							\
	$(GIMP_THREAD_LIBS)						\
	$(GIMP_MP_LIBS)							\
	$(FONTCONFIG_LIBS)						\
	$(INTLLIBS)							\
	$(REGEXREPL)

install-exec-hook:
if DEFAULT_BINARY
	cd $(DESTDIR)$(bindir) \
	&& rm -f gimp$(EXEEXT) \
	&& $(LN_S) gimp-$(LT_RELEASE)$(EXEEXT) gimp$(EXEEXT)
endif

uninstall-local:
if DEFAULT_BINARY
	rm -f $(DESTDIR)$(bindir)/gimp$(EXEEXT)
endif
