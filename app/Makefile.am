## Process this file with automake to produce Makefile.in

SUBDIRS = \
	paint-funcs	\
	base 		\
	core		\
	vectors		\
	config		\
	paint		\
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

##
## gui stuff for gui/, display/ or widgets/
##
gui_sources = \
	undo_history.c		\
	undo_history.h

##
## stuff which is about to be replaced by new subsystems
##
bye_sources = \
	general.c		\
	general.h		\
	ops_buttons.c		\
	ops_buttons.h		\
	path.c			\
	path.h			\
	pathP.h			\
	path_transform.h	\
	path_curves.h		\
	path_curves.c		\
	path_bezier.h		\
	path_bezier.c		\
	undo.c			\
	undo.h			\
	undo_types.h

##
## stuff
##
stuff_sources = \
	appenv.h		\
	app_procs.c		\
	app_procs.h		\
	main.c			\
	batch.c			\
	batch.h			\
	errors.c		\
	errors.h		\
	libgimp_glue.c		\
	libgimp_glue.h

gimp_1_3_SOURCES = \
	$(gui_sources)		\
	$(bye_sources)		\
	$(stuff_sources)

EXTRA_DIST = \
	makefile.mingw			\
	makefile.mingw.in		\
	makefile.msc			\
	gimp.rc				\
	gimp.sym			\
	wilber.ico			\
	arch/i386/mmx/detect_mmx.S	\
	arch/i386/mmx/paint_funcs_mmx.S

if HAVE_GLIBC_REGEX
REGEXREPL =
else
REGEXREPL = $(top_builddir)/regexrepl/libregex.a
endif

gimp_1_3_LDFLAGS = -export-dynamic -export-symbols $(srcdir)/gimp.sym

AM_CPPFLAGS = \
	-DG_LOG_DOMAIN=\"Gimp\"	\
	@GIMP_THREAD_FLAGS@ 	\
	@GIMP_MP_FLAGS@

INCLUDES = \
	-I$(top_srcdir)	\
	$(GTK_CFLAGS)	\
	-I$(includedir)

gimp_1_3_LDADD = \
	gui/libappgui.a							\
	display/libappdisplay.a						\
	tools/libapptools.a						\
	widgets/libappwidgets.a						\
	core/libappcore.a						\
	pdb/libapppdb.a							\
	vectors/libappvectors.a						\
	paint/libapppaint.a						\
	xcf/libappxcf.a							\
	file/libappfile.a						\
	plug-in/libappplug-in.a						\
	config/libappconfig.a						\
	paint-funcs/libapppaint-funcs.a					\
	base/libappbase.a						\
	$(top_builddir)/libgimpcolor/libgimpcolor-$(LT_RELEASE).la	\
	$(top_builddir)/libgimpmath/libgimpmath-$(LT_RELEASE).la	\
	$(top_builddir)/libgimpbase/libgimpbase-$(LT_RELEASE).la	\
	$(top_builddir)/libgimpmodule/libgimpmodule-$(LT_RELEASE).la	\
	$(top_builddir)/libgimpwidgets/libgimpwidgets-$(LT_RELEASE).la	\
	$(top_builddir)/libgimptool/libgimptool-$(LT_RELEASE).la	\
	$(GTK_LIBS)							\
	$(PANGOFT2_LIBS)						\
	$(LIBART_LIBS)							\
	$(GIMP_THREAD_LIBS)						\
	$(GIMP_MP_LIBS)							\
	$(INTLLIBS)							\
	$(REGEXREPL)

install-exec-hook:
if DEFAULT_BINARY
	cd $(DESTDIR)$(bindir) \
	&& rm -f gimp \
	&& $(LN_S) gimp-1.3 gimp
endif

uninstall-local:
if DEFAULT_BINARY
	rm -f $(DESTDIR)$(bindir)/gimp
endif
