## Process this file with automake to produce Makefile.in

GIMP_DOC_LOGO = $(abs_top_srcdir)/data/images/gimp-logo.ong

gimp-ui-3.0.toml: gimp-ui-3.0.toml.in
	$(AM_V_GEN) sed \
		-e 's/@GIMP_VERSION[@]/$(GIMP_VERSION)/' \
		-e 's/@GIMP_LOGO[@]/$(GIMP_DOC_LOGO)/' \
		$< $@

# Markdown content files
content_files = \
	widget-gallery.md \
	$(NULL)

GimpUi-reference: $(top_builddir)/libgimp/GimpUi-@GIMP_API_VERSION@.gir $(top_builddir)/libgimp/Gimp-@GIMP_API_VERSION@.gir $(content_files) gimp-ui-3.0.toml
	$(GI_DOCGEN) \
	generate \
	--quiet \
	--fatal-warnings \
	--config=gimp-ui-3.0.toml \
	--output-dir=$(abs_builddir)/Gimp-3.0 \
	--no-namespace-dir \
	--content-dir=$(abs_srcdir) \
    --add-include-path=$(top_abs_builddir)/gimp \
	$<
