if ENABLE_G_IR_DOC

Ligma-%-docs: $(top_builddir)/libligma/Ligma-@LIGMA_API_VERSION@.gir
	$(GIR_DOC_TOOL) -I "$(prefix)/share/gir-1.0/"			\
		-I "$(top_builddir)/libligma" --language=$*		\
		-o "$(builddir)/pages/$*/Ligma-$(LIGMA_API_VERSION)"	\
		$<
	sed -i 's/<\(Prefix\|Image\)/\&lt;\1/g' "$(builddir)/pages/$*/Ligma-$(LIGMA_API_VERSION)/Ligma.Procedure.add_menu_path.page"
	sed -i 's/&\([a-z0-9_]\+[^a-z0-9_;]\)/\&amp;\1/g' "$(builddir)/pages/$*/Ligma-$(LIGMA_API_VERSION)/Ligma.checks_get_colors.page"
	$(YELP_BUILD) cache								\
		-o "$(builddir)/pages/$*/Ligma-$(LIGMA_API_VERSION)/index.cache"	\
		"$(builddir)/pages/$*/Ligma-$(LIGMA_API_VERSION)/"
	mkdir -p "$(builddir)/html/$*/Ligma-$(LIGMA_API_VERSION)"
	$(YELP_BUILD) html						\
		-o "$(builddir)/html/$*/Ligma-$(LIGMA_API_VERSION)"	\
		"$(builddir)/pages/$*/Ligma-$(LIGMA_API_VERSION)/"
	echo "$* Ligma module documentation: $(builddir)/html/Python/Ligma-$(LIGMA_API_VERSION)"

LigmaUi-%-docs: $(top_builddir)/libligma/LigmaUi-@LIGMA_API_VERSION@.gir Ligma-%-docs
	$(GIR_DOC_TOOL) -I "$(prefix)/share/gir-1.0/"			\
		-I "$(top_builddir)/libligma" --language=$*		\
		-o "$(builddir)/pages/$*/LigmaUi-$(LIGMA_API_VERSION)"	\
		$<
	$(YELP_BUILD) cache								\
		-o "$(builddir)/pages/$*/LigmaUi-$(LIGMA_API_VERSION)/index.cache"	\
		"$(builddir)/pages/$*/LigmaUi-$(LIGMA_API_VERSION)/"
	mkdir -p "$(builddir)/html/$*/LigmaUi-$(LIGMA_API_VERSION)"
	$(YELP_BUILD) html						\
		-o "$(builddir)/html/$*/LigmaUi-$(LIGMA_API_VERSION)"	\
		"$(builddir)/pages/$*/LigmaUi-$(LIGMA_API_VERSION)/"
	echo "$* LigmaUi module documentation: $(builddir)/html/Python/LigmaUi-$(LIGMA_API_VERSION)"

all-local: LigmaUi-gjs-docs LigmaUi-python-docs

endif
