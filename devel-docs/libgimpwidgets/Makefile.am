## Process this file with automake to produce Makefile.in

# The name of the module.
DOC_MODULE = libgimpwidgets

# The top-level SGML file.
DOC_MAIN_SGML_FILE = $(DOC_MODULE)-docs.sgml

# The directory containing the source code.
DOC_SOURCE_DIR = $(top_srcdir)/$(DOC_MODULE)

# Extra options to pass to gtkdoc-scangobj
SCANGOBJ_OPTIONS =

# Extra options to supply to gtkdoc-scan
SCAN_OPTIONS =

# Extra options to supply to gtkdoc-mkdb
MKDB_OPTIONS =

# Extra options to supply to gtkdoc-fixref
FIXXREF_OPTIONS =

# Used for dependencies
HFILE_GLOB = $(DOC_SOURCE_DIR)/*.h
CFILE_GLOB = $(DOC_SOURCE_DIR)/*.c

# Header files to ignore when scanning
IGNORE_HFILES =


# Images to copy into HTML directory
IMAGES_DIR = $(top_srcdir)/themes/Default/images
HTML_IMAGES = \
	$(IMAGES_DIR)/stock-anchor-16.png			\
	$(IMAGES_DIR)/stock-char-picker-22.png			\
	$(IMAGES_DIR)/stock-color-picker-black-18.png		\
	$(IMAGES_DIR)/stock-color-picker-gray-18.png		\
	$(IMAGES_DIR)/stock-color-picker-white-18.png		\
	$(IMAGES_DIR)/stock-color-triangle-16.png		\
	$(IMAGES_DIR)/stock-convert-grayscale-16.png		\
	$(IMAGES_DIR)/stock-convert-indexed-16.png		\
	$(IMAGES_DIR)/stock-convert-rgb-16.png			\
	$(IMAGES_DIR)/stock-default-colors-12.png		\
	$(IMAGES_DIR)/stock-duplicate-16.png			\
	$(IMAGES_DIR)/stock-edit-16.png				\
	$(IMAGES_DIR)/stock-eye-20.png				\
	$(IMAGES_DIR)/stock-selection-grow-16.png		\
	$(IMAGES_DIR)/stock-hchain-24.png			\
	$(IMAGES_DIR)/stock-hchain-broken-24.png		\
	$(IMAGES_DIR)/stock-info-64.png				\
	$(IMAGES_DIR)/stock-invert-16.png			\
	$(IMAGES_DIR)/stock-layer-to-imagesize-16.png		\
	$(IMAGES_DIR)/stock-letter-spacing-22.png		\
	$(IMAGES_DIR)/stock-line-spacing-22.png			\
	$(IMAGES_DIR)/stock-linked-20.png			\
	$(IMAGES_DIR)/stock-merge-down-16.png			\
	$(IMAGES_DIR)/stock-navigation-16.png			\
	$(IMAGES_DIR)/stock-paste-as-new-16.png			\
	$(IMAGES_DIR)/stock-paste-into-16.png			\
	$(IMAGES_DIR)/stock-path-stroke-16.png			\
	$(IMAGES_DIR)/stock-plugin-16.png			\
	$(IMAGES_DIR)/stock-qmask-off-16.png			\
	$(IMAGES_DIR)/stock-qmask-on-16.png			\
	$(IMAGES_DIR)/stock-reset-16.png			\
	$(IMAGES_DIR)/stock-reshow-filter-16.png		\
	$(IMAGES_DIR)/stock-resize-16.png			\
	$(IMAGES_DIR)/stock-rotate-180-16.png			\
	$(IMAGES_DIR)/stock-rotate-270-16.png			\
	$(IMAGES_DIR)/stock-rotate-90-16.png			\
	$(IMAGES_DIR)/stock-scale-16.png			\
	$(IMAGES_DIR)/stock-selection-add-16.png		\
	$(IMAGES_DIR)/stock-selection-all-16.png		\
	$(IMAGES_DIR)/stock-selection-intersect-16.png		\
	$(IMAGES_DIR)/stock-selection-none-16.png		\
	$(IMAGES_DIR)/stock-selection-replace-16.png		\
	$(IMAGES_DIR)/stock-selection-shrink-16.png		\
	$(IMAGES_DIR)/stock-selection-stroke-16.png		\
	$(IMAGES_DIR)/stock-selection-subtract-16.png		\
	$(IMAGES_DIR)/stock-selection-to-channel-16.png		\
	$(IMAGES_DIR)/stock-selection-to-path-16.png		\
	$(IMAGES_DIR)/stock-swap-colors-12.png			\
	$(IMAGES_DIR)/stock-tool-options-16.png			\
	$(IMAGES_DIR)/stock-vchain-24.png			\
	$(IMAGES_DIR)/stock-vchain-broken-24.png		\
	$(IMAGES_DIR)/stock-warning-64.png			\
	$(IMAGES_DIR)/stock-wilber-64.png			\
	$(IMAGES_DIR)/stock-wilber-eek-64.png			\
	$(IMAGES_DIR)/tools/stock-tool-airbrush-22.png		\
	$(IMAGES_DIR)/tools/stock-tool-bezier-select-22.png	\
	$(IMAGES_DIR)/tools/stock-tool-blend-22.png		\
	$(IMAGES_DIR)/tools/stock-tool-blur-22.png		\
	$(IMAGES_DIR)/tools/stock-tool-brightness-contrast-22.png\
	$(IMAGES_DIR)/tools/stock-tool-bucket-fill-22.png	\
	$(IMAGES_DIR)/tools/stock-tool-by-color-select-22.png	\
	$(IMAGES_DIR)/tools/stock-tool-clone-22.png		\
	$(IMAGES_DIR)/tools/stock-tool-color-balance-22.png	\
	$(IMAGES_DIR)/tools/stock-tool-color-picker-22.png	\
	$(IMAGES_DIR)/tools/stock-tool-crop-22.png		\
	$(IMAGES_DIR)/tools/stock-tool-curves-22.png		\
	$(IMAGES_DIR)/tools/stock-tool-dodge-22.png		\
	$(IMAGES_DIR)/tools/stock-tool-ellipse-select-22.png	\
	$(IMAGES_DIR)/tools/stock-tool-eraser-22.png		\
	$(IMAGES_DIR)/tools/stock-tool-flip-22.png		\
	$(IMAGES_DIR)/tools/stock-tool-free-select-22.png	\
	$(IMAGES_DIR)/tools/stock-tool-fuzzy-select-22.png	\
	$(IMAGES_DIR)/tools/stock-tool-histogram-22.png		\
	$(IMAGES_DIR)/tools/stock-tool-hue-saturation-22.png	\
	$(IMAGES_DIR)/tools/stock-tool-ink-22.png		\
	$(IMAGES_DIR)/tools/stock-tool-iscissors-22.png		\
	$(IMAGES_DIR)/tools/stock-tool-levels-22.png		\
	$(IMAGES_DIR)/tools/stock-tool-measure-22.png		\
	$(IMAGES_DIR)/tools/stock-tool-move-22.png		\
	$(IMAGES_DIR)/tools/stock-tool-paintbrush-22.png	\
	$(IMAGES_DIR)/tools/stock-tool-path-22.png		\
	$(IMAGES_DIR)/tools/stock-tool-pencil-22.png		\
	$(IMAGES_DIR)/tools/stock-tool-perspective-22.png	\
	$(IMAGES_DIR)/tools/stock-tool-posterize-22.png		\
	$(IMAGES_DIR)/tools/stock-tool-rect-select-22.png	\
	$(IMAGES_DIR)/tools/stock-tool-rotate-22.png		\
	$(IMAGES_DIR)/tools/stock-tool-scale-22.png		\
	$(IMAGES_DIR)/tools/stock-tool-shear-22.png		\
	$(IMAGES_DIR)/tools/stock-tool-smudge-22.png		\
	$(IMAGES_DIR)/tools/stock-tool-text-22.png		\
	$(IMAGES_DIR)/tools/stock-tool-threshold-22.png		\
	$(IMAGES_DIR)/tools/stock-tool-zoom-22.png

# Extra SGML files that are included by $(DOC_MAIN_SGML_FILE)
content_files =

# Other files to distribute
extra_files = libgimpwidgets-include.c

# CFLAGS and LDFLAGS for compiling scan program. Only needed
# if $(DOC_MODULE).types is non-empty.
GTKDOC_CFLAGS = \
	@CFLAGS@		\
	-I$(top_srcdir) 	\
	-I$(top_builddir) 	\
	@GTK_CFLAGS@

GTKDOC_LIBS = \
        $(top_builddir)/libgimpbase/libgimpbase-$(LT_RELEASE).la        \
        $(top_builddir)/libgimpcolor/libgimpcolor-$(LT_RELEASE).la      \
        $(top_builddir)/libgimpmath/libgimpmath-$(LT_RELEASE).la        \
        $(top_builddir)/libgimpwidgets/libgimpwidgets-$(LT_RELEASE).la  \
        $(top_builddir)/libgimp/libgimp-$(LT_RELEASE).la		\
        $(top_builddir)/libgimp/libgimpui-$(LT_RELEASE).la		\
	@GTK_LIBS@

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

DOC_STAMPS=scan-build.stamp tmpl-build.stamp sgml-build.stamp html-build.stamp \
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

#### sgml ####

sgml-build.stamp: tmpl.stamp $(CFILE_GLOB) $(srcdir)/tmpl/*.sgml
	@echo '*** Building SGML ***'
	cd $(srcdir) && \
	gtkdoc-mkdb --module=$(DOC_MODULE) --source-dir=$(DOC_SOURCE_DIR) $(MKDB_OPTIONS)
	touch sgml-build.stamp

sgml.stamp: sgml-build.stamp
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
	cd $(srcdir) && rm -rf sgml html $(DOC_MODULE)-decl-list.txt $(DOC_MODULE)-decl.txt

install-data-local:
	$(mkinstalldirs) $(DESTDIR)$(TARGET_DIR)
	(installfiles=`echo $(srcdir)/html/*`; \
	if test "$$installfiles" = '$(srcdir)/html/*'; \
	then echo '-- Nothing to install' ; \
	else \
	  for i in $$installfiles; do \
	    echo '-- Installing '$$i ; \
	    $(INSTALL_DATA) $$i $(DESTDIR)$(TARGET_DIR); \
	  done; \
	  echo '-- Installing $(srcdir)/html/index.sgml' ; \
	  $(INSTALL_DATA) $(srcdir)/html/index.sgml $(DESTDIR)$(TARGET_DIR); \
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
	mkdir $(distdir)/sgml
	mkdir $(distdir)/html
	-cp $(srcdir)/tmpl/*.sgml $(distdir)/tmpl
	-cp $(srcdir)/sgml/*.sgml $(distdir)/sgml
	-cp $(srcdir)/html/* $(distdir)/html

.PHONY : dist-hook-local
