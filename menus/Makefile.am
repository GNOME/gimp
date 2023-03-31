## Makefile.am for gimp/menus

menudatadir = $(gimpdatadir)/menus

menudata_in_files = \
	dockable-menu.ui.in	\
	image-menu.ui.in

menudata_built_files = $(menudata_in_files:.ui.in=.ui)

menudata_DATA = \
	$(menudata_built_files)		\
	brush-editor-menu.ui		\
	brushes-menu.ui		\
	buffers-menu.ui		\
	channels-menu.ui		\
	colormap-menu.ui		\
	cursor-info-menu.ui		\
	dashboard-menu.ui		\
	documents-menu.ui		\
	dynamics-editor-menu.ui	\
	dynamics-menu.ui		\
	error-console-menu.ui		\
	fonts-menu.ui			\
	gradient-editor-menu.ui	\
	gradients-menu.ui		\
	images-menu.ui			\
	layers-menu.ui			\
	mypaint-brushes-menu.ui	\
	palette-editor-menu.ui		\
	palettes-menu.ui		\
	patterns-menu.ui		\
	tool-preset-editor-menu.ui	\
	tool-presets-menu.ui		\
	quick-mask-menu.ui		\
	sample-points-menu.ui		\
	selection-menu.ui		\
	templates-menu.ui		\
	text-editor-toolbar.ui		\
	text-tool-menu.ui		\
	tool-options-menu.ui		\
	undo-menu.ui			\
	vector-toolpath-menu.ui	\
	vectors-menu.ui

EXTRA_DIST = \
	$(menudata_DATA)		\
	$(menudata_in_files)		\
	dialogs-menuitems.ui		\
	gtkuimanager.dtd		\
	menus.xsl

MAINTAINERCLEANFILES = $(menudata_built_files)


if GIMP_UNSTABLE
UNSTABLE_MENU_PARAMS = --stringparam unstable-menus yes
endif

SUFFIXES = .ui.in .ui

# Adding a `realpath` because xsltproc adds an xml:base attribute when the path
# is absolute. This breaks our parsing at runtime.
.ui.in.ui:
if HAVE_XSLTPROC
	$(XSLTPROC) --xinclude $(UNSTABLE_MENU_PARAMS) $(srcdir)/menus.xsl "`realpath --relative-to=. $<`" > $(@) || rm -f $(@)
else
	@echo "*** xsltproc is required to build the menus XML files ***"; exit 1;
endif

# Specifying dependencies in the implicit rule above does not work, so
# specify them here instead
dockable-menu.ui image-menu.ui: $(top_srcdir)/configure.ac $(srcdir)/menus.xsl dialogs-menuitems.ui

validate: $(menudata_DATA)
if HAVE_XMLLINT
	@for menu in $^; do \
	   $(XMLLINT) --noout --path $(srcdir) --valid $$menu || \
	   ( echo "*** $$menu INVALID ***"; exit 1; ) ; \
	done
endif

dist-hook: validate
