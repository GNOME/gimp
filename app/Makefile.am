## Process this file with automake to produce Makefile.in

SUBDIRS = paint-funcs base core pdb plug-in xcf tools display widgets gui

scriptdata =

bin_PROGRAMS = gimp

gimp_SOURCES = @STRIP_BEGIN@ \
##
## gui stuff that needs to go to libgimp/, gui/ or /dev/null
##
	colormaps.c		\
	colormaps.h		\
	devices.c		\
	devices.h		\
	docindex.c		\
	docindex.h		\
	gimpprogress.c		\
	gimpprogress.h		\
	nav_window.c		\
	nav_window.h		\
	ops_buttons.c		\
	ops_buttons.h		\
	qmask.c			\
	qmask.h			\
	undo_history.c		\
	undo_history.h		\
##
## stuff that needs to be split up in core / ui files
##
	image_map.c		\
	image_map.h		\
	user_install.c		\
	user_install.h		\
##
## stuff for core/
##
	floating_sel.c		\
	floating_sel.h		\
##
## config stuff
##
	appenv.h		\
	general.c		\
	general.h		\
	gimprc.c		\
	gimprc.h		\
##
## file load/save stuff
##
	file-open.c		\
	file-open.h		\
	file-save.c		\
	file-save.h		\
	file-utils.h		\
	file-utils.c		\
##
## stuff for plug-in/
##
	plug_in.c		\
	plug_in.h		\
##
## stuff which is about to be replaced by new subsystems
##
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
	undo_types.h		\
##
## stuff
##
	app_procs.c		\
	app_procs.h		\
	main.c			\
	\
	batch.c			\
	batch.h			\
	errors.c		\
	errors.h		\
	gimphelp.c		\
	gimphelp.h		\
	libgimp_glue.c		\
	libgimp_glue.h		\
@STRIP_END@

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

gimp_LDFLAGS = -export-dynamic -export-symbols $(srcdir)/gimp.sym

AM_CPPFLAGS = @STRIP_BEGIN@ \
	-DLIBDIR=\""$(gimpplugindir)"\"	\
	-DLOCALEDIR=\""$(localedir)"\"	\
	-DG_LOG_DOMAIN=\"Gimp\"		\
	@GIMP_THREAD_FLAGS@ 		\
	@GIMP_MP_FLAGS@			\
@STRIP_END@

INCLUDES = @STRIP_BEGIN@ \
	-I$(top_srcdir)			\
	$(GTK_CFLAGS)			\
	-I$(includedir)			\
@STRIP_END@

gimp_LDADD = @STRIP_BEGIN@ \
	gui/libappgui.a							\
	tools/libapptools.a						\
	display/libappdisplay.a						\
	widgets/libappwidgets.a						\
	plug-in/libappplug-in.a						\
	core/libappcore.a						\
	pdb/libapppdb.a							\
	xcf/libappxcf.a							\
	paint-funcs/libapppaint-funcs.a					\
	base/libappbase.a						\
	$(top_builddir)/libgimpcolor/libgimpcolor-$(LT_RELEASE).la	\
	$(top_builddir)/libgimpmath/libgimpmath-$(LT_RELEASE).la	\
	$(top_builddir)/libgimpbase/libgimpbase-$(LT_RELEASE).la	\
	$(top_builddir)/libgimpwidgets/libgimpwidgets-$(LT_RELEASE).la	\
	$(GTK_LIBS)							\
	$(PANGOFT2_LIBS)						\
	$(GIMP_THREAD_LIBS)						\
	$(GIMP_MP_LIBS)							\
	$(INTLLIBS)							\
	$(REGEXREPL)							\
@STRIP_END@

.PHONY: files

files:
	@files=`ls $(DISTFILES) 2> /dev/null`; for p in $$files; do \
	  echo $$p; \
	done

makefile.mingw: $(top_builddir)/config.status $(top_srcdir)/app/makefile.mingw.in
	cd $(top_builddir) && CONFIG_FILES=app/$@ CONFIG_HEADERS= $(SHELL) ./config.status
