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
FIXXREF_OPTIONS =

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
	gimpimage-convert-data.h	\
	gimpimage-convert-fsdither.h	\
	gimphelp-ids.h			\
	gimprc-blurbs.h			\
	paint-funcs-generic.h

# Images to copy into HTML directory
HTML_IMAGES=

# Extra SGML files that are included by $(DOC_MAIN_SGML_FILE)
content_files =

# Other files to distribute
extra_files =

# CFLAGS and LDFLAGS for compiling scan program. Only needed
# if $(DOC_MODULE).types is non-empty.

GTKDOC_CFLAGS = \
	-g		\
	-I$(top_srcdir)	\
	$(GTK_CFLAGS)	\
	-I$(includedir)

unused_symbols = \
	-u $(SYMPREFIX)gimp_container_filter	\
	-u $(SYMPREFIX)gimp_xml_parser_new	\
	-u $(SYMPREFIX)gimp_drawable_stroke_vectors

GTKDOC_LIBS = \
	$(unused_symbols)						\
        $(top_builddir)/app/app_procs.o					\
        $(top_builddir)/app/batch.o					\
        $(top_builddir)/app/errors.o					\
        $(top_builddir)/app/units.o					\
        $(top_builddir)/app/config/libappconfig.a			\
        $(top_builddir)/app/gui/libappgui.a				\
        $(top_builddir)/app/display/libappdisplay.a			\
        $(top_builddir)/app/tools/libapptools.a				\
        $(top_builddir)/app/widgets/libappwidgets.a			\
        $(top_builddir)/app/core/libappcore.a				\
        $(top_builddir)/app/pdb/libapppdb.a				\
        $(top_builddir)/app/paint/libapppaint.a				\
        $(top_builddir)/app/xcf/libappxcf.a				\
        $(top_builddir)/app/text/libapptext.a				\
        $(top_builddir)/app/vectors/libappvectors.a			\
        $(top_builddir)/app/file/libappfile.a				\
        $(top_builddir)/app/plug-in/libappplug-in.a			\
        $(top_builddir)/app/paint-funcs/libapppaint-funcs.a		\
        $(top_builddir)/app/composite/libappcomposite.a			\
        $(top_builddir)/app/base/libappbase.a				\
        $(top_builddir)/libgimpcolor/libgimpcolor-$(LT_RELEASE).la      \
        $(top_builddir)/libgimpmath/libgimpmath-$(LT_RELEASE).la        \
        $(top_builddir)/libgimpbase/libgimpbase-$(LT_RELEASE).la        \
        $(top_builddir)/libgimpmodule/libgimpmodule-$(LT_RELEASE).la    \
        $(top_builddir)/libgimpthumb/libgimpthumb-$(LT_RELEASE).la    \
        $(top_builddir)/libgimpwidgets/libgimpwidgets-$(LT_RELEASE).la  \
        $(GTK_LIBS)                                                     \
        $(LIBART_LIBS)                                                  \
        $(PANGOFT2_LIBS)                                                \
        $(FONTCONFIG_LIBS)                                              \
        $(FREETYPE_LIBS)                                                \
        $(GIMP_THREAD_LIBS)                                             \
        $(GIMP_MP_LIBS)                                                 \
        $(INTLLIBS)                                                     \
        $(REGEXREPL)

GTKDOC_CC = $(LIBTOOL) --mode=compile $(CC)
GTKDOC_LD = $(LIBTOOL) --mode=link $(CC)


####################################
# Everything below here is generic #
####################################

# We set GPATH here; this gives us semantics for GNU make
# which are more like other make's VPATH, when it comes to
# whether a source that is a target of one rule is then
# searched for in VPATH/GPATH.
#
GPATH = $(srcdir)

TARGET_DIR=$(HTML_DIR)/$(DOC_MODULE)

EXTRA_DIST = 				\
	$(content_files)		\
	$(extra_files)			\
	$(HTML_IMAGES)			\
	$(DOC_MAIN_SGML_FILE)		\
	$(DOC_MODULE).types		\
	$(DOC_MODULE)-sections.txt

DOC_STAMPS=scan-build.stamp tmpl-build.stamp xml-build.stamp html-build.stamp \
	   $(srcdir)/tmpl.stamp $(srcdir)/sgml.stamp $(srcdir)/html.stamp

SCANOBJ_FILES = 		\
	$(DOC_MODULE).args 	\
	$(DOC_MODULE).hierarchy \
	$(DOC_MODULE).signals

if ENABLE_GTK_DOC
all-local: html-build.stamp

#### scan ####

