## Process this file with automake to produce Makefile.in

# The name of the module.
DOC_MODULE = app

# The top-level SGML file.
DOC_MAIN_SGML_FILE = $(DOC_MODULE)-docs.sgml

# The directory containing the source code.
DOC_SOURCE_DIR = $(top_srcdir)/$(DOC_MODULE)

# Extra options to pass to gtkdoc-scangobj
SCANGOBJ_OPTIONS =

# Extra options to supply to gtkdoc-scan
SCAN_OPTIONS =

# Extra options to supply to gtkdoc-mkdb
MKDB_OPTIONS = --sgml-mode --output-format=xml

# Extra options to supply to gtkdoc-fixref
FIXXREF_OPTIONS = --extra-dir=../libgimpbase/html --extra-dir=../libgimpcolor/html --extra-dir=../libgimpconfig/html --extra-dir=../libgimpmath/html --extra-dir=../libgimpwidgets/html


# Used for dependencies
HFILE_GLOB = $(DOC_SOURCE_DIR)/*.h
CFILE_GLOB = $(DOC_SOURCE_DIR)/*.c

# Header files to ignore when scanning
IGNORE_HFILES = \
	gimp-composite-dispatch.h	\
	gimp-composite-regression.h	\
	gimp-composite-altivec.h	\
	gimp-composite-3dnow.h		\
	gimp-composite-mmx.h		\
	gimp-composite-sse.h		\
	gimp-composite-sse2.h		\
	gimp-composite-vis.h		\
	gimp-composite-x86.h		\
	gimp-intl.h			\
	gimpdbusservice-glue.h		\
	gimpimage-convert-data.h	\
	gimpimage-convert-fsdither.h	\
	gimphelp-ids.h			\
	gimprc-blurbs.h			\
	paint-funcs-generic.h

# Images to copy into HTML directory
HTML_IMAGES =

# Extra SGML files that are included by $(DOC_MAIN_SGML_FILE)
content_files =

# CFLAGS and LDFLAGS for compiling scan program. Only needed
# if $(DOC_MODULE).types is non-empty.

GTKDOC_CFLAGS = \
	-g		\
	-I$(top_srcdir)	\
	$(GTK_CFLAGS)	\
	-I$(includedir)	\
	-UGTK_DISABLE_SINGLE_INCLUDES

GTKDOC_LIBS = \
	-u $(SYMPREFIX)xcf_init		   				     \
	-u $(SYMPREFIX)internal_procs_init 				     \
	-u $(SYMPREFIX)gimp_coords_mix	   				     \
	-u $(SYMPREFIX)gimp_plug_in_manager_restore			     \
	$(top_builddir)/app/app.o					     \
        $(top_builddir)/app/batch.o					     \
        $(top_builddir)/app/errors.o					     \
        $(top_builddir)/app/sanity.o					     \
        $(top_builddir)/app/units.o					     \
	$(top_builddir)/app/gimp-debug.o				     \
	$(top_builddir)/app/gimp-log.o					     \
        $(top_builddir)/app/gui/libappgui.a				     \
	$(top_builddir)/app/actions/libappactions.a			     \
        $(top_builddir)/app/dialogs/libappdialogs.a			     \
        $(top_builddir)/app/menus/libappmenus.a				     \
        $(top_builddir)/app/tools/libapptools.a				     \
        $(top_builddir)/app/display/libappdisplay.a			     \
        $(top_builddir)/app/widgets/libappwidgets.a			     \
	$(top_builddir)/app/pdb/libappinternal-procs.a			     \
        $(top_builddir)/app/xcf/libappxcf.a				     \
        $(top_builddir)/app/file/libappfile.a				     \
        $(top_builddir)/app/plug-in/libappplug-in.a			     \
        $(top_builddir)/app/pdb/libapppdb.a				     \
        $(top_builddir)/app/core/libappcore.a				     \
        $(top_builddir)/app/vectors/libappvectors.a			     \
        $(top_builddir)/app/paint/libapppaint.a				     \
        $(top_builddir)/app/text/libapptext.a				     \
        $(top_builddir)/app/config/libappconfig.a			     \
        $(top_builddir)/app/gegl/libappgegl.a				     \
        $(top_builddir)/app/paint-funcs/libapppaint-funcs.a		     \
        $(top_builddir)/app/composite/libappcomposite.a			     \
        $(top_builddir)/app/base/libappbase.a				     \
        $(top_builddir)/libgimpwidgets/libgimpwidgets-$(GIMP_API_VERSION).la \
        $(top_builddir)/libgimpmodule/libgimpmodule-$(GIMP_API_VERSION).la   \
        $(top_builddir)/libgimpcolor/libgimpcolor-$(GIMP_API_VERSION).la     \
        $(top_builddir)/libgimpthumb/libgimpthumb-$(GIMP_API_VERSION).la     \
        $(top_builddir)/libgimpmath/libgimpmath-$(GIMP_API_VERSION).la       \
        $(top_builddir)/libgimpconfig/libgimpconfig-$(GIMP_API_VERSION).la   \
        $(top_builddir)/libgimpbase/libgimpbase-$(GIMP_API_VERSION).la       \
        $(GTK_LIBS)                                                          \
        $(LIBART_LIBS)                                                       \
        $(PANGOFT2_LIBS)                                                     \
        $(FONTCONFIG_LIBS)                                                   \
        $(FREETYPE_LIBS)                                                     \
	$(DBUS_GLIB_LIBS)						     \
	$(GEGL_LIBS)							     \
	$(RT_LIBS)							     \
        $(INTLLIBS)


if ENABLE_GTK_DOC_APP

include $(top_srcdir)/gtk-doc.make

else

EXTRA_DIST = \
	$(DOC_MODULE)-docs.sgml		\
	$(DOC_MODULE)-overrides.txt	\
	$(DOC_MODULE)-sections.txt

endif

# Other files to distribute
EXTRA_DIST += \
	$(DOC_MODULE).types	\
	version.in
