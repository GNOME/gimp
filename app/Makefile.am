## Process this file with automake to produce Makefile.in

SUBDIRS = paint-funcs base core pdb tools widgets gui

scriptdata =

if HAVE_GLIBC_REGEX
regex_sources =
else
regex_sources = \
	regexrepl.c 		\
	regexrepl.h
endif

if HAVE_ASM_MMX
mmx_sources = \
	detect_mmx.S
else
mmx_sources =
endif

bin_PROGRAMS = gimp

gimp_SOURCES = @STRIP_BEGIN@ \
##
## gui stuff that needs to go to libgimp/, gui/ or /dev/null
##
	colormaps.c		\
	colormaps.h		\
	devices.c		\
	devices.h		\
	dialog_handler.c	\
	dialog_handler.h	\
	docindex.c		\
	docindex.h		\
	errorconsole.c		\
	errorconsole.h		\
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
## display stuff
##
	disp_callbacks.c	\
	disp_callbacks.h	\
	gdisplay.c		\
	gdisplay.h		\
	gdisplay_ops.c		\
	gdisplay_ops.h		\
	gximage.c		\
	gximage.h		\
	image_render.c		\
	image_render.h		\
	interface.c		\
	interface.h		\
	scale.c			\
	scale.h			\
	scroll.c		\
	scroll.h		\
	selection.c		\
	selection.h		\
	marching_ants.h		\
##
## stuff that needs to be split up in core / ui files
##
	global_edit.c		\
	global_edit.h		\
	image_map.c		\
	image_map.h		\
	module_db.c		\
	module_db.h		\
	user_install.c		\
	user_install.h		\
##
## stuff for core/
##
	datafiles.c		\
	datafiles.h		\
	floating_sel.c		\
	floating_sel.h		\
	parasitelist.c		\
	parasitelist.h		\
##
## stuff that needs to be sorted into yet undecided subsystems, split up,
## removed or otherwise tortured
##
	app_procs.c		\
	app_procs.h		\
	appenums.h		\
	appenv.h		\
	apptypes.h		\
	batch.c			\
	batch.h			\
	color_transfer.c	\
	color_transfer.h	\
	context_manager.c	\
	context_manager.h	\
	drawable.c		\
	drawable.h		\
	errors.c		\
	errors.h		\
	file-open.c		\
	file-open.h		\
	file-save.c		\
	file-save.h		\
	file-utils.h		\
	file-utils.c		\
	general.c		\
	general.h		\
	gimage.c		\
	gimage.h		\
	gimphelp.c		\
	gimphelp.h		\
	gimpparasite.c		\
	gimpparasite.h		\
	gimprc.c		\
	gimprc.h		\
	gimpunit.c		\
	image_new.c		\
	image_new.h		\
	libgimp_glue.c		\
	libgimp_glue.h		\
	main.c			\
	path.c			\
	path.h			\
	pathP.h			\
	path_transform.h	\
	path_curves.h		\
	path_curves.c		\
	path_bezier.h		\
	path_bezier.c		\
	plug_in.c		\
	plug_in.h		\
	undo.c			\
	undo.h			\
	undo_types.h		\
	unitrc.h		\
	xcf.c			\
	xcf.h			\
	$(regex_sources)	\
	$(mmx_sources)		\
@STRIP_END@

EXTRA_DIST = \
	makefile.mingw			\
	makefile.mingw.in		\
	makefile.msc			\
	gimp.rc				\
	gimp.sym			\
	wilber.ico			\
	arch/i386/mmx/detect_mmx.S	\
	arch/i386/mmx/paint_funcs_mmx.S	\
	detect_mmx.S			\
	regexrepl.c 			\
	regexrepl.h

gimp_LDFLAGS = -export-dynamic -export-symbols $(srcdir)/gimp.sym

AM_CPPFLAGS = @STRIP_BEGIN@ \
	-DLIBDIR=\""$(gimpplugindir)"\"	\
	-DLOCALEDIR=\""$(localedir)"\"	\
	-DREGEX_MALLOC			\
	-DG_LOG_DOMAIN=\"Gimp\"		\
	@GIMP_THREAD_FLAGS@ 		\
	@GIMP_MP_FLAGS@			\
@STRIP_END@

INCLUDES = @STRIP_BEGIN@ \
	-I$(top_srcdir)			\
	-I$(top_srcdir)/intl		\
	$(GTK_CFLAGS)			\
	-I$(includedir)			\
@STRIP_END@

gimp_LDADD = @STRIP_BEGIN@ \
	paint-funcs/libapppaint-funcs.a					\
	pdb/libapppdb.a							\
	tools/libapptools.a						\
	gui/libappgui.a							\
	widgets/libappwidgets.a						\
	core/libappcore.a						\
	base/libappbase.a						\
	$(top_builddir)/libgimpcolor/libgimpcolor-$(LT_RELEASE).la	\
	$(top_builddir)/libgimpmath/libgimpmath-$(LT_RELEASE).la	\
	$(top_builddir)/libgimpbase/libgimpbase-$(LT_RELEASE).la	\
	$(top_builddir)/libgimpwidgets/libgimpwidgets-$(LT_RELEASE).la	\
	$(GTK_LIBS)							\
	$(GIMP_THREAD_LIBS)						\
	$(GIMP_MP_LIBS)							\
	$(INTLLIBS)							\
@STRIP_END@

.PHONY: files

files:
	@files=`ls $(DISTFILES) 2> /dev/null`; for p in $$files; do \
	  echo $$p; \
	done

makefile.mingw: $(top_builddir)/config.status $(top_srcdir)/app/makefile.mingw.in
	cd $(top_builddir) && CONFIG_FILES=app/$@ CONFIG_HEADERS= $(SHELL) ./config.status
