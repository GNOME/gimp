## Process this file with automake to produce Makefile.in

SUBDIRS = \
	m4macros		\
	tools			\
	regexrepl		\
	cursors			\
	themes			\
	po			\
	po-libgimp		\
	po-plug-ins		\
	po-script-fu		\
	data			\
	tips			\
	libgimpbase		\
	libgimpcolor		\
	libgimpmath		\
	libgimpmodule		\
	libgimpthumb		\
	libgimpwidgets		\
	libgimp			\
	app			\
	$(GIMP_MODULES)		\
	$(GIMP_PLUGINS)		\
	etc			\
	$(GIMP_DEVEL_DOCS)	\
	docs

bin_SCRIPTS = gimptool-$(LT_RELEASE) @GIMPINSTALL@

pkgconfigdir = $(libdir)/pkgconfig
pkgconfig_DATA = \
	gimp-$(LT_RELEASE).pc		\
	gimpthumb-$(LT_RELEASE).pc	\
	gimpui-$(LT_RELEASE).pc

EXTRA_SCRIPTS = gimpinstall-$(LT_RELEASE)

EXTRA_DIST = \
	AUTHORS			\
	COPYING			\
	ChangeLog		\
	ChangeLog.pre-1-0	\
	ChangeLog.pre-1-2	\
	HACKING			\
	INSTALL			\
	LICENSE			\
	MAINTAINERS		\
	NEWS			\
	PLUGIN_MAINTAINERS	\
	README			\
	README.i18n		\
	README.win32		\
	TODO.xml		\
	config.h.win32		\
	gimp.spec.in		\
	gimp.spec		\
	gimp-zip.in		\
	gimptool-win32.c.in	\
	intltool-extract.in	\
	intltool-merge.in	\
	intltool-update.in	\
	makefile.msc	\
	po-libgimp/update.sh	\
	po-plug-ins/update.sh	\
	po-script-fu/update.sh

DISTCLEANFILES = \
	intltool-extract	\
	intltool-merge		\
	intltool-update


gimpinstall-@LT_RELEASE@:
	$(LN_S) $(srcdir)/install-sh $(srcdir)/gimpinstall-$(LT_RELEASE)

install-exec-hook:
if DEFAULT_BINARY
	cd $(DESTDIR)$(bindir) \
	&& rm -f gimptool \
	&& $(LN_S) gimptool-$(LT_RELEASE) gimptool
endif

uninstall-local:
if DEFAULT_BINARY
	rm -f $(DESTDIR)$(bindir)/gimptool
endif
