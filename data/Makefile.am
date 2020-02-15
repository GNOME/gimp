## Makefile.am for gimp/data

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

fontdir = $(gimpdatadir)/fonts

install-data-local:
	$(mkinstalldirs) $(DESTDIR)$(fontdir)
	echo '[package]' > $(DESTDIR)$(gimpdatadir)/gimp-release
	echo 'revision=@GIMP_PACKAGE_REVISION@' >> $(DESTDIR)$(gimpdatadir)/gimp-release

uninstall-local:
	rm -f $(DESTDIR)$(gimpdatadir)/gimp-release
