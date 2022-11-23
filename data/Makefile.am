## Makefile.am for ligma/data

SUBDIRS = \
	brushes		\
	dynamics	\
	environ		\
	gradients	\
	images		\
	interpreters	\
	palettes	\
	patterns	\
	tips		\
	tags		\
	tool-presets

fontdir = $(ligmadatadir)/fonts

install-data-local:
	$(mkinstalldirs) $(DESTDIR)$(fontdir)
	echo '[package]' > $(DESTDIR)$(ligmadatadir)/ligma-release
	echo 'revision=@LIGMA_PACKAGE_REVISION@' >> $(DESTDIR)$(ligmadatadir)/ligma-release

uninstall-local:
	rm -f $(DESTDIR)$(ligmadatadir)/ligma-release
