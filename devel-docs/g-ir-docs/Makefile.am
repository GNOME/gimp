if ENABLE_G_IR_DOC

Gimp-pythondocs: $(top_builddir)/libgimp/Gimp-@GIMP_API_VERSION@.gir
	$(GIR_DOC_TOOL) -I "$(prefix)/share/gir-1.0/"			\
		-I "$(top_builddir)/libgimp" --language=Python		\
		-o "$(builddir)/pages/Python/Gimp-$(GIMP_API_VERSION)"	\
		$^
	$(YELP_BUILD) cache								\
		-o "$(builddir)/pages/Python/Gimp-$(GIMP_API_VERSION)/index.cache"	\
		"$(builddir)/pages/Python/Gimp-$(GIMP_API_VERSION)/"
	mkdir -p "$(builddir)/html/Python/Gimp-$(GIMP_API_VERSION)"
	$(YELP_BUILD) html						\
		-o "$(builddir)/html/Python/Gimp-$(GIMP_API_VERSION)"	\
		"$(builddir)/pages/Python/Gimp-$(GIMP_API_VERSION)/"
	echo "Python Gimp module documentation: $(builddir)/html/Python/Gimp-$(GIMP_API_VERSION)"

GimpUi-pythondocs: $(top_builddir)/libgimp/GimpUi-@GIMP_API_VERSION@.gir
	$(GIR_DOC_TOOL) -I "$(prefix)/share/gir-1.0/"				\
		-I "$(top_builddir)/libgimp" --language=Python			\
		-o "$(builddir)/pages/Python/GimpUi-$(GIMP_API_VERSION)"	\
		$^
	$(YELP_BUILD) cache								\
		-o "$(builddir)/pages/Python/GimpUi-$(GIMP_API_VERSION)/index.cache"	\
		"$(builddir)/pages/Python/GimpUi-$(GIMP_API_VERSION)/"
	mkdir -p "$(builddir)/html/Python/GimpUi-$(GIMP_API_VERSION)"
	$(YELP_BUILD) html						\
		-o "$(builddir)/html/Python/GimpUi-$(GIMP_API_VERSION)"	\
		"$(builddir)/pages/Python/GimpUi-$(GIMP_API_VERSION)/"
	echo "Python GimpUi module documentation: $(builddir)/html/Python/Gimp-$(GIMP_API_VERSION)"

.PHONY: Gimp-pythondocs GimpUi-pythondocs
all-local: Gimp-pythondocs GimpUi-pythondocs

endif
