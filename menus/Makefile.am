## Makefile.am for gimp/menus

menudatadir = $(gimpdatadir)/menus

menudata_DATA = \
	brushes-menu.xml		\
	buffers-menu.xml		\
	channels-menu.xml		\
	colormap-editor-menu.xml	\
	dockable-menu.xml		\
	documents-menu.xml		\
	error-console-menu.xml		\
	file-open-menu.xml		\
	file-save-menu.xml		\
	fonts-menu.xml			\
	gradient-editor-menu.xml	\
	gradients-menu.xml		\
	image-menu.xml			\
	images-menu.xml			\
	layers-menu.xml			\
	palette-editor-menu.xml		\
	palettes-menu.xml		\
	patterns-menu.xml		\
	qmask-menu.xml			\
	templates-menu.xml		\
	tool-options-menu.xml		\
	toolbox-menu.xml		\
	vectors-menu.xml

EXTRA_DIST = \
	$(menudata_DATA)		\
	gtkuimanager.dtd


validate: $(menudata_DATA)
	@( xmllint=`which xmllint`; \
	  if test x$$xmllint != x && test -x $$xmllint; then \
	    for menu in $(menudata_DATA); do \
	      $$xmllint --noout --valid $$menu || \
	      ( echo "* $$menu INVALID *"; exit 1; ) ; \
	    done; \
	  else \
	    echo "Can't find xmllint to validate the menu files; proceed with fingers crossed..."; \
	  fi )

dist-hook: validate
