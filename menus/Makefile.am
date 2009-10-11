## Makefile.am for gimp/menus

menudatadir = $(gimpdatadir)/menus

menudata_in_files = \
	dockable-menu.xml.in	\
	image-menu.xml.in

menudata_built_files = $(menudata_in_files:.xml.in=.xml)

menudata_DATA = \
	$(menudata_built_files)		\
	brush-editor-menu.xml		\
	brushes-menu.xml		\
	buffers-menu.xml		\
	channels-menu.xml		\
	colormap-menu.xml		\
	cursor-info-menu.xml		\
	documents-menu.xml		\
	dynamics-editor-menu.xml	\
	dynamics-menu.xml		\
	error-console-menu.xml		\
	fonts-menu.xml			\
	gradient-editor-menu.xml	\
	gradients-menu.xml		\
	images-menu.xml			\
	layers-menu.xml			\
	palette-editor-menu.xml		\
	palettes-menu.xml		\
	patterns-menu.xml		\
	quick-mask-menu.xml		\
	sample-points-menu.xml		\
	selection-menu.xml		\
	templates-menu.xml		\
	text-editor-toolbar.xml		\
	text-tool-menu.xml		\
	tool-options-menu.xml		\
	undo-menu.xml			\
	vectors-menu.xml

EXTRA_DIST = \
	$(menudata_DATA)		\
	$(menudata_in_files)		\
	dialogs-menuitems.xml		\
	gtkuimanager.dtd		\
	menus.xsl			\
	makefile.msc

MAINTAINERCLEANFILES = $(menudata_built_files)


if GIMP_UNSTABLE
DEBUG_MENU_PARAMS = --stringparam debug-menu yes
endif

%.xml: %.xml.in $(top_srcdir)/configure.ac $(srcdir)/menus.xsl dialogs-menuitems.xml
if HAVE_XSLTPROC
	$(XSLTPROC) --xinclude $(DEBUG_MENU_PARAMS) $(srcdir)/menus.xsl $< > $(@) || rm -f $(@)
else
	@echo "*** xsltproc is required to build the menus XML files ***"; exit 1;
endif


validate: $(menudata_DATA)
if HAVE_XMLLINT
	@for menu in $^; do \
	   $(XMLLINT) --noout --path $(srcdir) --valid $$menu || \
	   ( echo "*** $$menu INVALID ***"; exit 1; ) ; \
	done
endif

dist-hook: validate