scan-build.stamp: $(HFILE_GLOB)
	@echo '*** Scanning header files ***'
	if grep -l '^..*$$' $(srcdir)/$(DOC_MODULE).types > /dev/null ; then \
	    CC="$(GTKDOC_CC)" LD="$(GTKDOC_LD)" CFLAGS="$(GTKDOC_CFLAGS)" LDFLAGS="$(GTKDOC_LIBS)" gtkdoc-scangobj $(SCANGOBJ_OPTIONS) --module=$(DOC_MODULE) --output-dir=$(srcdir) ; \
	else \
	    cd $(srcdir) ; \
	    for i in $(SCANOBJ_FILES) ; do \
               test -f $$i || touch $$i ; \
	    done \
	fi
	cd $(srcdir) && \
	  gtkdoc-scan --module=$(DOC_MODULE) --source-dir=$(DOC_SOURCE_DIR) --ignore-headers="$(IGNORE_HFILES)" $(SCAN_OPTIONS) $(EXTRA_HFILES)
	touch scan-build.stamp

$(DOC_MODULE)-decl.txt $(SCANOBJ_FILES): scan-build.stamp
	@true

#### templates ####

tmpl-build.stamp: $(DOC_MODULE)-decl.txt $(SCANOBJ_FILES) $(DOC_MODULE)-sections.txt
	@echo '*** Rebuilding template files ***'
	cd $(srcdir) && gtkdoc-mktmpl --module=$(DOC_MODULE)
	touch tmpl-build.stamp

tmpl.stamp: tmpl-build.stamp
	@true

#### xml ####

xml-build.stamp: tmpl.stamp $(CFILE_GLOB) $(srcdir)/tmpl/*.sgml
	@echo '*** Building XML ***'
	cd $(srcdir) && \
	gtkdoc-mkdb --module=$(DOC_MODULE) --source-dir=$(DOC_SOURCE_DIR) $(MKDB_OPTIONS)
	touch xml-build.stamp

sgml.stamp: xml-build.stamp
	@true

#### html ####

html-build.stamp: sgml.stamp $(DOC_MAIN_SGML_FILE) $(content_files)
	@echo '*** Building HTML ***'
	rm -rf $(srcdir)/html 
	mkdir $(srcdir)/html
	cd $(srcdir)/html && gtkdoc-mkhtml $(DOC_MODULE) ../$(DOC_MAIN_SGML_FILE)
	test "x$(HTML_IMAGES)" = "x" || ( cd $(srcdir) && cp $(HTML_IMAGES) html )
	@echo '-- Fixing Crossreferences' 
	cd $(srcdir) && gtkdoc-fixxref --module-dir=html --html-dir=$(HTML_DIR) $(FIXXREF_OPTIONS)
	touch html-build.stamp
else
all-local:
endif

##############

clean-local:
	rm -f *~ *.bak $(SCANOBJ_FILES) *-unused.txt $(DOC_STAMPS)

maintainer-clean-local: clean
	cd $(srcdir) && rm -rf xml html $(DOC_MODULE)-decl-list.txt $(DOC_MODULE)-decl.txt

install-data-local:
	$(mkinstalldirs) $(DESTDIR)$(TARGET_DIR)
	(installfiles=`echo $(srcdir)/html/*`; \
	if test -f '$(srcdir)/html/index.sgml'; \
	then
	  for i in $$installfiles; do \
	    echo '-- Installing '$$i ; \
	    $(INSTALL_DATA) $$i $(DESTDIR)$(TARGET_DIR); \
	  done; \
	  echo '-- Installing $(srcdir)/html/index.sgml' ; \
	  $(INSTALL_DATA) $(srcdir)/html/index.sgml $(DESTDIR)$(TARGET_DIR); \
	else
	  echo '-- Nothing to install' ; \
	fi)

uninstall-local:
	(installfiles=`echo $(srcdir)/html/*`; \
	for i in $$installfiles; do \
	  rm -f $(DESTDIR)$(TARGET_DIR)/`basename $$i`; \
	done; \
	rm -f $(DESTDIR)$(TARGET_DIR)/index.sgml)


#
# Require gtk-doc when making dist
#
if ENABLE_GTK_DOC
dist-check-gtkdoc:
else
dist-check-gtkdoc:
	@echo "*** gtk-doc must be installed and enabled in order to make dist"
	@false
endif

dist-hook: dist-check-gtkdoc dist-hook-local
	mkdir $(distdir)/tmpl
	mkdir $(distdir)/xml
	mkdir $(distdir)/html
	-cp $(srcdir)/tmpl/*.sgml $(distdir)/tmpl
	-cp $(srcdir)/xml/*.xml $(distdir)/xml
	-cp $(srcdir)/html/* $(distdir)/html

.PHONY : dist-hook-local
