## Process this file with automake to produce Makefile.in

# The name of the module.
DOC_MODULE = libgimpwidgets3

# The top-level SGML file.
DOC_MAIN_SGML_FILE = $(DOC_MODULE)-docs.sgml

# The directory containing the source code.
DOC_SOURCE_DIR = $(top_srcdir)/libgimpwidgets

# Extra options to pass to gtkdoc-scangobj
# make sure babl is initialized, otherwise we segfault.
SCANGOBJ_OPTIONS = --type-init-func="do { void babl_init (void); babl_init (); } while (0)"

# Extra options to supply to gtkdoc-scan
SCAN_OPTIONS = --deprecated-guards="GIMP_DISABLE_DEPRECATED"

# Extra options to supply to gtkdoc-mkdb
MKDB_OPTIONS = --sgml-mode --output-format=xml --name-space=gimp

# Extra options to supply to gtkdoc-fixref
FIXXREF_OPTIONS = \
	--extra-dir=../libgimpbase/html		\
	--extra-dir=../libgimpcolor/html	\
	--extra-dir=../libgimpconfig/html	\
	--extra-dir=../libgimpmath/html		\
	--extra-dir=../libgimp/html

# Used for dependencies
HFILE_GLOB = $(DOC_SOURCE_DIR)/*.h
CFILE_GLOB = $(DOC_SOURCE_DIR)/*.c

# Header files to ignore when scanning
IGNORE_HFILES = \
	gimp-icon-pixbufs.h		\
	gimp-color-picker-cursors.h	\
	gimpcolorprofilestore-private.h	\
	gimpeevl.h			\
	gimpwidgets-private.h

# Images to copy into HTML directory
HTML_IMAGES = \
	$(top_srcdir)/icons/Color/12/gimp-*.png		\
	$(top_srcdir)/icons/Color/16/gimp-*.png		\
	$(top_srcdir)/icons/Color/18/gimp-*.png		\
	$(top_srcdir)/icons/Color/20/gimp-*.png		\
	$(top_srcdir)/icons/Color/22/gimp-*.png		\
	$(top_srcdir)/icons/Color/24/gimp-*.png		\
	$(top_srcdir)/icons/Color/32/gimp-*.png		\
	$(top_srcdir)/icons/Color/48/gimp-*.png		\
	$(top_srcdir)/icons/Color/64/gimp-*.png		\
	$(top_srcdir)/icons/Color/128/gimp-*.png	\
	$(top_srcdir)/icons/Color/192/gimp-*.png	\
	$(top_srcdir)/icons/Color/256/gimp-*.png	\
	$(srcdir)/images/gimp-widget-*.png

# Extra SGML files that are included by $(DOC_MAIN_SGML_FILE)
content_files = visual-index.xml

# Other files to distribute
extra_files =

# CFLAGS and LDFLAGS for compiling scan program. Only needed
# if $(DOC_MODULE).types is non-empty.
GTKDOC_CFLAGS = \
	@CFLAGS@					\
	-I$(top_srcdir) 				\
	-I$(top_builddir) 				\
	@GEGL_CFLAGS@					\
	@GTK_CFLAGS@					\
	-DGIMP_ENABLE_CONTROLLER_UNDER_CONSTRUCTION	\
	-UGIMP_DISABLE_DEPRECATED

GTKDOC_LIBS = \
	$(top_builddir)/libgimpbase/libgimpbase-$(GIMP_API_VERSION).la		\
	$(top_builddir)/libgimpcolor/libgimpcolor-$(GIMP_API_VERSION).la	\
	$(top_builddir)/libgimpmath/libgimpmath-$(GIMP_API_VERSION).la		\
	$(top_builddir)/libgimpconfig/libgimpconfig-$(GIMP_API_VERSION).la	\
	$(top_builddir)/libgimpwidgets/libgimpwidgets-$(GIMP_API_VERSION).la	\
	@GEGL_LIBS@								\
	@GTK_LIBS@


include $(top_srcdir)/gtk-doc.make
