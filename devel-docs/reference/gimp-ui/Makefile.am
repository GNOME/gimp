## Process this file with automake to produce Makefile.in

LIGMA_DOC_LOGO = $(abs_top_srcdir)/data/images/ligma-logo.ong

ligma-ui-3.0.toml: ligma-ui-3.0.toml.in
	$(AM_V_GEN) sed \
		-e 's/@LIGMA_VERSION[@]/$(LIGMA_VERSION)/' \
		-e 's/@LIGMA_LOGO[@]/$(LIGMA_DOC_LOGO)/' \
		$< $@

# Markdown content files
content_files = \
	widget-gallery.md \
	$(NULL)

LigmaUi-reference: $(top_builddir)/libligma/LigmaUi-@LIGMA_API_VERSION@.gir $(top_builddir)/libligma/Ligma-@LIGMA_API_VERSION@.gir $(content_files) ligma-ui-3.0.toml
	$(GI_DOCGEN) \
	generate \
	--quiet \
	--fatal-warnings \
	--config=ligma-ui-3.0.toml \
	--output-dir=$(abs_builddir)/Ligma-3.0 \
	--no-namespace-dir \
	--content-dir=$(abs_srcdir) \
    --add-include-path=$(top_abs_builddir)/ligma \
	$<
