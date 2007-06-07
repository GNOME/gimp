## Makefile.am for gimp/menus

menudatadir = $(gimpdatadir)/menus

menudata_in_files = \
	dockable-menu.xml.in	\
	image-menu.xml.in	\
	toolbox-menu.xml.in

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
	tool-options-menu.xml		\
	tools-menu.xml			\
	undo-menu.xml			\
	vectors-menu.xml

EXTRA_DIST = \
	$(menudata_DATA)		\
	$(menudata_in_files)		\
	dialogs-menuitems.xml		\
	gtkuimanager.dtd		\
	menus.xsl			\
	makefile.msc

MAINTAINERCLEAN_FILES = $(menudata_built_files)


if GIMP_UNSTABLE
XSLTPARAMS = --stringparam debug-menu yes
endif

%.xml: %.xml.in $(srcdir)/menus.xsl dialogs-menuitems.xml
if HAVE_XSLTPROC
	$(XSLTPROC) --xinclude $(XSLTPARAMS) $(srcdir)/menus.xsl $< > $(@) || rm -f $(@)
else
	@echo "*** xsltproc is required to build the menus XML files ***"; exit 1;
endif


validate: $(menudata_DATA)
if HAVE_XMLLINT
	@cd $(srcdir); \
	for menu in $(menudata_DATA); do \
	   $(XMLLINT) --noout --valid $$menu || \
	   ( echo "*** $$menu INVALID ***"; exit 1; ) ; \
	done
endif

dist-hook: validate
