## Process this file with automake to produce Makefile.in

SUBDIRS = \
	m4macros		\
	tools			\
	regexrepl		\
	pixmaps			\
	cursors			\
	themes			\
	po			\
	po-libgimp		\
	po-plug-ins		\
	po-script-fu		\
	$(GIMP_PERL_PO)		\
	libgimpbase		\
	libgimpcolor		\
	libgimpmath		\
	libgimpwidgets		\
	libgimp			\
	app			\
	$(GIMP_MODULES)		\
	$(GIMP_PLUGINS)		\
	data			\
	etc			\
	tips			\
	docs			\
	devel-docs		\
	build

bin_SCRIPTS = gimptool-1.3 @GIMPINSTALL@

pkgconfigdir = $(libdir)/pkgconfig
pkgconfig_DATA = gimp-1.3.pc gimpui-1.3.pc

EXTRA_SCRIPTS = gimpinstall

EXTRA_DIST = \
	AUTHORS				\
	COPYING				\
	ChangeLog			\
	ChangeLog.pre-1-0		\
	ChangeLog.pre-1-2		\
	HACKING				\
	INSTALL				\
	MAINTAINERS			\
	NEWS				\
	PLUGIN_MAINTAINERS		\
	README				\
	README.i18n			\
	README.perl			\
	README.win32			\
	TODO				\
	TODO.xml			\
	config.h.win32			\
	po/po2tbl.sed.in		\
	po/update.sh			\
	po/update.pl			\
	po-libgimp/update.sh		\
	po-plug-ins/update.sh		\
	po-script-fu/script-fu-xgettext	\
	po-script-fu/update.sh

gimpinstall:
	$(LN_S) $(srcdir)/install-sh $(srcdir)/gimpinstall

install-exec-local:
	cd $(DESTDIR)$(bindir) \
	&& rm -f gimp-config-1.3 \
	&& $(LN_S) gimptool-1.3 gimp-config-1.3

uninstall-local:
	rm -f $(DESTDIR)$(bindir)/gimp-config-1.3
