if ENABLE_G_IR_DOC

Gimp-%-docs: $(top_builddir)/libgimp/Gimp-@GIMP_API_VERSION@.gir
	$(GIR_DOC_TOOL) -I "$(prefix)/share/gir-1.0/"			\
		-I "$(top_builddir)/libgimp" --language=$*		\
		-o "$(builddir)/pages/$*/Gimp-$(GIMP_API_VERSION)"	\
		$<
	sed -i 's/<\(Prefix\|Image\)/\&lt;\1/g' "$(builddir)/pages/$*/Gimp-$(GIMP_API_VERSION)/Gimp.Procedure.add_menu_path.page"
	$(YELP_BUILD) cache								\
		-o "$(builddir)/pages/$*/Gimp-$(GIMP_API_VERSION)/index.cache"	\
		"$(builddir)/pages/$*/Gimp-$(GIMP_API_VERSION)/"
	mkdir -p "$(builddir)/html/$*/Gimp-$(GIMP_API_VERSION)"
	$(YELP_BUILD) html						\
		-o "$(builddir)/html/$*/Gimp-$(GIMP_API_VERSION)"	\
		"$(builddir)/pages/$*/Gimp-$(GIMP_API_VERSION)/"
	echo "$* Gimp module documentation: $(builddir)/html/Python/Gimp-$(GIMP_API_VERSION)"

GimpUi-%-docs: $(top_builddir)/libgimp/GimpUi-@GIMP_API_VERSION@.gir Gimp-%-docs
	$(GIR_DOC_TOOL) -I "$(prefix)/share/gir-1.0/"			\
		-I "$(top_builddir)/libgimp" --language=$*		\
		-o "$(builddir)/pages/$*/GimpUi-$(GIMP_API_VERSION)"	\
		$<
	$(YELP_BUILD) cache								\
		-o "$(builddir)/pages/$*/GimpUi-$(GIMP_API_VERSION)/index.cache"	\
		"$(builddir)/pages/$*/GimpUi-$(GIMP_API_VERSION)/"
	mkdir -p "$(builddir)/html/$*/GimpUi-$(GIMP_API_VERSION)"
	$(YELP_BUILD) html						\
		-o "$(builddir)/html/$*/GimpUi-$(GIMP_API_VERSION)"	\
		"$(builddir)/pages/$*/GimpUi-$(GIMP_API_VERSION)/"
	echo "$* GimpUi module documentation: $(builddir)/html/Python/GimpUi-$(GIMP_API_VERSION)"

all-local: GimpUi-gjs-docs GimpUi-python-docs

endif
